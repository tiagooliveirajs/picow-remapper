#include <stdbool.h>
#include <stdint.h>

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
        remapper_usb_hid_pico_task();

        const bool usb_mounted = remapper_usb_hid_pico_mounted();
        if (usb_mounted != usb_was_mounted) {
            last_mouse_buttons_valid = false;
            usb_was_mounted = usb_mounted;
        }

        if (remapper_bt_runtime_take_overflow()) {
            (void)remapper_hid_aggregator_release_source(
                &hid_aggregator,
                ble_mouse_source);
            last_mouse_buttons_valid = false;
        }

        remapper_bt_runtime_message_t runtime_message;
        while (remapper_bt_runtime_poll(&runtime_message)) {
            remapper_ble_hogp_event_t ble_event;
            if (!remapper_ble_hogp_decode_runtime_message(
                    &runtime_message,
                    &ble_event)) {
                continue;
            }

            switch (ble_event.type) {
            case REMAPPER_BLE_HOGP_EVENT_CONNECTED:
                last_mouse_buttons_valid = false;
                break;
            case REMAPPER_BLE_HOGP_EVENT_DISCONNECTED:
                (void)remapper_hid_aggregator_release_source(
                    &hid_aggregator,
                    ble_mouse_source);
                last_mouse_buttons_valid = false;
                break;
            case REMAPPER_BLE_HOGP_EVENT_MOUSE:
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

        remapper_hat_pico_task();
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
