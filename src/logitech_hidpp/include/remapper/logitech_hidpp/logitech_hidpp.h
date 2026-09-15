#ifndef REMAPPER_LOGITECH_HIDPP_LOGITECH_HIDPP_H
#define REMAPPER_LOGITECH_HIDPP_LOGITECH_HIDPP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REMAPPER_HIDPP_LONG_PAYLOAD_SIZE 19u
#define REMAPPER_HIDPP_REPORT_ID_SHORT 0x10u
#define REMAPPER_HIDPP_REPORT_ID_LONG 0x11u
#define REMAPPER_HIDPP_REPROG_CONTROLS_V4 0x1b04u
#define REMAPPER_HIDPP_FORWARD_CID 0x0056u

typedef enum {
    REMAPPER_HIDPP_OUTPUT_NONE = 0,
    REMAPPER_HIDPP_OUTPUT_GET_FEATURE,
    REMAPPER_HIDPP_OUTPUT_SET_FORWARD_DIVERT,
} remapper_hidpp_output_kind_t;

typedef struct {
    remapper_hidpp_output_kind_t kind;
    uint8_t report_id;
    uint8_t payload[REMAPPER_HIDPP_LONG_PAYLOAD_SIZE];
    size_t payload_len;
} remapper_hidpp_output_t;

typedef struct {
    bool consumed;
    bool held_changed;
    bool forward_held;
} remapper_hidpp_input_result_t;

typedef struct {
    bool connected;
    bool forward_desired;
    bool forward_applied;
    bool forward_held;
    bool feature_failed;
    uint8_t feature_index;
    remapper_hidpp_output_kind_t waiting_for;
    bool pending_enable;
} remapper_logitech_hidpp_t;

void remapper_logitech_hidpp_init(remapper_logitech_hidpp_t *hidpp);
void remapper_logitech_hidpp_on_connect(remapper_logitech_hidpp_t *hidpp);
void remapper_logitech_hidpp_on_disconnect(remapper_logitech_hidpp_t *hidpp);
void remapper_logitech_hidpp_set_forward_desired(
    remapper_logitech_hidpp_t *hidpp,
    bool desired);
bool remapper_logitech_hidpp_next_output(
    remapper_logitech_hidpp_t *hidpp,
    remapper_hidpp_output_t *output);
void remapper_logitech_hidpp_output_result(
    remapper_logitech_hidpp_t *hidpp,
    remapper_hidpp_output_kind_t kind,
    bool accepted);
bool remapper_logitech_hidpp_process_input(
    remapper_logitech_hidpp_t *hidpp,
    uint8_t report_id,
    const uint8_t *payload,
    size_t payload_len,
    remapper_hidpp_input_result_t *result);
bool remapper_logitech_hidpp_claims_forward(
    const remapper_logitech_hidpp_t *hidpp);

/* Pico composition glue. The implementation never exposes HID++ to app/USB. */
bool remapper_logitech_hidpp_pico_start(void);
void remapper_logitech_hidpp_pico_set_forward_fix(bool enabled);

#ifdef __cplusplus
}
#endif

#endif
