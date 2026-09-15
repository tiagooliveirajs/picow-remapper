#include "remapper/hid_aggregator/hid_aggregator.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

static bool key_bitmap_get(const uint8_t *bitmap, remapper_key_t key)
{
    const uint8_t mask = (uint8_t)(1u << (key & 7u));
    return (bitmap[key >> 3u] & mask) != 0u;
}

static void key_bitmap_set(uint8_t *bitmap, remapper_key_t key, bool pressed)
{
    const uint8_t mask = (uint8_t)(1u << (key & 7u));
    uint8_t *cell = &bitmap[key >> 3u];
    if (pressed) {
        *cell = (uint8_t)(*cell | mask);
    } else {
        *cell = (uint8_t)(*cell & (uint8_t)~mask);
    }
}

static int32_t saturating_add_i32(int32_t current, int32_t delta)
{
    const int64_t sum = (int64_t)current + (int64_t)delta;
    if (sum > INT32_MAX) return INT32_MAX;
    if (sum < INT32_MIN) return INT32_MIN;
    return (int32_t)sum;
}

static remapper_hid_source_state_t *find_source(
    remapper_hid_aggregator_t *aggregator,
    remapper_hid_source_t source)
{
    for (size_t i = 0u; i < REMAPPER_HID_AGGREGATOR_MAX_SOURCES; ++i) {
        remapper_hid_source_state_t *slot = &aggregator->sources[i];
        if (slot->active && remapper_hid_source_equal(slot->id, source)) return slot;
    }
    return NULL;
}

static remapper_hid_source_state_t *find_or_allocate_source(
    remapper_hid_aggregator_t *aggregator,
    remapper_hid_source_t source)
{
    remapper_hid_source_state_t *slot = find_source(aggregator, source);
    if (slot != NULL) return slot;

    for (size_t i = 0u; i < REMAPPER_HID_AGGREGATOR_MAX_SOURCES; ++i) {
        slot = &aggregator->sources[i];
        if (!slot->active) {
            memset(slot, 0, sizeof(*slot));
            slot->active = true;
            slot->id = source;
            return slot;
        }
    }
    return NULL;
}

void remapper_hid_aggregator_init(remapper_hid_aggregator_t *aggregator)
{
    if (aggregator != NULL) memset(aggregator, 0, sizeof(*aggregator));
}

bool remapper_hid_aggregator_apply_mouse(
    remapper_hid_aggregator_t *aggregator,
    const remapper_canonical_mouse_event_t *event)
{
    if (aggregator == NULL || event == NULL || !remapper_hid_source_is_valid(event->source)) return false;

    switch (event->type) {
    case REMAPPER_MOUSE_EVENT_BUTTON: {
        const remapper_mouse_button_t button = event->data.button.button;
        if ((unsigned int)button >= (unsigned int)REMAPPER_MOUSE_BUTTON_COUNT) return false;

        remapper_hid_source_state_t *slot = find_source(aggregator, event->source);
        const uint8_t mask = (uint8_t)(1u << (uint8_t)button);
        if (event->data.button.pressed) {
            if (slot == NULL) {
                slot = find_or_allocate_source(aggregator, event->source);
                if (slot == NULL) return false;
            }
            if ((slot->mouse_buttons & mask) == 0u) {
                slot->mouse_buttons = (uint8_t)(slot->mouse_buttons | mask);
                ++aggregator->mouse_button_refs[button];
            }
            return true;
        }

        if (slot != NULL && (slot->mouse_buttons & mask) != 0u) {
            slot->mouse_buttons = (uint8_t)(slot->mouse_buttons & (uint8_t)~mask);
            if (aggregator->mouse_button_refs[button] > 0u) --aggregator->mouse_button_refs[button];
        }
        return true;
    }

    case REMAPPER_MOUSE_EVENT_MOVE:
        aggregator->pending_dx = saturating_add_i32(aggregator->pending_dx, event->data.move.dx);
        aggregator->pending_dy = saturating_add_i32(aggregator->pending_dy, event->data.move.dy);
        return true;

    case REMAPPER_MOUSE_EVENT_WHEEL:
        aggregator->pending_wheel_vertical = saturating_add_i32(
            aggregator->pending_wheel_vertical, event->data.wheel.vertical);
        aggregator->pending_wheel_horizontal = saturating_add_i32(
            aggregator->pending_wheel_horizontal, event->data.wheel.horizontal);
        return true;

    default:
        return false;
    }
}

