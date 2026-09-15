#include "remapper/ble_hogp/ble_hogp.h"

#include <string.h>

#include "btstack.h"

#define BLE_HOGP_DESCRIPTOR_STORAGE_SIZE 2048u
#define BLE_HOGP_REJECTED_DEVICE_CAPACITY 4u
#define BLE_APPEARANCE_HID_GENERIC 960u
#define BLE_APPEARANCE_HID_MOUSE 962u
#define BLE_APPEARANCE_HID_LAST 1023u

_Static_assert(
    sizeof(remapper_canonical_mouse_event_t) <=
        REMAPPER_BT_RUNTIME_MESSAGE_PAYLOAD_SIZE,
    "canonical mouse event must fit the cross-core runtime message");
_Static_assert(
    sizeof(remapper_ble_hogp_debug_record_t) <=
        REMAPPER_BT_RUNTIME_MESSAGE_PAYLOAD_SIZE,
    "BLE debug record must fit the cross-core runtime message");

typedef enum {
    BLE_HOGP_STATE_WAITING_FOR_STACK = 0,
    BLE_HOGP_STATE_SCANNING,
    BLE_HOGP_STATE_CONNECTING,
    BLE_HOGP_STATE_SECURING,
    BLE_HOGP_STATE_CONNECTING_HIDS,
    BLE_HOGP_STATE_READY,
    BLE_HOGP_STATE_DISCONNECTING,
} ble_hogp_state_t;

typedef struct {
    bool used;
    bd_addr_type_t address_type;
    bd_addr_t address;
} ble_hogp_rejected_device_t;

static ble_hogp_state_t g_state;
static bd_addr_t g_remote_address;
static bd_addr_type_t g_remote_address_type;
static hci_con_handle_t g_connection_handle = HCI_CON_HANDLE_INVALID;
static uint16_t g_hids_cid;
static uint8_t g_descriptor_storage[BLE_HOGP_DESCRIPTOR_STORAGE_SIZE];
static remapper_ble_hogp_parser_t g_parser;
static ble_hogp_rejected_device_t g_rejected_devices[BLE_HOGP_REJECTED_DEVICE_CAPACITY];
static size_t g_rejected_next;
static btstack_packet_callback_registration_t g_hci_registration;
static btstack_packet_callback_registration_t g_sm_registration;

static void handle_gatt_client_event(
    uint8_t packet_type,
    uint16_t channel,
    uint8_t *packet,
    uint16_t size);

static void publish_debug(
    remapper_ble_hogp_debug_code_t code,
    uint8_t status,
    uint8_t report_id,
    int32_t a,
    int32_t b,
    int32_t c)
{
#if defined(REMAPPER_USB_DEBUG_CDC) && REMAPPER_USB_DEBUG_CDC
    const remapper_ble_hogp_debug_record_t record = {
        .code = (uint8_t)code,
        .status = status,
        .report_id = report_id,
        .reserved = 0u,
        .a = a,
        .b = b,
        .c = c,
    };
    (void)remapper_bt_runtime_publish(
        REMAPPER_BLE_HOGP_RUNTIME_CHANNEL,
        REMAPPER_BLE_HOGP_MESSAGE_DEBUG,
        &record,
        (uint16_t)sizeof(record));
#else
    (void)code;
    (void)status;
    (void)report_id;
    (void)a;
    (void)b;
    (void)c;
#endif
}

static bool publish_status(remapper_ble_hogp_message_type_t type)
{
    return remapper_bt_runtime_publish(
        REMAPPER_BLE_HOGP_RUNTIME_CHANNEL,
        (uint16_t)type,
        NULL,
        0u);
}

static bool publish_mouse_event(
    void *context,
    const remapper_canonical_mouse_event_t *event)
{
    (void)context;
    return remapper_bt_runtime_publish(
        REMAPPER_BLE_HOGP_RUNTIME_CHANNEL,
        REMAPPER_BLE_HOGP_MESSAGE_MOUSE,
        event,
        (uint16_t)sizeof(*event));
}

static bool advertisement_has_hid_service(const uint8_t *packet)
{
    const uint8_t *data = gap_event_advertising_report_get_data(packet);
    const uint8_t length = gap_event_advertising_report_get_data_length(packet);
    return ad_data_contains_uuid16(
        length,
        data,
        ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE);
}

static uint16_t advertisement_appearance(const uint8_t *packet)
{
    const uint8_t *data = gap_event_advertising_report_get_data(packet);
    const uint8_t length = gap_event_advertising_report_get_data_length(packet);
    ad_context_t context;
    for (ad_iterator_init(&context, length, (uint8_t *)data);
         ad_iterator_has_more(&context);
         ad_iterator_next(&context)) {
        if (ad_iterator_get_data_type(&context) != BLUETOOTH_DATA_TYPE_APPEARANCE ||
            ad_iterator_get_data_len(&context) < 2u) {
            continue;
        }
        return little_endian_read_16(ad_iterator_get_data(&context), 0u);
    }
    return 0u;
}

