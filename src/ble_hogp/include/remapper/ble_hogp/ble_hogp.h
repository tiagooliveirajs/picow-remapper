#ifndef REMAPPER_BLE_HOGP_BLE_HOGP_H
#define REMAPPER_BLE_HOGP_BLE_HOGP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "remapper/bt_runtime/bt_runtime.h"
#include "remapper/domain/hid.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REMAPPER_BLE_HOGP_MAX_FIELDS 48u
#define REMAPPER_BLE_HOGP_MAX_REPORTS 16u
#define REMAPPER_BLE_HOGP_RUNTIME_CHANNEL UINT16_C(0x0601)

typedef enum {
    REMAPPER_BLE_HOGP_MESSAGE_CONNECTED = 1,
    REMAPPER_BLE_HOGP_MESSAGE_DISCONNECTED = 2,
    REMAPPER_BLE_HOGP_MESSAGE_MOUSE = 3,
    REMAPPER_BLE_HOGP_MESSAGE_DEBUG = 4,
} remapper_ble_hogp_message_type_t;

typedef enum {
    REMAPPER_BLE_HOGP_DEBUG_SESSION_SETUP = 1,
    REMAPPER_BLE_HOGP_DEBUG_STACK_READY,
    REMAPPER_BLE_HOGP_DEBUG_SCAN_STARTED,
    REMAPPER_BLE_HOGP_DEBUG_HID_ADVERTISEMENT,
    REMAPPER_BLE_HOGP_DEBUG_CONNECTING,
    REMAPPER_BLE_HOGP_DEBUG_LE_CONNECTED,
    REMAPPER_BLE_HOGP_DEBUG_PAIRING_STARTED,
    REMAPPER_BLE_HOGP_DEBUG_PAIRING_COMPLETE,
    REMAPPER_BLE_HOGP_DEBUG_HIDS_CONNECTING,
    REMAPPER_BLE_HOGP_DEBUG_HIDS_CONNECTED,
    REMAPPER_BLE_HOGP_DEBUG_REPORT_MAP,
    REMAPPER_BLE_HOGP_DEBUG_PARSER_READY,
    REMAPPER_BLE_HOGP_DEBUG_REPORT_RX,
    REMAPPER_BLE_HOGP_DEBUG_DISCONNECTED,
    REMAPPER_BLE_HOGP_DEBUG_RESCAN,
    REMAPPER_BLE_HOGP_DEBUG_ERROR,
} remapper_ble_hogp_debug_code_t;

typedef struct {
    uint8_t code;
    uint8_t status;
    uint8_t report_id;
    uint8_t reserved;
    int32_t a;
    int32_t b;
    int32_t c;
} remapper_ble_hogp_debug_record_t;

typedef enum {
    REMAPPER_BLE_HOGP_FIELD_BUTTON = 0,
    REMAPPER_BLE_HOGP_FIELD_X,
    REMAPPER_BLE_HOGP_FIELD_Y,
    REMAPPER_BLE_HOGP_FIELD_WHEEL,
    REMAPPER_BLE_HOGP_FIELD_PAN,
} remapper_ble_hogp_field_kind_t;

typedef struct {
    uint8_t report_id;
    remapper_ble_hogp_field_kind_t kind;
    uint16_t bit_offset;
    uint8_t bit_size;
    uint8_t button_index;
    bool signed_value;
} remapper_ble_hogp_field_t;

typedef struct {
    uint8_t report_id;
    uint16_t input_bits;
    uint8_t button_mask;
} remapper_ble_hogp_report_state_t;

typedef struct {
    remapper_hid_source_t source;
    remapper_ble_hogp_field_t fields[REMAPPER_BLE_HOGP_MAX_FIELDS];
    size_t field_count;
    remapper_ble_hogp_report_state_t reports[REMAPPER_BLE_HOGP_MAX_REPORTS];
    size_t report_count;
    uint8_t aggregate_buttons;
    bool configured;
} remapper_ble_hogp_parser_t;

typedef bool (*remapper_ble_hogp_emit_fn)(
    void *context,
    const remapper_canonical_mouse_event_t *event);

typedef enum {
    REMAPPER_BLE_HOGP_EVENT_CONNECTED = 0,
    REMAPPER_BLE_HOGP_EVENT_DISCONNECTED,
    REMAPPER_BLE_HOGP_EVENT_MOUSE,
} remapper_ble_hogp_event_type_t;

typedef struct {
    remapper_ble_hogp_event_type_t type;
    remapper_canonical_mouse_event_t mouse;
} remapper_ble_hogp_event_t;

bool remapper_ble_hogp_parser_configure(
    remapper_ble_hogp_parser_t *parser,
    remapper_hid_source_t source,
    const uint8_t *descriptor,
    size_t descriptor_len);
bool remapper_ble_hogp_parser_has_mouse(
    const remapper_ble_hogp_parser_t *parser);
bool remapper_ble_hogp_parser_parse_report(
    remapper_ble_hogp_parser_t *parser,
    uint8_t report_id,
    const uint8_t *report,
    size_t report_len,
    remapper_ble_hogp_emit_fn emit,
    void *context);

bool remapper_ble_hogp_decode_runtime_message(
    const remapper_bt_runtime_message_t *message,
    remapper_ble_hogp_event_t *event);

bool remapper_ble_hogp_start(void);

#ifdef __cplusplus
}
#endif

#endif