bool remapper_hid_aggregator_apply_keyboard(
    remapper_hid_aggregator_t *aggregator,
    const remapper_canonical_keyboard_event_t *event)
{
    if (aggregator == NULL || event == NULL || !remapper_hid_source_is_valid(event->source)) return false;

    remapper_hid_source_state_t *slot = find_source(aggregator, event->source);
    switch (event->type) {
    case REMAPPER_KEYBOARD_EVENT_KEY: {
        const remapper_key_t key = event->data.key.key;
        if (event->data.key.pressed) {
            if (slot == NULL) {
                slot = find_or_allocate_source(aggregator, event->source);
                if (slot == NULL) return false;
            }
            if (!key_bitmap_get(slot->key_bitmap, key)) {
                key_bitmap_set(slot->key_bitmap, key, true);
                ++aggregator->key_refs[key];
            }
            return true;
        }

        if (slot != NULL && key_bitmap_get(slot->key_bitmap, key)) {
            key_bitmap_set(slot->key_bitmap, key, false);
            if (aggregator->key_refs[key] > 0u) --aggregator->key_refs[key];
        }
        return true;
    }

    case REMAPPER_KEYBOARD_EVENT_MODIFIER: {
        const remapper_modifier_t modifier = event->data.modifier.modifier;
        if ((unsigned int)modifier >= (unsigned int)REMAPPER_MOD_COUNT) return false;
        const uint8_t mask = (uint8_t)(1u << (uint8_t)modifier);

        if (event->data.modifier.pressed) {
            if (slot == NULL) {
                slot = find_or_allocate_source(aggregator, event->source);
                if (slot == NULL) return false;
            }
            if ((slot->modifiers & mask) == 0u) {
                slot->modifiers = (uint8_t)(slot->modifiers | mask);
                ++aggregator->modifier_refs[modifier];
            }
            return true;
        }

        if (slot != NULL && (slot->modifiers & mask) != 0u) {
            slot->modifiers = (uint8_t)(slot->modifiers & (uint8_t)~mask);
            if (aggregator->modifier_refs[modifier] > 0u) --aggregator->modifier_refs[modifier];
        }
        return true;
    }

    default:
        return false;
    }
}

bool remapper_hid_aggregator_release_source(
    remapper_hid_aggregator_t *aggregator,
    remapper_hid_source_t source)
{
    if (aggregator == NULL || !remapper_hid_source_is_valid(source)) return false;

    remapper_hid_source_state_t *slot = find_source(aggregator, source);
    if (slot == NULL) return true;

    for (size_t button = 0u; button < REMAPPER_MOUSE_BUTTON_COUNT; ++button) {
        const uint8_t mask = (uint8_t)(1u << button);
        if ((slot->mouse_buttons & mask) != 0u && aggregator->mouse_button_refs[button] > 0u) {
            --aggregator->mouse_button_refs[button];
        }
    }

    for (size_t key = 0u; key < REMAPPER_HID_KEY_COUNT; ++key) {
        if (key_bitmap_get(slot->key_bitmap, (remapper_key_t)key) && aggregator->key_refs[key] > 0u) {
            --aggregator->key_refs[key];
        }
    }

    for (size_t modifier = 0u; modifier < REMAPPER_MOD_COUNT; ++modifier) {
        const uint8_t mask = (uint8_t)(1u << modifier);
        if ((slot->modifiers & mask) != 0u && aggregator->modifier_refs[modifier] > 0u) {
            --aggregator->modifier_refs[modifier];
        }
    }

    memset(slot, 0, sizeof(*slot));
    return true;
}

void remapper_hid_aggregator_snapshot(
    const remapper_hid_aggregator_t *aggregator,
    remapper_hid_output_state_t *out_state)
{
    if (out_state == NULL) return;
    memset(out_state, 0, sizeof(*out_state));
    if (aggregator == NULL) return;

    for (size_t button = 0u; button < REMAPPER_MOUSE_BUTTON_COUNT; ++button) {
        if (aggregator->mouse_button_refs[button] > 0u) {
            out_state->mouse_buttons = (uint8_t)(out_state->mouse_buttons | (uint8_t)(1u << button));
        }
    }

    for (size_t key = 0u; key < REMAPPER_HID_KEY_COUNT; ++key) {
        if (aggregator->key_refs[key] > 0u) key_bitmap_set(out_state->key_bitmap, (remapper_key_t)key, true);
    }

    for (size_t modifier = 0u; modifier < REMAPPER_MOD_COUNT; ++modifier) {
        if (aggregator->modifier_refs[modifier] > 0u) {
            out_state->modifiers = (uint8_t)(out_state->modifiers | (uint8_t)(1u << modifier));
        }
    }

    out_state->dx = aggregator->pending_dx;
    out_state->dy = aggregator->pending_dy;
    out_state->wheel_vertical = aggregator->pending_wheel_vertical;
    out_state->wheel_horizontal = aggregator->pending_wheel_horizontal;
}

void remapper_hid_aggregator_take_output(
    remapper_hid_aggregator_t *aggregator,
    remapper_hid_output_state_t *out_state)
{
    remapper_hid_aggregator_snapshot(aggregator, out_state);
    if (aggregator == NULL) return;
    aggregator->pending_dx = 0;
    aggregator->pending_dy = 0;
    aggregator->pending_wheel_vertical = 0;
    aggregator->pending_wheel_horizontal = 0;
}

bool remapper_hid_output_mouse_button_is_down(
    const remapper_hid_output_state_t *state,
    remapper_mouse_button_t button)
{
    if (state == NULL || (unsigned int)button >= (unsigned int)REMAPPER_MOUSE_BUTTON_COUNT) return false;
    return (state->mouse_buttons & (uint8_t)(1u << (uint8_t)button)) != 0u;
}

bool remapper_hid_output_key_is_down(
    const remapper_hid_output_state_t *state,
    remapper_key_t key)
{
    return state != NULL && key_bitmap_get(state->key_bitmap, key);
}