static bool appearance_is_explicit_non_mouse_hid(uint16_t appearance)
{
    return appearance >= BLE_APPEARANCE_HID_GENERIC &&
        appearance <= BLE_APPEARANCE_HID_LAST &&
        appearance != BLE_APPEARANCE_HID_GENERIC &&
        appearance != BLE_APPEARANCE_HID_MOUSE;
}

static bool address_is_rejected(
    const bd_addr_t address,
    bd_addr_type_t address_type)
{
    for (size_t index = 0u; index < BLE_HOGP_REJECTED_DEVICE_CAPACITY; ++index) {
        if (!g_rejected_devices[index].used ||
            g_rejected_devices[index].address_type != address_type) {
            continue;
        }
        if (memcmp(g_rejected_devices[index].address, address, sizeof(bd_addr_t)) == 0) {
            return true;
        }
    }
    return false;
}

static void reject_address(
    const bd_addr_t address,
    bd_addr_type_t address_type)
{
    if (address_is_rejected(address, address_type)) return;
    ble_hogp_rejected_device_t *slot = &g_rejected_devices[g_rejected_next];
    slot->used = true;
    slot->address_type = address_type;
    memcpy(slot->address, address, sizeof(bd_addr_t));
    g_rejected_next = (g_rejected_next + 1u) % BLE_HOGP_REJECTED_DEVICE_CAPACITY;
}

static void start_scan(void)
{
    g_state = BLE_HOGP_STATE_SCANNING;
    gap_set_scan_parameters(0u, 48u, 48u);
    gap_start_scan();
    publish_debug(REMAPPER_BLE_HOGP_DEBUG_SCAN_STARTED, 0u, 0u, 0, 0, 0);
}

static void disconnect_and_rescan(void)
{
    const ble_hogp_state_t previous_state = g_state;
    publish_debug(
        REMAPPER_BLE_HOGP_DEBUG_RESCAN,
        0u,
        0u,
        (int32_t)previous_state,
        0,
        0);
    if (previous_state == BLE_HOGP_STATE_READY) {
        (void)publish_status(REMAPPER_BLE_HOGP_MESSAGE_DISCONNECTED);
    }
    g_state = BLE_HOGP_STATE_DISCONNECTING;
    if (g_connection_handle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(g_connection_handle);
    } else {
        start_scan();
    }
}

static void connect_hid_service(void)
{
    g_state = BLE_HOGP_STATE_CONNECTING_HIDS;
    publish_debug(
        REMAPPER_BLE_HOGP_DEBUG_HIDS_CONNECTING,
        0u,
        0u,
        (int32_t)g_connection_handle,
        0,
        0);
    g_hids_cid = 0u;
    const uint8_t status = hids_client_connect(
        g_connection_handle,
        &handle_gatt_client_event,
        HID_PROTOCOL_MODE_REPORT,
        &g_hids_cid);
    if (status != ERROR_CODE_SUCCESS) {
        publish_debug(REMAPPER_BLE_HOGP_DEBUG_ERROR, status, 0u, 4, 0, 0);
        disconnect_and_rescan();
    }
}

