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
#include "remapper/logitech_hidpp/logitech_hidpp.h"
#include "remapper/profiles/profiles.h"
#include "remapper/remap/remap.h"
#include "remapper/renderer/renderer.h"
#include "remapper/renderer/st7789_pico.h"
#include "remapper/storage_pico/storage_pico.h"
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
        remapper_usb_hid_pico_debug_printf("[BT] session setup\r\n"); break;
    case REMAPPER_BLE_HOGP_DEBUG_STACK_READY:
        remapper_usb_hid_pico_debug_printf("[BT] stack ready\r\n"); break;
    case REMAPPER_BLE_HOGP_DEBUG_SCAN_STARTED:
        remapper_usb_hid_pico_debug_printf("[BLE] scan started\r\n"); break;
    case REMAPPER_BLE_HOGP_DEBUG_HID_ADVERTISEMENT:
        remapper_usb_hid_pico_debug_printf(
            "[BLE] HID advertisement type=%u rssi=%ld appearance=%ld\r\n",
            record->status, (long)record->a, (long)record->b); break;
    case REMAPPER_BLE_HOGP_DEBUG_CONNECTING:
        remapper_usb_hid_pico_debug_printf(
            "[BLE] connecting address_type=%u appearance=%ld\r\n",
            record->status, (long)record->a); break;
    case REMAPPER_BLE_HOGP_DEBUG_LE_CONNECTED:
        remapper_usb_hid_pico_debug_printf(
            "[BLE] LE connected status=%u handle=%ld\r\n",
            record->status, (long)record->a); break;
    case REMAPPER_BLE_HOGP_DEBUG_PAIRING_STARTED:
        remapper_usb_hid_pico_debug_printf(
            "[SM] pairing started handle=%ld\r\n", (long)record->a); break;
    case REMAPPER_BLE_HOGP_DEBUG_PAIRING_COMPLETE:
        remapper_usb_hid_pico_debug_printf(
            "[SM] security complete status=%u handle=%ld reencrypt=%ld\r\n",
            record->status, (long)record->a, (long)record->b); break;
    case REMAPPER_BLE_HOGP_DEBUG_HIDS_CONNECTING:
        remapper_usb_hid_pico_debug_printf(
            "[HOGP] connecting HIDS handle=%ld\r\n", (long)record->a); break;
    case REMAPPER_BLE_HOGP_DEBUG_HIDS_CONNECTED:
        remapper_usb_hid_pico_debug_printf(
            "[HOGP] service connected status=%u cid=%ld\r\n",
            record->status, (long)record->a); break;
    case REMAPPER_BLE_HOGP_DEBUG_REPORT_MAP:
        remapper_usb_hid_pico_debug_printf(
            "[HOGP] report map len=%ld\r\n", (long)record->a); break;
    case REMAPPER_BLE_HOGP_DEBUG_PARSER_READY:
        remapper_usb_hid_pico_debug_printf(
            "[HOGP] parser ready fields=%ld reports=%ld\r\n",
            (long)record->a, (long)record->b); break;
    case REMAPPER_BLE_HOGP_DEBUG_REPORT_RX:
        remapper_usb_hid_pico_debug_printf(
            "[HOGP] report id=%u len=%ld\r\n",
            record->report_id, (long)record->a); break;
    case REMAPPER_BLE_HOGP_DEBUG_DISCONNECTED:
        remapper_usb_hid_pico_debug_printf(
            "[BLE] disconnected reason=%u handle=%ld\r\n",
            record->status, (long)record->a); break;
    case REMAPPER_BLE_HOGP_DEBUG_RESCAN:
        remapper_usb_hid_pico_debug_printf(
            "[BLE] disconnect/rescan state=%ld\r\n", (long)record->a); break;
    case REMAPPER_BLE_HOGP_DEBUG_ERROR:
        remapper_usb_hid_pico_debug_printf(
            "[ERR] BLE stage=%ld status=%u report=%u a=%ld b=%ld\r\n",
            (long)record->a, record->status, record->report_id,
            (long)record->b, (long)record->c); break;
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
        &report, output.mouse_buttons, dx, dy, wheel, pan);
    if (!remapper_usb_hid_pico_send_mouse(&report)) return;

    (void)remapper_hid_aggregator_consume_relative(
        aggregator, dx, dy, wheel, pan);
    *last_buttons = output.mouse_buttons;
    *last_buttons_valid = true;
}

