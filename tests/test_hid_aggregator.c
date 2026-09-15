#include <stdio.h>
#include <stdlib.h>

#include "remapper/hid_aggregator.h"

#define CHECK(expr)                                                             \
    do {                                                                        \
        if (!(expr)) {                                                          \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n",                    \
                    __FILE__, __LINE__, #expr);                                 \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

static remapper_canonical_mouse_event_t mouse_button(
    remapper_hid_source_t source,
    remapper_mouse_button_t button,
    bool pressed)
{
    remapper_canonical_mouse_event_t event = {0};
    event.source = source;
    event.type = REMAPPER_MOUSE_EVENT_BUTTON;
    event.data.button.button = button;
    event.data.button.pressed = pressed;
    return event;
}

static remapper_canonical_keyboard_event_t keyboard_key(
    remapper_hid_source_t source,
    remapper_key_t key,
    bool pressed)
{
    remapper_canonical_keyboard_event_t event = {0};
    event.source = source;
    event.type = REMAPPER_KEYBOARD_EVENT_KEY;
    event.data.key.key = key;
    event.data.key.pressed = pressed;
    return event;
}

static remapper_canonical_keyboard_event_t keyboard_modifier(
    remapper_hid_source_t source,
    remapper_modifier_t modifier,
    bool pressed)
{
    remapper_canonical_keyboard_event_t event = {0};
    event.source = source;
    event.type = REMAPPER_KEYBOARD_EVENT_MODIFIER;
    event.data.modifier.modifier = modifier;
    event.data.modifier.pressed = pressed;
    return event;
}

static bool mouse_is_down(const remapper_hid_output_state_t *state,
                          remapper_mouse_button_t button)
{
    return (state->mouse_buttons & (uint16_t)(1u << (uint16_t)button)) != 0u;
}

static void test_two_sources_share_mouse_button(void)
{
    remapper_hid_aggregator_t agg;
    remapper_hid_output_state_t output;
    remapper_hid_aggregator_init(&agg);

    const remapper_hid_source_t mouse =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1);
    const remapper_hid_source_t composite =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_COMPOSITE, 1);

    remapper_canonical_mouse_event_t event =
        mouse_button(mouse, REMAPPER_MOUSE_BUTTON_LEFT, true);
    CHECK(remapper_hid_aggregator_apply_mouse(&agg, &event));
    /* Duplicate press from one source must be idempotent. */
    CHECK(remapper_hid_aggregator_apply_mouse(&agg, &event));

    event = mouse_button(composite, REMAPPER_MOUSE_BUTTON_LEFT, true);
    CHECK(remapper_hid_aggregator_apply_mouse(&agg, &event));
    remapper_hid_aggregator_snapshot(&agg, &output);
    CHECK(mouse_is_down(&output, REMAPPER_MOUSE_BUTTON_LEFT));

    event = mouse_button(mouse, REMAPPER_MOUSE_BUTTON_LEFT, false);
    CHECK(remapper_hid_aggregator_apply_mouse(&agg, &event));
    remapper_hid_aggregator_snapshot(&agg, &output);
    CHECK(mouse_is_down(&output, REMAPPER_MOUSE_BUTTON_LEFT));

    event = mouse_button(composite, REMAPPER_MOUSE_BUTTON_LEFT, false);
    CHECK(remapper_hid_aggregator_apply_mouse(&agg, &event));
    /* Duplicate release must not underflow ownership. */
    CHECK(remapper_hid_aggregator_apply_mouse(&agg, &event));
    remapper_hid_aggregator_snapshot(&agg, &output);
    CHECK(!mouse_is_down(&output, REMAPPER_MOUSE_BUTTON_LEFT));
}

static void test_keyboard_key_and_modifier_refcounts(void)
{
    remapper_hid_aggregator_t agg;
    remapper_hid_output_state_t output;
    remapper_hid_aggregator_init(&agg);

    const remapper_hid_source_t keyboard =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_KEYBOARD, 1);
    const remapper_hid_source_t composite =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_COMPOSITE, 1);

    remapper_canonical_keyboard_event_t key =
        keyboard_key(keyboard, REMAPPER_KEY_A, true);
    CHECK(remapper_hid_aggregator_apply_keyboard(&agg, &key));
    key = keyboard_key(composite, REMAPPER_KEY_A, true);
    CHECK(remapper_hid_aggregator_apply_keyboard(&agg, &key));

    remapper_canonical_keyboard_event_t modifier =
        keyboard_modifier(keyboard, REMAPPER_MOD_LEFT_SHIFT, true);
    CHECK(remapper_hid_aggregator_apply_keyboard(&agg, &modifier));
    modifier = keyboard_modifier(composite, REMAPPER_MOD_LEFT_SHIFT, true);
    CHECK(remapper_hid_aggregator_apply_keyboard(&agg, &modifier));

    CHECK(remapper_hid_aggregator_release_source(&agg, keyboard));
    remapper_hid_aggregator_snapshot(&agg, &output);
    CHECK(remapper_hid_output_key_is_down(&output, REMAPPER_KEY_A));
    CHECK((output.modifiers & (1u << REMAPPER_MOD_LEFT_SHIFT)) != 0u);

    CHECK(remapper_hid_aggregator_release_source(&agg, composite));
    remapper_hid_aggregator_snapshot(&agg, &output);
    CHECK(!remapper_hid_output_key_is_down(&output, REMAPPER_KEY_A));
    CHECK(output.modifiers == 0u);
}

