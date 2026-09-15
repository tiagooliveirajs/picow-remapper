#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "remapper/app/ui_projection.h"
#include "remapper/ble_hogp/ble_hogp.h"
#include "remapper/bt_runtime/bt_runtime.h"
#include "remapper/hat/hat.h"
#include "remapper/hid_aggregator/hid_aggregator.h"
#include "remapper/interaction/interaction.h"
#include "remapper/renderer/renderer.h"
#include "remapper/renderer/st7789_pico.h"
#include "remapper/usb_hid/usb_hid.h"

#define REMAPPER_RUNTIME_MESSAGES_PER_TICK 32u

static remapper_control_t interaction_control(remapper_hat_control_t control)
{
    switch (control) {
    case REMAPPER_HAT_JOY_UP: return REMAPPER_CONTROL_JOY_UP;
    case REMAPPER_HAT_JOY_DOWN: return REMAPPER_CONTROL_JOY_DOWN;
    case REMAPPER_HAT_JOY_LEFT: return REMAPPER_CONTROL_JOY_LEFT;
    case REMAPPER_HAT_JOY_RIGHT: return REMAPPER_CONTROL_JOY_RIGHT;
    case REMAPPER_HAT_JOY_PRESS: return REMAPPER_CONTROL_JOY_PRESS;
    case REMAPPER_HAT_KEY_A: return REMAPPER_CONTROL_KEY_A;
    case REMAPPER_HAT_KEY_B: return REMAPPER_CONTROL_KEY_B;
    case REMAPPER_HAT_KEY_X: return REMAPPER_CONTROL_KEY_X;
    case REMAPPER_HAT_KEY_Y: return REMAPPER_CONTROL_KEY_Y;
    case REMAPPER_HAT_CONTROL_COUNT: return REMAPPER_CONTROL_COUNT;
    }
    return REMAPPER_CONTROL_COUNT;
}

static bool render_state(
    const remapper_display_hal_t *display,
    const remapper_interaction_state_t *state)
{
    remapper_ui_frame_t frame;
    remapper_ui_project(state, &frame);
    return remapper_renderer_render(display, &frame);
}

static int8_t clamp_i8(int32_t value)
{
    if (value > INT8_MAX) return INT8_MAX;
    if (value < INT8_MIN) return INT8_MIN;
    return (int8_t)value;
}

static void log_ble_debug_record(const remapper_ble_hogp_debug_record_t *record)
{
    if (record == NULL) return;
    switch ((remapper_ble_hogp_debug_code_t)record->code) {
    case REMAPPER_BLE_HOGP_DEBUG_SESSION_SETUP:
        remapper_usb_hid_pico_debug_printf("[BT] session setup\r\n");
        break;
    case REMAPPER_BLE_HOGP_DEBUG_STACK_READY:
        remapper_usb_hid_pico_debug_printf("[BT] stack ready\r\n");
        break;
    case REMAPPER_BLE_HOGP_DEBUG_SCAN_STARTED:
        remapper_usb_hid_pico_debug_printf("[BLE] scan started\r\n");
        break;
    case REMAPPER_BLE_HOGP_DEBUG_HID_ADVERTISEMENT:
        remapper_usb_hid_pico_debug_printf(
            "[BLE] HID advertisement type=%u rssi=%ld appearance=%ld\r\n",
            record->status,
            (long)record->a,
            (long)record->b);
        break;
    case REMAPPER_BLE_HOGP_DEBUG_CONNECTING:
        remapper_usb_hid_pico_debug_printf(
            "[BLE] connecting address_type=%u appearance=%ld\r\n",
            record->status,
            (long)record->a);
        break;
    case REMAPPER_BLE_HOGP_DEBUG_LE_CONNECTED:
        remapper_usb_hid_pico_debug_printf(
            "[BLE] LE connected status=%u handle=%ld\r\n",
            record->status,
            (long)record->a);
        break;
    case REMAPPER_BLE_HOGP_DEBUG_PAIRING_STARTED:
        remapper_usb_hid_pico_debug_printf("[SM] pairing started handle=%ld\r\n", (long)record->a);
        break;
    case REMAPPER_BLE_HOGP_DEBUG_PAIRING_COMPLETE:
        remapper_usb_hid_pico_debug_printf(
            "[SM] security complete status=%u handle=%ld reencrypt=%ld\r\n",
            record->status,
            (long)record->a,
            (long)record->b);
        break;
    case REMAPPER_BLE_HOGP_DEBUG_HIDS_CONNECTING:
        remapper_usb_hid_pico_debug_printf("[HOGP] connecting HIDS handle=%ld\r\n", (long)record->a);
        break;
    case REMAPPER_BLE_HOGP_DEBUG_HIDS_CONNECTED:
        remapper_usb_hid_pico_debug_printf(
            "[HOGP] service connected status=%u cid=%ld\r\n",
            record->status,
            (long)record->a);
        break;
    case REMAPPER_BLE_HOGP_DEBUG_REPORT_MAP:
        remapper_usb_hid_pico_debug_printf("[HOGP] report map len=%ld\r\n", (long)record->a);
        break;
    case REMAPPER_BLE_HOGP_DEBUG_PARSER_READY:
        remapper_usb_hid_pico_debug_printf(
            "[HOGP] parser ready fields=%ld reports=%ld\r\n",
            (long)record->a,
            (long)record->b);
        break;
    case REMAPPER_BLE_HOGP_DEBUG_REPORT_RX:
        remapper_usb_hid_pico_debug_printf(
            "[HOGP] report id=%u len=%ld\r\n",
            record->report_id,
            (long)record->a);
        break;
    case REMAPPER_BLE_HOGP_DEBUG_DISCONNECTED:
        remapper_usb_hid_pico_debug_printf(
            "[BLE] disconnected reason=%u handle=%ld\r\n",
            record->status,
            (long)record->a);
        break;
    case REMAPPER_BLE_HOGP_DEBUG_RESCAN:
        remapper_usb_hid_pico_debug_printf("[BLE] disconnect/rescan state=%ld\r\n", (long)record->a);
        break;
    case REMAPPER_BLE_HOGP_DEBUG_ERROR:
        remapper_usb_hid_pico_debug_printf(
            "[ERR] BLE stage=%ld status=%u report=%u a=%ld b=%ld\r\n",
            (long)record->a,
            record->status,
            record->report_id,
            (long)record->b,
            (long)record->c);
        break;
    }
}

