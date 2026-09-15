#include "remapper/logitech_hidpp/logitech_hidpp.h"

#include <string.h>

#include "remapper/ble_hogp/ble_hogp.h"

static remapper_logitech_hidpp_t g_hidpp;
static volatile bool g_forward_desired;
static remapper_hidpp_output_kind_t g_last_output_kind;

static bool vendor_input(
    void *context,
    remapper_hid_source_t source,
    uint8_t report_id,
    const uint8_t *payload,
    size_t payload_len,
    remapper_ble_hogp_emit_fn emit,
    void *emit_context)
{
    (void)context;
    remapper_hidpp_input_result_t result;
    if (!remapper_logitech_hidpp_process_input(
            &g_hidpp,
            report_id,
            payload,
            payload_len,
            &result)) {
        return false;
    }

    if (result.held_changed && emit != NULL) {
        remapper_canonical_mouse_event_t event = {0};
        event.source = source;
        event.type = REMAPPER_MOUSE_EVENT_BUTTON;
        event.data.button.button = REMAPPER_MOUSE_BUTTON_FORWARD;
        event.data.button.pressed = result.forward_held;
        (void)emit(emit_context, &event);
    }
    return result.consumed;
}

static bool vendor_next_output(
    void *context,
    uint8_t *report_id,
    uint8_t *payload,
    uint16_t *payload_len,
    uint16_t payload_capacity)
{
    (void)context;
    if (report_id == NULL || payload == NULL || payload_len == NULL) return false;

    remapper_logitech_hidpp_set_forward_desired(
        &g_hidpp,
        g_forward_desired);

    remapper_hidpp_output_t output;
    if (!remapper_logitech_hidpp_next_output(&g_hidpp, &output) ||
        output.payload_len > payload_capacity || output.payload_len > UINT16_MAX) {
        return false;
    }

    *report_id = output.report_id;
    *payload_len = (uint16_t)output.payload_len;
    memcpy(payload, output.payload, output.payload_len);
    g_last_output_kind = output.kind;
    return true;
}

static void vendor_output_result(void *context, bool accepted)
{
    (void)context;
    remapper_logitech_hidpp_output_result(
        &g_hidpp,
        g_last_output_kind,
        accepted);
}

static bool vendor_claims_button(void *context, remapper_mouse_button_t button)
{
    (void)context;
    return button == REMAPPER_MOUSE_BUTTON_FORWARD &&
        remapper_logitech_hidpp_claims_forward(&g_hidpp);
}

static void vendor_session(void *context, bool connected)
{
    (void)context;
    if (connected) {
        remapper_logitech_hidpp_on_connect(&g_hidpp);
        remapper_logitech_hidpp_set_forward_desired(
            &g_hidpp,
            g_forward_desired);
    } else {
        remapper_logitech_hidpp_on_disconnect(&g_hidpp);
    }
    g_last_output_kind = REMAPPER_HIDPP_OUTPUT_NONE;
}

bool remapper_logitech_hidpp_pico_start(void)
{
    remapper_logitech_hidpp_init(&g_hidpp);
    g_forward_desired = false;
    g_last_output_kind = REMAPPER_HIDPP_OUTPUT_NONE;

    const remapper_ble_hogp_vendor_backend_t backend = {
        .context = NULL,
        .input = vendor_input,
        .next_output = vendor_next_output,
        .output_result = vendor_output_result,
        .claims_button = vendor_claims_button,
        .session = vendor_session,
    };
    return remapper_ble_hogp_register_vendor_backend(&backend);
}

void remapper_logitech_hidpp_pico_set_forward_fix(bool enabled)
{
    g_forward_desired = enabled;
}