static void build_keyboard_report(
    const remapper_hid_output_state_t *output,
    remapper_usb_keyboard_report_t *report)
{
    uint8_t keys[REMAPPER_USB_HID_KEYCODE_COUNT] = {0};
    size_t out = 0u;
    for (unsigned int key = 0u;
         key < REMAPPER_HID_KEY_COUNT && out < REMAPPER_USB_HID_KEYCODE_COUNT;
         ++key) {
        if ((output->key_bitmap[key >> 3u] & (uint8_t)(1u << (key & 7u))) != 0u) {
            keys[out++] = (uint8_t)key;
        }
    }
    remapper_usb_hid_build_keyboard_report(report, output->modifiers, keys);
}

static void service_usb_keyboard(
    remapper_hid_aggregator_t *aggregator,
    remapper_usb_keyboard_report_t *last_report,
    bool *last_valid)
{
    remapper_hid_output_state_t output;
    remapper_hid_aggregator_snapshot(aggregator, &output);
    remapper_usb_keyboard_report_t report;
    build_keyboard_report(&output, &report);
    if (*last_valid && memcmp(last_report, &report, sizeof(report)) == 0) return;
    if (!remapper_usb_hid_pico_send_keyboard(&report)) return;
    *last_report = report;
    *last_valid = true;
}

static uint32_t profile_storage_key(const remapper_ble_hogp_peer_t *peer)
{
    uint32_t value = UINT32_C(2166136261);
    const uint8_t prefix[] = {'G', '0', '7'};
    for (size_t i = 0u; i < sizeof(prefix); ++i) {
        value = (value ^ prefix[i]) * UINT32_C(16777619);
    }
    value = (value ^ peer->address_type) * UINT32_C(16777619);
    for (size_t i = 0u; i < sizeof(peer->address); ++i) {
        value = (value ^ peer->address[i]) * UINT32_C(16777619);
    }
    return value == 0u ? 1u : value;
}

static void apply_runtime_profile(
    remapper_profiles_t *profiles,
    remapper_remap_t *remap,
    remapper_hid_aggregator_t *aggregator,
    const remapper_mouse_profile_config_t *config,
    bool *mouse_valid,
    bool *keyboard_valid)
{
    const remapper_hid_source_t ble_source =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1u);
    const remapper_hid_source_t synthetic_source =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_SYNTHETIC_REMAP, 1u);
    (void)remapper_hid_aggregator_release_source(aggregator, ble_source);
    (void)remapper_hid_aggregator_release_source(aggregator, synthetic_source);

    remapper_profiles_activate(profiles, config);
    remapper_mouse_profile_config_t active;
    remapper_profiles_configure_active(profiles, &active);
    remapper_remap_set_profile(remap, &active);
    remapper_logitech_hidpp_pico_set_forward_fix(
        remapper_profiles_requires_forward_held_fix(&active));
    *mouse_valid = false;
    *keyboard_valid = false;
}

static bool persist_and_apply_profile(
    remapper_profiles_t *profiles,
    remapper_remap_t *remap,
    remapper_hid_aggregator_t *aggregator,
    const remapper_mouse_profile_config_t *config,
    uint32_t storage_key,
    bool mouse_connected,
    bool *mouse_valid,
    bool *keyboard_valid)
{
    if (!mouse_connected || storage_key == 0u) return false;
    remapper_profiles_t candidate = *profiles;
    remapper_profiles_activate(&candidate, config);
    uint8_t serialized[REMAPPER_PROFILE_SERIALIZED_SIZE];
    if (!remapper_profiles_serialize(&candidate, serialized) ||
        !remapper_storage_pico_write(storage_key, serialized, sizeof(serialized))) {
        return false;
    }
    apply_runtime_profile(
        profiles, remap, aggregator, config, mouse_valid, keyboard_valid);
    return true;
}

