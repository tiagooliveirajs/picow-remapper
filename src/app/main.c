#include <stdbool.h>

#include "pico/stdlib.h"
#include "remapper/app/ui_projection.h"
#include "remapper/hat/hat.h"
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

int main(void)
{
    remapper_interaction_state_t interaction;
    remapper_display_hal_t display;

    if (!remapper_usb_hid_pico_init()) {
        while (true) tight_loop_contents();
    }

    remapper_interaction_init(&interaction);
    remapper_hat_pico_init();
    if (!remapper_st7789_pico_init(&display)) {
        while (true) {
            remapper_usb_hid_pico_task();
            tight_loop_contents();
        }
    }

    (void)render_state(&display, &interaction);
    remapper_st7789_pico_set_backlight(true);

    while (true) {
        remapper_usb_hid_pico_task();
        remapper_hat_pico_task();

        remapper_hat_event_t hat_event;
        while (remapper_hat_pico_poll_event(&hat_event)) {
            const remapper_control_t control = interaction_control(hat_event.control);
            if (control == REMAPPER_CONTROL_COUNT) continue;

            const remapper_input_event_t event = {
                .control = control,
                .phase = hat_event.pressed ? REMAPPER_INPUT_PRESS : REMAPPER_INPUT_RELEASE,
            };
            const remapper_interaction_result_t result = remapper_interaction_handle_event(&interaction, event);

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
