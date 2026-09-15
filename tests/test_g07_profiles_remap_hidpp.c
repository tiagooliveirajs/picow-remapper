#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "remapper/hid_aggregator/hid_aggregator.h"
#include "remapper/logitech_hidpp/logitech_hidpp.h"
#include "remapper/profiles/profiles.h"
#include "remapper/remap/remap.h"

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
    exit(1); } } while (0)

static void test_profile_presets_and_transaction(void)
{
    remapper_profiles_t profiles;
    remapper_profiles_init(&profiles);
    CHECK(profiles.active_kind == REMAPPER_MOUSE_PROFILE_PASSTHROUGH);

    remapper_mouse_profile_config_t config;
    remapper_profiles_configure_active(&profiles, &config);
    CHECK(config.targets[REMAPPER_MOUSE_SOURCE_LEFT] == REMAPPER_MOUSE_TARGET_LEFT);
    CHECK(config.targets[REMAPPER_MOUSE_SOURCE_FORWARD] == REMAPPER_MOUSE_TARGET_FORWARD);
    CHECK(!remapper_profiles_requires_forward_held_fix(&config));

    CHECK(remapper_profiles_build_preset(
        REMAPPER_MOUSE_PROFILE_DEFAULT_REMAP, &profiles, &config));
    CHECK(config.targets[REMAPPER_MOUSE_SOURCE_LEFT] == REMAPPER_MOUSE_TARGET_ESCAPE);
    CHECK(config.targets[REMAPPER_MOUSE_SOURCE_RIGHT] == REMAPPER_MOUSE_TARGET_BACKWARD);
    CHECK(config.targets[REMAPPER_MOUSE_SOURCE_MIDDLE] == REMAPPER_MOUSE_TARGET_FORWARD);
    CHECK(config.targets[REMAPPER_MOUSE_SOURCE_FORWARD] == REMAPPER_MOUSE_TARGET_LEFT);
    CHECK(config.targets[REMAPPER_MOUSE_SOURCE_BACKWARD] == REMAPPER_MOUSE_TARGET_RIGHT);
    CHECK(remapper_profiles_requires_forward_held_fix(&config));

    CHECK(remapper_profiles_build_preset(
        REMAPPER_MOUSE_PROFILE_ESCAPE_REMAP, &profiles, &config));
    CHECK(config.targets[REMAPPER_MOUSE_SOURCE_LEFT] == REMAPPER_MOUSE_TARGET_ESCAPE);
    CHECK(config.targets[REMAPPER_MOUSE_SOURCE_RIGHT] == REMAPPER_MOUSE_TARGET_RIGHT);
    CHECK(config.targets[REMAPPER_MOUSE_SOURCE_FORWARD] == REMAPPER_MOUSE_TARGET_FORWARD);
    CHECK(!remapper_profiles_requires_forward_held_fix(&config));

    CHECK(remapper_profiles_draft_set(
        &profiles, REMAPPER_MOUSE_SOURCE_FORWARD, REMAPPER_MOUSE_TARGET_RIGHT));
    CHECK(profiles.active_kind == REMAPPER_MOUSE_PROFILE_PASSTHROUGH);
    remapper_profiles_configure_active(&profiles, &config);
    CHECK(config.targets[REMAPPER_MOUSE_SOURCE_FORWARD] == REMAPPER_MOUSE_TARGET_FORWARD);

    remapper_mouse_profile_config_t custom;
    CHECK(remapper_profiles_custom_candidate(&profiles, &custom));
    CHECK(custom.kind == REMAPPER_MOUSE_PROFILE_CUSTOM_REMAP);
    CHECK(custom.targets[REMAPPER_MOUSE_SOURCE_FORWARD] == REMAPPER_MOUSE_TARGET_RIGHT);
    remapper_profiles_activate(&profiles, &custom);
    CHECK(profiles.active_kind == REMAPPER_MOUSE_PROFILE_CUSTOM_REMAP);

    uint8_t serialized[REMAPPER_PROFILE_SERIALIZED_SIZE];
    CHECK(remapper_profiles_serialize(&profiles, serialized));
    remapper_profiles_t restored;
    remapper_profiles_init(&restored);
    CHECK(remapper_profiles_restore(&restored, serialized));
    remapper_profiles_configure_active(&restored, &config);
    CHECK(config.kind == REMAPPER_MOUSE_PROFILE_CUSTOM_REMAP);
    CHECK(config.targets[REMAPPER_MOUSE_SOURCE_FORWARD] == REMAPPER_MOUSE_TARGET_RIGHT);

    serialized[7] ^= 1u;
    CHECK(!remapper_profiles_restore(&restored, serialized));
}

static remapper_canonical_mouse_event_t button_event(
    remapper_mouse_button_t button, bool pressed)
{
    remapper_canonical_mouse_event_t event = {0};
    event.source = remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1u);
    event.type = REMAPPER_MOUSE_EVENT_BUTTON;
    event.data.button.button = button;
    event.data.button.pressed = pressed;
    return event;
}