static void log_mouse_event(const remapper_canonical_mouse_event_t *event)
{
    if (event == NULL) return;
    switch (event->type) {
    case REMAPPER_MOUSE_EVENT_BUTTON:
        remapper_usb_hid_pico_debug_printf(
            "[HID] button=%u pressed=%u\r\n",
            (unsigned)event->data.button.button,
            event->data.button.pressed ? 1u : 0u);
        break;
    case REMAPPER_MOUSE_EVENT_MOVE:
        remapper_usb_hid_pico_debug_printf(
            "[HID] move dx=%d dy=%d\r\n",
            event->data.move.dx,
            event->data.move.dy);
        break;
    case REMAPPER_MOUSE_EVENT_WHEEL:
        remapper_usb_hid_pico_debug_printf(
            "[HID] wheel v=%d h=%d\r\n",
            event->data.wheel.vertical,
            event->data.wheel.horizontal);
        break;
    }
}

static void service_usb_mouse(
    remapper_hid_aggregator_t *aggregator,
    uint8_t *last_buttons,
    bool *last_buttons_valid)
{
    remapper_hid_output_state_t output;
    remapper_hid_aggregator_snapshot(aggregator, &output);

    const int8_t dx = clamp_i8(output.dx);
    const int8_t dy = clamp_i8(output.dy);
    const int8_t wheel = clamp_i8(output.wheel_vertical);
    const int8_t pan = clamp_i8(output.wheel_horizontal);
    const bool has_relative = dx != 0 || dy != 0 || wheel != 0 || pan != 0;
    const bool buttons_changed =
        !*last_buttons_valid || output.mouse_buttons != *last_buttons;
    if (!has_relative && !buttons_changed) return;

    remapper_usb_mouse_report_t report;
    remapper_usb_hid_build_mouse_report(
        &report,
        output.mouse_buttons,
        dx,
        dy,
        wheel,
        pan);

    if (!remapper_usb_hid_pico_send_mouse(&report)) return;

    remapper_usb_hid_pico_debug_printf(
        "[USB] mouse buttons=0x%02x dx=%d dy=%d wheel=%d pan=%d\r\n",
        output.mouse_buttons,
        dx,
        dy,
        wheel,
        pan);

    (void)remapper_hid_aggregator_consume_relative(
        aggregator,
        dx,
        dy,
        wheel,
        pan);
    *last_buttons = output.mouse_buttons;
    *last_buttons_valid = true;
}