static void handle_gatt_client_event(
    uint8_t packet_type,
    uint16_t channel,
    uint8_t *packet,
    uint16_t size)
{
    (void)packet_type;
    (void)channel;
    (void)size;

    if (hci_event_packet_get_type(packet) != HCI_EVENT_GATTSERVICE_META) {
        return;
    }

    switch (hci_event_gattservice_meta_get_subevent_code(packet)) {
    case GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED: {
        const uint8_t status =
            gattservice_subevent_hid_service_connected_get_status(packet);
        publish_debug(
            REMAPPER_BLE_HOGP_DEBUG_HIDS_CONNECTED,
            status,
            0u,
            (int32_t)g_hids_cid,
            0,
            0);
        if (status != ERROR_CODE_SUCCESS) {
            publish_debug(REMAPPER_BLE_HOGP_DEBUG_ERROR, status, 0u, 1, 0, 0);
            disconnect_and_rescan();
            return;
        }

        const uint8_t *descriptor =
            hids_client_descriptor_storage_get_descriptor_data(g_hids_cid, 0u);
        const uint16_t descriptor_len =
            hids_client_descriptor_storage_get_descriptor_len(g_hids_cid, 0u);
        publish_debug(
            REMAPPER_BLE_HOGP_DEBUG_REPORT_MAP,
            0u,
            0u,
            (int32_t)descriptor_len,
            0,
            0);
        const remapper_hid_source_t source =
            remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1u);

        const bool parser_ready = descriptor != NULL && descriptor_len > 0u &&
            remapper_ble_hogp_parser_configure(
                &g_parser,
                source,
                descriptor,
                descriptor_len) &&
            remapper_ble_hogp_parser_has_mouse(&g_parser);
        if (!parser_ready) {
            publish_debug(
                REMAPPER_BLE_HOGP_DEBUG_ERROR,
                0u,
                0u,
                2,
                (int32_t)descriptor_len,
                0);
            /* A valid HIDS report map with no Mouse collection is a keyboard/
             * other HID. Remember it so scan does not pair it in a loop. */
            if (descriptor != NULL && descriptor_len > 0u) {
                reject_address(g_remote_address, g_remote_address_type);
            }
            disconnect_and_rescan();
            return;
        }

        publish_debug(
            REMAPPER_BLE_HOGP_DEBUG_PARSER_READY,
            0u,
            0u,
            (int32_t)g_parser.field_count,
            (int32_t)g_parser.report_count,
            0);
        g_state = BLE_HOGP_STATE_READY;
        (void)publish_status(REMAPPER_BLE_HOGP_MESSAGE_CONNECTED);
        break;
    }

    case GATTSERVICE_SUBEVENT_HID_SERVICE_DISCONNECTED:
        publish_debug(REMAPPER_BLE_HOGP_DEBUG_ERROR, 0u, 0u, 5, 0, 0);
        if (g_state != BLE_HOGP_STATE_DISCONNECTING) {
            disconnect_and_rescan();
        }
        break;

    case GATTSERVICE_SUBEVENT_HID_REPORT: {
        if (g_state != BLE_HOGP_STATE_READY) break;
        const uint8_t report_id =
            gattservice_subevent_hid_report_get_report_id(packet);
        const uint16_t report_len =
            gattservice_subevent_hid_report_get_report_len(packet);
        publish_debug(
            REMAPPER_BLE_HOGP_DEBUG_REPORT_RX,
            0u,
            report_id,
            (int32_t)report_len,
            0,
            0);
        if (!remapper_ble_hogp_parser_parse_report(
                &g_parser,
                report_id,
                gattservice_subevent_hid_report_get_report(packet),
                report_len,
                publish_mouse_event,
                NULL)) {
            publish_debug(
                REMAPPER_BLE_HOGP_DEBUG_ERROR,
                0u,
                report_id,
                3,
                (int32_t)report_len,
                0);
            disconnect_and_rescan();
        }
        break;
    }

    default:
        break;
    }
}