static bool map_screen_source(
    remapper_ux_screen_t screen,
    remapper_mouse_source_button_t *source)
{
    if (source == NULL) return false;
    switch (screen) {
    case REMAPPER_UX_SCREEN_MAP_LEFT: *source = REMAPPER_MOUSE_SOURCE_LEFT; return true;
    case REMAPPER_UX_SCREEN_MAP_RIGHT: *source = REMAPPER_MOUSE_SOURCE_RIGHT; return true;
    case REMAPPER_UX_SCREEN_MAP_MIDDLE: *source = REMAPPER_MOUSE_SOURCE_MIDDLE; return true;
    case REMAPPER_UX_SCREEN_MAP_FORWARD: *source = REMAPPER_MOUSE_SOURCE_FORWARD; return true;
    case REMAPPER_UX_SCREEN_MAP_BACKWARD: *source = REMAPPER_MOUSE_SOURCE_BACKWARD; return true;
    default: return false;
    }
}

static void configure_dynamic_screen(
    remapper_interaction_state_t *interaction,
    const remapper_profiles_t *profiles)
{
    remapper_mouse_source_button_t source;
    if (!map_screen_source(interaction->screen, &source)) return;
    static const uint32_t targets[] = {
        REMAPPER_MOUSE_TARGET_LEFT,
        REMAPPER_MOUSE_TARGET_RIGHT,
        REMAPPER_MOUSE_TARGET_MIDDLE,
        REMAPPER_MOUSE_TARGET_FORWARD,
        REMAPPER_MOUSE_TARGET_BACKWARD,
    };
    if (!remapper_interaction_set_dynamic_options(
            interaction, targets, sizeof(targets) / sizeof(targets[0]))) {
        return;
    }
    const remapper_mouse_target_t current = profiles->draft_valid
        ? profiles->draft_targets[source]
        : profiles->custom_targets[source];
    if (current <= REMAPPER_MOUSE_TARGET_BACKWARD) {
        interaction->selected_option = (uint8_t)current;
    }
}

static void handle_command(
    const remapper_command_t *command,
    remapper_profiles_t *profiles,
    remapper_remap_t *remap,
    remapper_hid_aggregator_t *aggregator,
    uint32_t storage_key,
    bool mouse_connected,
    bool *mouse_valid,
    bool *keyboard_valid)
{
    if (command == NULL) return;
    remapper_mouse_profile_config_t candidate;

    switch (command->kind) {
    case REMAPPER_COMMAND_APPLY_MOUSE_PROFILE:
        if (command->data.profile <= REMAPPER_MOUSE_PROFILE_ESCAPE_REMAP &&
            remapper_profiles_build_preset(command->data.profile, profiles, &candidate)) {
            (void)persist_and_apply_profile(
                profiles, remap, aggregator, &candidate,
                storage_key, mouse_connected, mouse_valid, keyboard_valid);
        }
        break;
    case REMAPPER_COMMAND_APPLY_CUSTOM_MAPPING:
        if (command->data.custom_mapping.target_token <= REMAPPER_MOUSE_TARGET_BACKWARD) {
            (void)remapper_profiles_draft_set(
                profiles,
                command->data.custom_mapping.source,
                (remapper_mouse_target_t)command->data.custom_mapping.target_token);
        }
        break;
    case REMAPPER_COMMAND_COMMIT_CUSTOM_REMAP:
        if (remapper_profiles_custom_candidate(profiles, &candidate)) {
            (void)persist_and_apply_profile(
                profiles, remap, aggregator, &candidate,
                storage_key, mouse_connected, mouse_valid, keyboard_valid);
        }
        break;
    case REMAPPER_COMMAND_NONE:
    case REMAPPER_COMMAND_START_PAIRING:
    case REMAPPER_COMMAND_SELECT_SAVED_DEVICE:
    case REMAPPER_COMMAND_REMOVE_SELECTED_DEVICE:
        break;
    }
}