static void test_remap_and_ownership(void)
{
    remapper_profiles_t profiles;
    remapper_profiles_init(&profiles);
    remapper_mouse_profile_config_t config;
    CHECK(remapper_profiles_build_preset(
        REMAPPER_MOUSE_PROFILE_DEFAULT_REMAP, &profiles, &config));

    remapper_remap_t remap;
    remapper_remap_init(&remap);
    remapper_remap_set_profile(&remap, &config);
    remapper_hid_aggregator_t aggregator;
    remapper_hid_aggregator_init(&aggregator);

    remapper_remap_result_t result;
    remapper_canonical_mouse_event_t input =
        button_event(REMAPPER_MOUSE_BUTTON_LEFT, true);
    CHECK(remapper_remap_process_mouse(&remap, &input, &result));
    CHECK(!result.has_mouse && result.has_keyboard);
    CHECK(result.keyboard.data.key.key == REMAPPER_KEY_ESCAPE);
    CHECK(result.keyboard.data.key.pressed);
    CHECK(remapper_hid_aggregator_apply_keyboard(&aggregator, &result.keyboard));

    remapper_hid_output_state_t output;
    remapper_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(remapper_hid_output_key_is_down(&output, REMAPPER_KEY_ESCAPE));

    input = button_event(REMAPPER_MOUSE_BUTTON_FORWARD, true);
    CHECK(remapper_remap_process_mouse(&remap, &input, &result));
    CHECK(result.has_mouse && !result.has_keyboard);
    CHECK(result.mouse.data.button.button == REMAPPER_MOUSE_BUTTON_LEFT);
    CHECK(remapper_hid_aggregator_apply_mouse(&aggregator, &result.mouse));
    remapper_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(remapper_hid_output_mouse_button_is_down(&output, REMAPPER_MOUSE_BUTTON_LEFT));

    input = button_event(REMAPPER_MOUSE_BUTTON_FORWARD, false);
    CHECK(remapper_remap_process_mouse(&remap, &input, &result));
    CHECK(remapper_hid_aggregator_apply_mouse(&aggregator, &result.mouse));
    input = button_event(REMAPPER_MOUSE_BUTTON_LEFT, false);
    CHECK(remapper_remap_process_mouse(&remap, &input, &result));
    CHECK(remapper_hid_aggregator_apply_keyboard(&aggregator, &result.keyboard));
    remapper_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(!remapper_hid_output_mouse_button_is_down(&output, REMAPPER_MOUSE_BUTTON_LEFT));
    CHECK(!remapper_hid_output_key_is_down(&output, REMAPPER_KEY_ESCAPE));
}

static void test_hidpp_forward_diversion(void)
{
    remapper_logitech_hidpp_t hidpp;
    remapper_logitech_hidpp_init(&hidpp);
    remapper_logitech_hidpp_set_forward_desired(&hidpp, true);
    remapper_logitech_hidpp_on_connect(&hidpp);

    remapper_hidpp_output_t output;
    CHECK(remapper_logitech_hidpp_next_output(&hidpp, &output));
    CHECK(output.kind == REMAPPER_HIDPP_OUTPUT_GET_FEATURE);
    CHECK(output.report_id == REMAPPER_HIDPP_REPORT_ID_LONG);
    CHECK(output.payload[3] == 0x1bu && output.payload[4] == 0x04u);
    remapper_logitech_hidpp_output_result(&hidpp, output.kind, true);

    uint8_t feature_response[REMAPPER_HIDPP_LONG_PAYLOAD_SIZE] = {0};
    feature_response[0] = 0xffu;
    feature_response[1] = 0x00u;
    feature_response[2] = 0x02u;
    feature_response[3] = 0x07u;
    remapper_hidpp_input_result_t input_result;
    CHECK(remapper_logitech_hidpp_process_input(
        &hidpp, REMAPPER_HIDPP_REPORT_ID_LONG,
        feature_response, sizeof(feature_response), &input_result));
    CHECK(hidpp.feature_index == 0x07u);

    CHECK(remapper_logitech_hidpp_next_output(&hidpp, &output));
    CHECK(output.kind == REMAPPER_HIDPP_OUTPUT_SET_FORWARD_DIVERT);
    CHECK(output.payload[3] == 0x00u && output.payload[4] == 0x56u);
    CHECK(output.payload[5] == 0x03u);
    remapper_logitech_hidpp_output_result(&hidpp, output.kind, true);

    uint8_t set_response[REMAPPER_HIDPP_LONG_PAYLOAD_SIZE] = {0};
    set_response[0] = 0xffu;
    set_response[1] = 0x07u;
    set_response[2] = 0x32u;
    CHECK(remapper_logitech_hidpp_process_input(
        &hidpp, REMAPPER_HIDPP_REPORT_ID_LONG,
        set_response, sizeof(set_response), &input_result));
    CHECK(remapper_logitech_hidpp_claims_forward(&hidpp));

    uint8_t held_event[REMAPPER_HIDPP_LONG_PAYLOAD_SIZE] = {0};
    held_event[0] = 0xffu;
    held_event[1] = 0x07u;
    held_event[2] = 0x00u;
    held_event[3] = 0x00u;
    held_event[4] = 0x56u;
    CHECK(remapper_logitech_hidpp_process_input(
        &hidpp, REMAPPER_HIDPP_REPORT_ID_LONG,
        held_event, sizeof(held_event), &input_result));
    CHECK(input_result.held_changed && input_result.forward_held);

    uint8_t released_event[REMAPPER_HIDPP_LONG_PAYLOAD_SIZE] = {0};
    released_event[0] = 0xffu;
    released_event[1] = 0x07u;
    released_event[2] = 0x00u;
    CHECK(remapper_logitech_hidpp_process_input(
        &hidpp, REMAPPER_HIDPP_REPORT_ID_LONG,
        released_event, sizeof(released_event), &input_result));
    CHECK(input_result.held_changed && !input_result.forward_held);

    remapper_logitech_hidpp_set_forward_desired(&hidpp, false);
    CHECK(remapper_logitech_hidpp_next_output(&hidpp, &output));
    CHECK(output.kind == REMAPPER_HIDPP_OUTPUT_SET_FORWARD_DIVERT);
    CHECK(output.payload[5] == 0x02u);
}

int main(void)
{
    test_profile_presets_and_transaction();
    test_remap_and_ownership();
    test_hidpp_forward_diversion();
    puts("G07 profiles/remap/HID++ tests passed");
    return 0;
}
