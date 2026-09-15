#include "remapper/remap/remap.h"

#include <string.h>

static remapper_mouse_target_t identity_target(remapper_mouse_source_button_t source)
{
    switch (source) {
    case REMAPPER_MOUSE_SOURCE_LEFT: return REMAPPER_MOUSE_TARGET_LEFT;
    case REMAPPER_MOUSE_SOURCE_RIGHT: return REMAPPER_MOUSE_TARGET_RIGHT;
    case REMAPPER_MOUSE_SOURCE_MIDDLE: return REMAPPER_MOUSE_TARGET_MIDDLE;
    case REMAPPER_MOUSE_SOURCE_FORWARD: return REMAPPER_MOUSE_TARGET_FORWARD;
    case REMAPPER_MOUSE_SOURCE_BACKWARD: return REMAPPER_MOUSE_TARGET_BACKWARD;
    }
    return REMAPPER_MOUSE_TARGET_LEFT;
}

static bool button_to_source(
    remapper_mouse_button_t button,
    remapper_mouse_source_button_t *source)
{
    if (source == NULL) return false;
    switch (button) {
    case REMAPPER_MOUSE_BUTTON_LEFT:
        *source = REMAPPER_MOUSE_SOURCE_LEFT;
        return true;
    case REMAPPER_MOUSE_BUTTON_RIGHT:
        *source = REMAPPER_MOUSE_SOURCE_RIGHT;
        return true;
    case REMAPPER_MOUSE_BUTTON_MIDDLE:
        *source = REMAPPER_MOUSE_SOURCE_MIDDLE;
        return true;
    case REMAPPER_MOUSE_BUTTON_FORWARD:
        *source = REMAPPER_MOUSE_SOURCE_FORWARD;
        return true;
    case REMAPPER_MOUSE_BUTTON_BACK:
        *source = REMAPPER_MOUSE_SOURCE_BACKWARD;
        return true;
    default:
        return false;
    }
}

static bool target_to_button(
    remapper_mouse_target_t target,
    remapper_mouse_button_t *button)
{
    if (button == NULL) return false;
    switch (target) {
    case REMAPPER_MOUSE_TARGET_LEFT:
        *button = REMAPPER_MOUSE_BUTTON_LEFT;
        return true;
    case REMAPPER_MOUSE_TARGET_RIGHT:
        *button = REMAPPER_MOUSE_BUTTON_RIGHT;
        return true;
    case REMAPPER_MOUSE_TARGET_MIDDLE:
        *button = REMAPPER_MOUSE_BUTTON_MIDDLE;
        return true;
    case REMAPPER_MOUSE_TARGET_FORWARD:
        *button = REMAPPER_MOUSE_BUTTON_FORWARD;
        return true;
    case REMAPPER_MOUSE_TARGET_BACKWARD:
        *button = REMAPPER_MOUSE_BUTTON_BACK;
        return true;
    case REMAPPER_MOUSE_TARGET_ESCAPE:
    case REMAPPER_MOUSE_TARGET_COUNT:
        return false;
    }
    return false;
}

void remapper_remap_init(remapper_remap_t *remap)
{
    if (remap == NULL) return;
    memset(remap, 0, sizeof(*remap));
    remap->profile.kind = REMAPPER_MOUSE_PROFILE_PASSTHROUGH;
    for (unsigned int source = 0u; source < REMAPPER_MOUSE_SOURCE_COUNT; ++source) {
        remap->profile.targets[source] =
            identity_target((remapper_mouse_source_button_t)source);
    }
}

void remapper_remap_set_profile(
    remapper_remap_t *remap,
    const remapper_mouse_profile_config_t *profile)
{
    if (remap == NULL || profile == NULL) return;
    remap->profile = *profile;
}

bool remapper_remap_process_mouse(
    const remapper_remap_t *remap,
    const remapper_canonical_mouse_event_t *input,
    remapper_remap_result_t *result)
{
    if (remap == NULL || input == NULL || result == NULL ||
        !remapper_hid_source_is_valid(input->source)) {
        return false;
    }

    memset(result, 0, sizeof(*result));
    if (input->type != REMAPPER_MOUSE_EVENT_BUTTON) {
        result->has_mouse = true;
        result->mouse = *input;
        return true;
    }

    remapper_mouse_source_button_t source;
    if (!button_to_source(input->data.button.button, &source)) {
        result->has_mouse = true;
        result->mouse = *input;
        return true;
    }

    remapper_mouse_target_t target = remap->profile.targets[source];
    if (remap->profile.kind == REMAPPER_MOUSE_PROFILE_PASSTHROUGH) {
        target = identity_target(source);
    }

    if (target == REMAPPER_MOUSE_TARGET_ESCAPE) {
        result->has_keyboard = true;
        result->keyboard.source = remapper_hid_source_make(
            REMAPPER_HID_SOURCE_SYNTHETIC_REMAP,
            input->source.instance);
        result->keyboard.type = REMAPPER_KEYBOARD_EVENT_KEY;
        result->keyboard.data.key.key = REMAPPER_KEY_ESCAPE;
        result->keyboard.data.key.pressed = input->data.button.pressed;
        return true;
    }

    remapper_mouse_button_t button;
    if (!target_to_button(target, &button)) return false;
    result->has_mouse = true;
    result->mouse = *input;
    result->mouse.data.button.button = button;
    return true;
}
