#include "remapper/ble_hogp/ble_hogp.h"

#include <string.h>

#include "btstack.h"

#define BLE_HOGP_DESCRIPTOR_STORAGE_SIZE 1024u

_Static_assert(
    sizeof(remapper_canonical_mouse_event_t) <=
        REMAPPER_BT_RUNTIME_MESSAGE_PAYLOAD_SIZE,
    "canonical mouse event must fit the cross-core runtime message");

typedef enum {
    BLE_HOGP_STATE_WAITING_FOR_STACK = 0,
    BLE_HOGP_STATE_SCANNING,
    BLE_HOGP_STATE_CONNECTING,
    BLE_HOGP_STATE_SECURING,
    BLE_HOGP_STATE_CONNECTING_HIDS,
    BLE_HOGP_STATE_READY,
} ble_hogp_state_t;

static ble_hogp_state_t g_state;
static bd_addr_t g_remote_address;
static bd_addr_type_t g_remote_address_type;
static hci_con_handle_t g_connection_handle = HCI_CON_HANDLE_INVALID;
static uint16_t g_hids_cid;
static uint8_t g_descriptor_storage[BLE_HOGP_DESCRIPTOR_STORAGE_SIZE];
static remapper_ble_hogp_parser_t g_parser;
static btstack_packet_callback_registration_t g_hci_registration;
static btstack_packet_callback_registration_t g_sm_registration;

static void handle_gatt_client_event(
    uint8_t packet_type,
    uint16_t channel,
    uint8_t *packet,
    uint16_t size);

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

static void start_scan(void)
{
    g_state = BLE_HOGP_STATE_SCANNING;
    gap_set_scan_parameters(0u, 48u, 48u);
    gap_start_scan();
}

static void disconnect_and_rescan(void)
{
    if (g_connection_handle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(g_connection_handle);
    } else {
        start_scan();
    }
}

static void connect_hid_service(void)
{
    g_state = BLE_HOGP_STATE_CONNECTING_HIDS;
    hids_client_connect(
        g_connection_handle,
        &handle_gatt_client_event,
        HID_PROTOCOL_MODE_REPORT,
        &g_hids_cid);
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
        if (status != ERROR_CODE_SUCCESS) {
            disconnect_and_rescan();
            return;
        }

        const uint8_t *descriptor =
            hids_client_descriptor_storage_get_descriptor_data(g_hids_cid, 0u);
        const uint16_t descriptor_len =
            hids_client_descriptor_storage_get_descriptor_len(g_hids_cid, 0u);
        const remapper_hid_source_t source =
            remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1u);

        if (descriptor == NULL || descriptor_len == 0u ||
            !remapper_ble_hogp_parser_configure(
                &g_parser,
                source,
                descriptor,
                descriptor_len) ||
            !remapper_ble_hogp_parser_has_mouse(&g_parser)) {
            disconnect_and_rescan();
            return;
        }

        g_state = BLE_HOGP_STATE_READY;
        (void)publish_status(REMAPPER_BLE_HOGP_MESSAGE_CONNECTED);
        break;
    }

    case GATTSERVICE_SUBEVENT_HID_SERVICE_DISCONNECTED:
        break;

    case GATTSERVICE_SUBEVENT_HID_REPORT:
        if (g_state != BLE_HOGP_STATE_READY) break;
        if (!remapper_ble_hogp_parser_parse_report(
                &g_parser,
                gattservice_subevent_hid_report_get_report_id(packet),
                gattservice_subevent_hid_report_get_report(packet),
                gattservice_subevent_hid_report_get_report_len(packet),
                publish_mouse_event,
                NULL)) {
            disconnect_and_rescan();
        }
        break;

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
            start_scan();
        }
        break;

    case GAP_EVENT_ADVERTISING_REPORT:
        if (g_state != BLE_HOGP_STATE_SCANNING ||
            !advertisement_has_hid_service(packet)) {
            break;
        }
        gap_stop_scan();
        gap_event_advertising_report_get_address(packet, g_remote_address);
        g_remote_address_type =
            gap_event_advertising_report_get_address_type(packet);
        g_state = BLE_HOGP_STATE_CONNECTING;
        gap_connect(g_remote_address, g_remote_address_type);
        break;

    case HCI_EVENT_META_GAP:
        if (hci_event_gap_meta_get_subevent_code(packet) !=
                GAP_SUBEVENT_LE_CONNECTION_COMPLETE ||
            g_state != BLE_HOGP_STATE_CONNECTING) {
            break;
        }
        g_connection_handle =
            gap_subevent_le_connection_complete_get_connection_handle(packet);
        g_state = BLE_HOGP_STATE_SECURING;
        sm_request_pairing(g_connection_handle);
        break;

    case HCI_EVENT_DISCONNECTION_COMPLETE: {
        const bool was_ready = g_state == BLE_HOGP_STATE_READY;
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

    case SM_EVENT_PAIRING_COMPLETE:
        if (sm_event_pairing_complete_get_status(packet) == ERROR_CODE_SUCCESS) {
            security_ready = true;
        } else {
            disconnect_and_rescan();
        }
        break;

    case SM_EVENT_REENCRYPTION_COMPLETE:
        security_ready = true;
        break;

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
    g_state = BLE_HOGP_STATE_WAITING_FOR_STACK;
    g_connection_handle = HCI_CON_HANDLE_INVALID;
    g_hids_cid = 0u;

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