static void hci_packet_handler(
    uint8_t packet_type,
    uint16_t channel,
    uint8_t *packet,
    uint16_t size)
{
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING &&
            g_state == BLE_HOGP_STATE_WAITING_FOR_STACK) {
            publish_debug(REMAPPER_BLE_HOGP_DEBUG_STACK_READY, 0u, 0u, 0, 0, 0);
            start_scan();
        }
        break;

    case GAP_EVENT_ADVERTISING_REPORT: {
        if (g_state != BLE_HOGP_STATE_SCANNING ||
            !advertisement_has_hid_service(packet)) {
            break;
        }

        bd_addr_t candidate_address;
        gap_event_advertising_report_get_address(packet, candidate_address);
        const bd_addr_type_t candidate_type =
            gap_event_advertising_report_get_address_type(packet);
        const uint16_t appearance = advertisement_appearance(packet);
        publish_debug(
            REMAPPER_BLE_HOGP_DEBUG_HID_ADVERTISEMENT,
            (uint8_t)candidate_type,
            0u,
            (int32_t)gap_event_advertising_report_get_rssi(packet),
            (int32_t)appearance,
            0);

        if (address_is_rejected(candidate_address, candidate_type)) {
            break;
        }
        if (appearance_is_explicit_non_mouse_hid(appearance)) {
            reject_address(candidate_address, candidate_type);
            publish_debug(
                REMAPPER_BLE_HOGP_DEBUG_ERROR,
                0u,
                0u,
                6,
                (int32_t)appearance,
                0);
            break;
        }

        gap_stop_scan();
        memcpy(g_remote_address, candidate_address, sizeof(bd_addr_t));
        g_remote_address_type = candidate_type;
        g_state = BLE_HOGP_STATE_CONNECTING;
        publish_debug(
            REMAPPER_BLE_HOGP_DEBUG_CONNECTING,
            (uint8_t)g_remote_address_type,
            0u,
            (int32_t)appearance,
            0,
            0);
        gap_connect(g_remote_address, g_remote_address_type);
        break;
    }

    case HCI_EVENT_META_GAP:
        if (hci_event_gap_meta_get_subevent_code(packet) !=
                GAP_SUBEVENT_LE_CONNECTION_COMPLETE ||
            g_state != BLE_HOGP_STATE_CONNECTING) {
            break;
        }
        {
            const uint8_t status =
                gap_subevent_le_connection_complete_get_status(packet);
            const hci_con_handle_t handle =
                gap_subevent_le_connection_complete_get_connection_handle(packet);
            publish_debug(
                REMAPPER_BLE_HOGP_DEBUG_LE_CONNECTED,
                status,
                0u,
                (int32_t)handle,
                0,
                0);
            if (status != ERROR_CODE_SUCCESS) {
                g_connection_handle = HCI_CON_HANDLE_INVALID;
                publish_debug(REMAPPER_BLE_HOGP_DEBUG_ERROR, status, 0u, 7, 0, 0);
                start_scan();
                break;
            }
            g_connection_handle = handle;
        }
        g_state = BLE_HOGP_STATE_SECURING;
        publish_debug(
            REMAPPER_BLE_HOGP_DEBUG_PAIRING_STARTED,
            0u,
            0u,
            (int32_t)g_connection_handle,
            0,
            0);
        sm_request_pairing(g_connection_handle);
        break;

    case HCI_EVENT_DISCONNECTION_COMPLETE: {
        const bool was_ready = g_state == BLE_HOGP_STATE_READY;
        publish_debug(
            REMAPPER_BLE_HOGP_DEBUG_DISCONNECTED,
            hci_event_disconnection_complete_get_reason(packet),
            0u,
            (int32_t)g_connection_handle,
            0,
            0);
        g_connection_handle = HCI_CON_HANDLE_INVALID;
        g_hids_cid = 0u;
        memset(&g_parser, 0, sizeof(g_parser));
        if (was_ready) {
            (void)publish_status(REMAPPER_BLE_HOGP_MESSAGE_DISCONNECTED);
        }
        start_scan();
        break;
    }

    default:
        break;
    }
}

static void sm_packet_handler(
    uint8_t packet_type,
    uint16_t channel,
    uint8_t *packet,
    uint16_t size)
{
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET) return;

    bool security_ready = false;
    switch (hci_event_packet_get_type(packet)) {
    case SM_EVENT_JUST_WORKS_REQUEST:
        sm_just_works_confirm(
            sm_event_just_works_request_get_handle(packet));
        break;

    case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
        sm_numeric_comparison_confirm(
            sm_event_passkey_display_number_get_handle(packet));
        break;

    case SM_EVENT_PAIRING_COMPLETE: {
        const uint8_t status = sm_event_pairing_complete_get_status(packet);
        publish_debug(
            REMAPPER_BLE_HOGP_DEBUG_PAIRING_COMPLETE,
            status,
            0u,
            (int32_t)g_connection_handle,
            0,
            0);
        if (status == ERROR_CODE_SUCCESS) {
            security_ready = true;
        } else {
            disconnect_and_rescan();
        }
        break;
    }

    case SM_EVENT_REENCRYPTION_COMPLETE: {
        const uint8_t status = sm_event_reencryption_complete_get_status(packet);
        publish_debug(
            REMAPPER_BLE_HOGP_DEBUG_PAIRING_COMPLETE,
            status,
            0u,
            (int32_t)g_connection_handle,
            1,
            0);
        if (status == ERROR_CODE_SUCCESS) {
            security_ready = true;
        } else {
            publish_debug(REMAPPER_BLE_HOGP_DEBUG_ERROR, status, 0u, 8, 0, 0);
            disconnect_and_rescan();
        }
        break;
    }

    default:
        break;
    }

    if (security_ready && g_state == BLE_HOGP_STATE_SECURING) {
        connect_hid_service();
    }
}

static void ble_hogp_session_setup(void)
{
    memset(&g_parser, 0, sizeof(g_parser));
    memset(g_rejected_devices, 0, sizeof(g_rejected_devices));
    g_rejected_next = 0u;
    g_state = BLE_HOGP_STATE_WAITING_FOR_STACK;
    g_connection_handle = HCI_CON_HANDLE_INVALID;
    g_hids_cid = 0u;
    publish_debug(REMAPPER_BLE_HOGP_DEBUG_SESSION_SETUP, 0u, 0u, 0, 0, 0);

    hids_client_init(g_descriptor_storage, sizeof(g_descriptor_storage));

    g_hci_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&g_hci_registration);

    g_sm_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&g_sm_registration);
}

bool remapper_ble_hogp_start(void)
{
    return remapper_bt_runtime_start(ble_hogp_session_setup);
}