int main(void)
{
    remapper_interaction_state_t interaction;
    remapper_display_hal_t display;
    remapper_hid_aggregator_t hid_aggregator;
    const remapper_hid_source_t ble_mouse_source =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1u);
    uint8_t last_mouse_buttons = 0u;
    bool last_mouse_buttons_valid = false;
    bool usb_was_mounted = false;

    if (!remapper_usb_hid_pico_init()) {
        while (true) tight_loop_contents();
    }
    remapper_usb_hid_pico_debug_printf(
        "[BOOT] %s %s usb_debug_cdc=%u\r\n",
        REMAPPER_GATE_NAME,
        REMAPPER_FIRMWARE_VERSION,
#if defined(REMAPPER_USB_DEBUG_CDC) && REMAPPER_USB_DEBUG_CDC
        1u
#else
        0u
#endif
    );

    remapper_interaction_init(&interaction);
    remapper_hid_aggregator_init(&hid_aggregator);
    remapper_hat_pico_init();
    if (!remapper_st7789_pico_init(&display)) {
        while (true) {
            remapper_usb_hid_pico_task();
            tight_loop_contents();
        }
    }

    (void)render_state(&display, &interaction);
    remapper_st7789_pico_set_backlight(true);

    (void)remapper_ble_hogp_start();

    while (true) {
        /* USB service is non-blocking; HAT scan is always serviced every tick. */
        remapper_usb_hid_pico_task();
        remapper_hat_pico_task();

        const bool usb_mounted = remapper_usb_hid_pico_mounted();
        if (usb_mounted != usb_was_mounted) {
            remapper_usb_hid_pico_debug_printf("[USB] mounted=%u\r\n", usb_mounted ? 1u : 0u);
            last_mouse_buttons_valid = false;
            usb_was_mounted = usb_mounted;
        }

        if (remapper_bt_runtime_take_overflow()) {
            remapper_usb_hid_pico_debug_printf("[QUEUE] overflow: releasing BLE mouse source\r\n");
            (void)remapper_hid_aggregator_release_source(
                &hid_aggregator,
                ble_mouse_source);
            last_mouse_buttons_valid = false;
        }

        remapper_bt_runtime_message_t runtime_message;
        for (size_t runtime_count = 0u;
             runtime_count < REMAPPER_RUNTIME_MESSAGES_PER_TICK &&
                 remapper_bt_runtime_poll(&runtime_message);
             ++runtime_count) {
            if (runtime_message.channel == REMAPPER_BLE_HOGP_RUNTIME_CHANNEL &&
                runtime_message.type == REMAPPER_BLE_HOGP_MESSAGE_DEBUG &&
                runtime_message.length == sizeof(remapper_ble_hogp_debug_record_t)) {
                remapper_ble_hogp_debug_record_t record;
                memcpy(&record, runtime_message.payload, sizeof(record));
                log_ble_debug_record(&record);
                continue;
            }

            remapper_ble_hogp_event_t ble_event;
            if (!remapper_ble_hogp_decode_runtime_message(
                    &runtime_message,
                    &ble_event)) {
                continue;
            }

            switch (ble_event.type) {
            case REMAPPER_BLE_HOGP_EVENT_CONNECTED:
                remapper_usb_hid_pico_debug_printf("[HOGP] canonical mouse connected\r\n");
                last_mouse_buttons_valid = false;
                break;
            case REMAPPER_BLE_HOGP_EVENT_DISCONNECTED:
                remapper_usb_hid_pico_debug_printf("[HOGP] canonical mouse disconnected\r\n");
                (void)remapper_hid_aggregator_release_source(
                    &hid_aggregator,
                    ble_mouse_source);
                last_mouse_buttons_valid = false;
                break;
            case REMAPPER_BLE_HOGP_EVENT_MOUSE:
                log_mouse_event(&ble_event.mouse);
                (void)remapper_hid_aggregator_apply_mouse(
                    &hid_aggregator,
                    &ble_event.mouse);
                break;
            }
        }

        if (usb_mounted) {
            service_usb_mouse(
                &hid_aggregator,
                &last_mouse_buttons,
                &last_mouse_buttons_valid);
        }

        remapper_hat_event_t hat_event;
        while (remapper_hat_pico_poll_event(&hat_event)) {
            const remapper_control_t control = interaction_control(hat_event.control);
            if (control == REMAPPER_CONTROL_COUNT) continue;

            const remapper_input_event_t event = {
                .control = control,
                .phase = hat_event.pressed ? REMAPPER_INPUT_PRESS : REMAPPER_INPUT_RELEASE,
            };
            const remapper_interaction_result_t result =
                remapper_interaction_handle_event(&interaction, event);

            if (result.lock_changed && interaction.locked) {
                remapper_st7789_pico_set_backlight(false);
            }
            if (result.visual_changed && !interaction.locked) {
                (void)render_state(&display, &interaction);
            }
            if (result.lock_changed && !interaction.locked) {
                remapper_st7789_pico_set_backlight(true);
            }

            (void)result.command;
        }
        tight_loop_contents();
    }
}