int main(void)
{
    remapper_interaction_state_t interaction;
    remapper_display_hal_t display;
    remapper_hid_aggregator_t hid_aggregator;
    remapper_profiles_t profiles;
    remapper_remap_t remap;
    const remapper_hid_source_t ble_mouse_source =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1u);
    const remapper_hid_source_t synthetic_source =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_SYNTHETIC_REMAP, 1u);
    uint8_t last_mouse_buttons = 0u;
    bool last_mouse_buttons_valid = false;
    remapper_usb_keyboard_report_t last_keyboard_report = {0};
    bool last_keyboard_valid = false;
    bool usb_was_mounted = false;
    bool mouse_connected = false;
    uint32_t active_profile_storage_key = 0u;

    if (!remapper_usb_hid_pico_init()) {
        while (true) tight_loop_contents();
    }

    remapper_interaction_init(&interaction);
    remapper_hid_aggregator_init(&hid_aggregator);
    remapper_profiles_init(&profiles);
    remapper_remap_init(&remap);
    remapper_hat_pico_init();
    if (!remapper_st7789_pico_init(&display)) {
        while (true) {
            remapper_usb_hid_pico_task();
            tight_loop_contents();
        }
    }

    (void)render_state(&display, &interaction);
    remapper_st7789_pico_set_backlight(true);

    (void)remapper_logitech_hidpp_pico_start();
    (void)remapper_ble_hogp_start();

    while (true) {
        remapper_usb_hid_pico_task();
        remapper_hat_pico_task();

        const bool usb_mounted = remapper_usb_hid_pico_mounted();
        if (usb_mounted != usb_was_mounted) {
            last_mouse_buttons_valid = false;
            last_keyboard_valid = false;
            usb_was_mounted = usb_mounted;
        }

        if (remapper_bt_runtime_take_overflow()) {
            (void)remapper_hid_aggregator_release_source(
                &hid_aggregator, ble_mouse_source);
            (void)remapper_hid_aggregator_release_source(
                &hid_aggregator, synthetic_source);
            last_mouse_buttons_valid = false;
            last_keyboard_valid = false;
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
                    &runtime_message, &ble_event)) {
                continue;
            }

            switch (ble_event.type) {
            case REMAPPER_BLE_HOGP_EVENT_CONNECTED: {
                mouse_connected = true;
                active_profile_storage_key = profile_storage_key(&ble_event.peer);
                remapper_profiles_init(&profiles);
                uint8_t serialized[REMAPPER_PROFILE_SERIALIZED_SIZE];
                if (remapper_storage_pico_read(
                        active_profile_storage_key,
                        serialized,
                        sizeof(serialized))) {
                    (void)remapper_profiles_restore(&profiles, serialized);
                }
                remapper_mouse_profile_config_t active;
                remapper_profiles_configure_active(&profiles, &active);
                remapper_remap_set_profile(&remap, &active);
                remapper_logitech_hidpp_pico_set_forward_fix(
                    remapper_profiles_requires_forward_held_fix(&active));
                last_mouse_buttons_valid = false;
                last_keyboard_valid = false;
                break;
            }
            case REMAPPER_BLE_HOGP_EVENT_DISCONNECTED:
                mouse_connected = false;
                active_profile_storage_key = 0u;
                remapper_logitech_hidpp_pico_set_forward_fix(false);
                (void)remapper_hid_aggregator_release_source(
                    &hid_aggregator, ble_mouse_source);
                (void)remapper_hid_aggregator_release_source(
                    &hid_aggregator, synthetic_source);
                last_mouse_buttons_valid = false;
                last_keyboard_valid = false;
                break;
            case REMAPPER_BLE_HOGP_EVENT_MOUSE: {
                remapper_remap_result_t mapped;
                if (!remapper_remap_process_mouse(&remap, &ble_event.mouse, &mapped)) {
                    break;
                }
                if (mapped.has_mouse) {
                    (void)remapper_hid_aggregator_apply_mouse(
                        &hid_aggregator, &mapped.mouse);
                }
                if (mapped.has_keyboard) {
                    (void)remapper_hid_aggregator_apply_keyboard(
                        &hid_aggregator, &mapped.keyboard);
                }
                break;
            }
            }
        }

        if (usb_mounted) {
            service_usb_mouse(
                &hid_aggregator,
                &last_mouse_buttons,
                &last_mouse_buttons_valid);
            service_usb_keyboard(
                &hid_aggregator,
                &last_keyboard_report,
                &last_keyboard_valid);
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

            if (result.screen_changed) {
                configure_dynamic_screen(&interaction, &profiles);
            }
            handle_command(
                &result.command,
                &profiles,
                &remap,
                &hid_aggregator,
                active_profile_storage_key,
                mouse_connected,
                &last_mouse_buttons_valid,
                &last_keyboard_valid);

            if (result.lock_changed && interaction.locked) {
                remapper_st7789_pico_set_backlight(false);
            }
            if (result.visual_changed && !interaction.locked) {
                (void)render_state(&display, &interaction);
            }
            if (result.lock_changed && !interaction.locked) {
                remapper_st7789_pico_set_backlight(true);
            }
        }
        tight_loop_contents();
    }
}