static void test_disconnect_releases_only_affected_source(void)
{
    remapper_hid_aggregator_t agg;
    remapper_hid_output_state_t output;
    remapper_hid_aggregator_init(&agg);

    const remapper_hid_source_t mouse =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 7);
    const remapper_hid_source_t composite =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_COMPOSITE, 9);

    remapper_canonical_mouse_event_t event =
        mouse_button(mouse, REMAPPER_MOUSE_BUTTON_LEFT, true);
    CHECK(remapper_hid_aggregator_apply_mouse(&agg, &event));
    event = mouse_button(mouse, REMAPPER_MOUSE_BUTTON_RIGHT, true);
    CHECK(remapper_hid_aggregator_apply_mouse(&agg, &event));
    event = mouse_button(composite, REMAPPER_MOUSE_BUTTON_LEFT, true);
    CHECK(remapper_hid_aggregator_apply_mouse(&agg, &event));

    CHECK(remapper_hid_aggregator_release_source(&agg, mouse));
    remapper_hid_aggregator_snapshot(&agg, &output);
    CHECK(mouse_is_down(&output, REMAPPER_MOUSE_BUTTON_LEFT));
    CHECK(!mouse_is_down(&output, REMAPPER_MOUSE_BUTTON_RIGHT));

    /* Releasing an already absent source is a safe teardown operation. */
    CHECK(remapper_hid_aggregator_release_source(&agg, mouse));
}

static void test_synthetic_keyboard_coexists_with_physical_keyboard(void)
{
    remapper_hid_aggregator_t agg;
    remapper_hid_output_state_t output;
    remapper_hid_aggregator_init(&agg);

    const remapper_hid_source_t physical =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_KEYBOARD, 1);
    const remapper_hid_source_t synthetic =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_SYNTHETIC_REMAP, 1);

    remapper_canonical_keyboard_event_t event =
        keyboard_key(physical, REMAPPER_KEY_ESCAPE, true);
    CHECK(remapper_hid_aggregator_apply_keyboard(&agg, &event));
    event = keyboard_key(synthetic, REMAPPER_KEY_ESCAPE, true);
    CHECK(remapper_hid_aggregator_apply_keyboard(&agg, &event));

    CHECK(remapper_hid_aggregator_release_source(&agg, synthetic));
    remapper_hid_aggregator_snapshot(&agg, &output);
    CHECK(remapper_hid_output_key_is_down(&output, REMAPPER_KEY_ESCAPE));

    event = keyboard_key(physical, REMAPPER_KEY_ESCAPE, false);
    CHECK(remapper_hid_aggregator_apply_keyboard(&agg, &event));
    remapper_hid_aggregator_snapshot(&agg, &output);
    CHECK(!remapper_hid_output_key_is_down(&output, REMAPPER_KEY_ESCAPE));
}

static void test_relative_motion_is_aggregated_then_consumed(void)
{
    remapper_hid_aggregator_t agg;
    remapper_hid_output_state_t output;
    remapper_hid_aggregator_init(&agg);

    const remapper_hid_source_t mouse =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1);
    const remapper_hid_source_t composite =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_COMPOSITE, 1);

    remapper_canonical_mouse_event_t move = {0};
    move.source = mouse;
    move.type = REMAPPER_MOUSE_EVENT_MOVE;
    move.data.move.dx = 12;
    move.data.move.dy = -3;
    CHECK(remapper_hid_aggregator_apply_mouse(&agg, &move));

    move.source = composite;
    move.data.move.dx = -2;
    move.data.move.dy = 8;
    CHECK(remapper_hid_aggregator_apply_mouse(&agg, &move));

    remapper_canonical_mouse_event_t wheel = {0};
    wheel.source = mouse;
    wheel.type = REMAPPER_MOUSE_EVENT_WHEEL;
    wheel.data.wheel.vertical = 2;
    wheel.data.wheel.horizontal = -1;
    CHECK(remapper_hid_aggregator_apply_mouse(&agg, &wheel));

    remapper_hid_aggregator_take_output(&agg, &output);
    CHECK(output.dx == 10);
    CHECK(output.dy == 5);
    CHECK(output.wheel_vertical == 2);
    CHECK(output.wheel_horizontal == -1);

    remapper_hid_aggregator_snapshot(&agg, &output);
    CHECK(output.dx == 0);
    CHECK(output.dy == 0);
    CHECK(output.wheel_vertical == 0);
    CHECK(output.wheel_horizontal == 0);
}

int main(void)
{
    test_two_sources_share_mouse_button();
    test_keyboard_key_and_modifier_refcounts();
    test_disconnect_releases_only_affected_source();
    test_synthetic_keyboard_coexists_with_physical_keyboard();
    test_relative_motion_is_aggregated_then_consumed();

    puts("G05 canonical HID ownership tests passed");
    return 0;
}
