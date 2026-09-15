#include "remapper/app/ui_projection.h"

#include <stdio.h>

#include "remapper/domain/product_types.h"

static bool is_pressed(
    const remapper_interaction_state_t *state,
    remapper_control_t control)
{
    return remapper_interaction_is_pressed(state, control);
}

static remapper_ui_tone_t press_tone(
    const remapper_interaction_state_t *state,
    remapper_control_t control)
{
    return is_pressed(state, control)
        ? REMAPPER_UI_TONE_EMPHASIZED
        : REMAPPER_UI_TONE_ACTIONABLE;
}

static void set_hint(
    remapper_ui_frame_t *frame,
    uint8_t row,
    const char *text,
    remapper_control_t control,
    const remapper_interaction_state_t *state)
{
    (void)remapper_ui_frame_set_text(
        frame, row, 0u, text, press_tone(state, control));
}

static void project_learn(
    const remapper_interaction_state_t *state,
    remapper_ui_frame_t *frame)
{
    remapper_ui_frame_reset(frame, true, REMAPPER_RENDERER_TEXT_ROWS);
    (void)remapper_ui_frame_set_text(frame, 0u, 0u, "LEARN THE KEYS", REMAPPER_UI_TONE_TITLE);
    (void)remapper_ui_frame_set_text(frame, 1u, 0u, "UP/DOWN: SELECT", REMAPPER_UI_TONE_ACTIONABLE);
    (void)remapper_ui_frame_set_text(frame, 2u, 0u, "RIGHT/PRESS: OPEN", REMAPPER_UI_TONE_ACTIONABLE);
    (void)remapper_ui_frame_set_text(frame, 3u, 0u, "LEFT: HOME", REMAPPER_UI_TONE_ACTIONABLE);
    (void)remapper_ui_frame_set_text(frame, 4u, 0u, "KEY A: APPLY", REMAPPER_UI_TONE_ACTIONABLE);
    (void)remapper_ui_frame_set_text(frame, 5u, 0u, "KEY B: BACK", REMAPPER_UI_TONE_ACTIONABLE);
    (void)remapper_ui_frame_set_text(frame, 6u, 0u, "KEY X: HELP", REMAPPER_UI_TONE_ACTIONABLE);
    (void)remapper_ui_frame_set_text(frame, 7u, 0u, "KEY Y: LOCK", REMAPPER_UI_TONE_ACTIONABLE);
    (void)remapper_ui_frame_set_text(frame, 8u, 0u, "RELEASE RUNS ACTION", REMAPPER_UI_TONE_STATIC);

    if (is_pressed(state, REMAPPER_CONTROL_JOY_UP))
        (void)remapper_ui_frame_set_tone_span(frame, 1u, 0u, 2u, REMAPPER_UI_TONE_EMPHASIZED);
    if (is_pressed(state, REMAPPER_CONTROL_JOY_DOWN))
        (void)remapper_ui_frame_set_tone_span(frame, 1u, 3u, 4u, REMAPPER_UI_TONE_EMPHASIZED);
    if (is_pressed(state, REMAPPER_CONTROL_JOY_RIGHT))
        (void)remapper_ui_frame_set_tone_span(frame, 2u, 0u, 5u, REMAPPER_UI_TONE_EMPHASIZED);
    if (is_pressed(state, REMAPPER_CONTROL_JOY_PRESS))
        (void)remapper_ui_frame_set_tone_span(frame, 2u, 6u, 5u, REMAPPER_UI_TONE_EMPHASIZED);
    if (is_pressed(state, REMAPPER_CONTROL_JOY_LEFT))
        (void)remapper_ui_frame_set_tone_span(frame, 3u, 0u, 4u, REMAPPER_UI_TONE_EMPHASIZED);
    if (is_pressed(state, REMAPPER_CONTROL_KEY_A))
        (void)remapper_ui_frame_set_tone_span(frame, 4u, 0u, 5u, REMAPPER_UI_TONE_EMPHASIZED);
    if (is_pressed(state, REMAPPER_CONTROL_KEY_B))
        (void)remapper_ui_frame_set_tone_span(frame, 5u, 0u, 5u, REMAPPER_UI_TONE_EMPHASIZED);
    if (is_pressed(state, REMAPPER_CONTROL_KEY_X))
        (void)remapper_ui_frame_set_tone_span(frame, 6u, 0u, 5u, REMAPPER_UI_TONE_EMPHASIZED);
    if (is_pressed(state, REMAPPER_CONTROL_KEY_Y))
        (void)remapper_ui_frame_set_tone_span(frame, 7u, 0u, 5u, REMAPPER_UI_TONE_EMPHASIZED);
}

static uint8_t hint_start_for_screen(remapper_ux_screen_t screen)
{
    switch (screen) {
    case REMAPPER_UX_SCREEN_STATUS:
    case REMAPPER_UX_SCREEN_PAIR_MOUSE:
    case REMAPPER_UX_SCREEN_PROFILE_PASSTHROUGH:
    case REMAPPER_UX_SCREEN_PROFILE_DEFAULT_REMAP:
    case REMAPPER_UX_SCREEN_PROFILE_ESCAPE_REMAP:
    case REMAPPER_UX_SCREEN_MAP_LEFT:
    case REMAPPER_UX_SCREEN_MAP_RIGHT:
    case REMAPPER_UX_SCREEN_MAP_MIDDLE:
    case REMAPPER_UX_SCREEN_MAP_FORWARD:
    case REMAPPER_UX_SCREEN_MAP_BACKWARD:
    case REMAPPER_UX_SCREEN_PAIR_KEYBOARD:
    case REMAPPER_UX_SCREEN_PAIR_COMPOSITE:
    case REMAPPER_UX_SCREEN_REMOVE_DEVICE:
        return 6u;
    case REMAPPER_UX_SCREEN_HELP:
        return 8u;
    default:
        return 7u;
    }
}

static const char *target_label(uint32_t value)
{
    switch ((remapper_mouse_target_t)value) {
    case REMAPPER_MOUSE_TARGET_LEFT: return "LEFT";
    case REMAPPER_MOUSE_TARGET_RIGHT: return "RIGHT";
    case REMAPPER_MOUSE_TARGET_MIDDLE: return "MIDDLE";
    case REMAPPER_MOUSE_TARGET_FORWARD: return "FORWARD";
    case REMAPPER_MOUSE_TARGET_BACKWARD: return "BACKWARD";
    case REMAPPER_MOUSE_TARGET_ESCAPE: return "ESCAPE";
    case REMAPPER_MOUSE_TARGET_COUNT: break;
    }
    return "INVALID";
}

static void project_options(
    const remapper_interaction_state_t *state,
    remapper_ui_frame_t *frame,
    const remapper_ux_screen_definition_t *definition)
{
    if (definition->dynamic_options) {
        if (state->dynamic_option_count == 0u) {
            (void)remapper_ui_frame_set_text(
                frame, 1u, 0u, "NO OPTIONS", REMAPPER_UI_TONE_STATIC);
            return;
        }
        const size_t visible = state->dynamic_option_count < 5u
            ? state->dynamic_option_count : 5u;
        for (size_t i = 0u; i < visible; ++i) {
            char label[REMAPPER_RENDERER_TEXT_COLS + 1u];
            (void)snprintf(
                label, sizeof(label), " %s",
                target_label(state->dynamic_option_values[i]));
            const bool selected = i == state->selected_option &&
                remapper_interaction_selection_emphasized(state);
            (void)remapper_ui_frame_set_text(
                frame, (uint8_t)(i + 1u), 0u, label,
                selected ? REMAPPER_UI_TONE_EMPHASIZED : REMAPPER_UI_TONE_ACTIONABLE);
        }
        return;
    }

    size_t visible = definition->option_count;
    if (visible > 5u) visible = 5u;
    for (size_t i = 0u; i < visible; ++i) {
        const remapper_ux_option_t *option = remapper_ux_option_at(state->screen, i);
        if (option == NULL) continue;
        char label[REMAPPER_RENDERER_TEXT_COLS + 1u];
        (void)snprintf(label, sizeof(label), " %s", option->label);
        const bool selected = i == state->selected_option &&
            remapper_interaction_selection_emphasized(state);
        (void)remapper_ui_frame_set_text(
            frame, (uint8_t)(i + 1u), 0u, label,
            selected ? REMAPPER_UI_TONE_EMPHASIZED : REMAPPER_UI_TONE_ACTIONABLE);
    }
}

static void project_profile_preview(
    remapper_ux_screen_t screen,
    remapper_ui_frame_t *frame)
{
    switch (screen) {
    case REMAPPER_UX_SCREEN_PROFILE_PASSTHROUGH:
        (void)remapper_ui_frame_set_text(frame, 1u, 0u, "ALL BUTTONS ORIGINAL", REMAPPER_UI_TONE_STATIC);
        break;
    case REMAPPER_UX_SCREEN_PROFILE_DEFAULT_REMAP:
        (void)remapper_ui_frame_set_text(frame, 1u, 0u, "LEFT -> ESCAPE", REMAPPER_UI_TONE_STATIC);
        (void)remapper_ui_frame_set_text(frame, 2u, 0u, "FWD -> LEFT", REMAPPER_UI_TONE_STATIC);
        (void)remapper_ui_frame_set_text(frame, 3u, 0u, "BACK -> RIGHT", REMAPPER_UI_TONE_STATIC);
        (void)remapper_ui_frame_set_text(frame, 4u, 0u, "RIGHT -> BACK", REMAPPER_UI_TONE_STATIC);
        (void)remapper_ui_frame_set_text(frame, 5u, 0u, "MIDDLE -> FORWARD", REMAPPER_UI_TONE_STATIC);
        break;
    case REMAPPER_UX_SCREEN_PROFILE_ESCAPE_REMAP:
        (void)remapper_ui_frame_set_text(frame, 1u, 0u, "LEFT -> ESCAPE", REMAPPER_UI_TONE_STATIC);
        (void)remapper_ui_frame_set_text(frame, 2u, 0u, "OTHERS PASSTHROUGH", REMAPPER_UI_TONE_STATIC);
        break;
    default:
        break;
    }
}

static void project_body(
    const remapper_interaction_state_t *state,
    remapper_ui_frame_t *frame,
    const remapper_ux_screen_definition_t *definition)
{
    if (definition->option_count > 0u || definition->dynamic_options) {
        project_options(state, frame, definition);
        return;
    }
    switch (state->screen) {
    case REMAPPER_UX_SCREEN_STATUS:
        (void)remapper_ui_frame_set_text(frame, 1u, 0u, "USB HID READY", REMAPPER_UI_TONE_STATIC);
        break;
    case REMAPPER_UX_SCREEN_PAIR_MOUSE:
    case REMAPPER_UX_SCREEN_PAIR_KEYBOARD:
    case REMAPPER_UX_SCREEN_PAIR_COMPOSITE:
        (void)remapper_ui_frame_set_text(frame, 1u, 0u, "PAIRING UI LATER", REMAPPER_UI_TONE_STATIC);
        break;
    case REMAPPER_UX_SCREEN_PROFILE_PASSTHROUGH:
    case REMAPPER_UX_SCREEN_PROFILE_DEFAULT_REMAP:
    case REMAPPER_UX_SCREEN_PROFILE_ESCAPE_REMAP:
        project_profile_preview(state->screen, frame);
        break;
    case REMAPPER_UX_SCREEN_REMOVE_DEVICE:
        (void)remapper_ui_frame_set_text(frame, 1u, 0u, "CONFIRM REMOVE?", REMAPPER_UI_TONE_STATIC);
        break;
    case REMAPPER_UX_SCREEN_HELP:
        (void)remapper_ui_frame_set_text(frame, 1u, 0u, "CONTEXT HELP", REMAPPER_UI_TONE_STATIC);
        (void)remapper_ui_frame_set_text(frame, 2u, 0u, "ANY CONTROL RETURNS", REMAPPER_UI_TONE_STATIC);
        break;
    default:
        break;
    }
}

static void project_hints(
    const remapper_interaction_state_t *state,
    remapper_ui_frame_t *frame,
    uint8_t hint_start)
{
    switch (state->screen) {
    case REMAPPER_UX_SCREEN_HOME:
        (void)remapper_ui_frame_set_text(frame, hint_start, 0u, "JOY: SELECT/OPEN", REMAPPER_UI_TONE_ACTIONABLE);
        set_hint(frame, (uint8_t)(hint_start + 1u), "KEY Y: LOCK", REMAPPER_CONTROL_KEY_Y, state);
        break;
    case REMAPPER_UX_SCREEN_HELP:
        (void)remapper_ui_frame_set_text(
            frame, hint_start, 0u, "ANY KEY: BACK",
            state->pressed_mask != 0u ? REMAPPER_UI_TONE_EMPHASIZED : REMAPPER_UI_TONE_ACTIONABLE);
        break;
    case REMAPPER_UX_SCREEN_STATUS:
    case REMAPPER_UX_SCREEN_PAIR_MOUSE:
    case REMAPPER_UX_SCREEN_PAIR_KEYBOARD:
    case REMAPPER_UX_SCREEN_PAIR_COMPOSITE:
        set_hint(frame, hint_start, "KEY B: BACK", REMAPPER_CONTROL_KEY_B, state);
        set_hint(frame, (uint8_t)(hint_start + 1u), "KEY X: HELP", REMAPPER_CONTROL_KEY_X, state);
        set_hint(frame, (uint8_t)(hint_start + 2u), "KEY Y: LOCK", REMAPPER_CONTROL_KEY_Y, state);
        break;
    case REMAPPER_UX_SCREEN_PROFILE_PASSTHROUGH:
    case REMAPPER_UX_SCREEN_PROFILE_DEFAULT_REMAP:
    case REMAPPER_UX_SCREEN_PROFILE_ESCAPE_REMAP:
        set_hint(frame, hint_start, "KEY A: APPLY", REMAPPER_CONTROL_KEY_A, state);
        set_hint(frame, (uint8_t)(hint_start + 1u), "KEY B: BACK", REMAPPER_CONTROL_KEY_B, state);
        set_hint(frame, (uint8_t)(hint_start + 2u), "KEY Y: LOCK", REMAPPER_CONTROL_KEY_Y, state);
        break;
    case REMAPPER_UX_SCREEN_MAP_LEFT:
    case REMAPPER_UX_SCREEN_MAP_RIGHT:
    case REMAPPER_UX_SCREEN_MAP_MIDDLE:
    case REMAPPER_UX_SCREEN_MAP_FORWARD:
    case REMAPPER_UX_SCREEN_MAP_BACKWARD:
        set_hint(frame, hint_start, "KEY A: APPLY THIS", REMAPPER_CONTROL_KEY_A, state);
        set_hint(frame, (uint8_t)(hint_start + 1u), "KEY B: BACK", REMAPPER_CONTROL_KEY_B, state);
        set_hint(frame, (uint8_t)(hint_start + 2u), "KEY Y: LOCK", REMAPPER_CONTROL_KEY_Y, state);
        break;
    case REMAPPER_UX_SCREEN_REMOVE_DEVICE:
        set_hint(frame, hint_start, "KEY A: REMOVE", REMAPPER_CONTROL_KEY_A, state);
        set_hint(frame, (uint8_t)(hint_start + 1u), "KEY B: BACK", REMAPPER_CONTROL_KEY_B, state);
        set_hint(frame, (uint8_t)(hint_start + 2u), "KEY Y: LOCK", REMAPPER_CONTROL_KEY_Y, state);
        break;
    case REMAPPER_UX_SCREEN_EDIT_CUSTOM_REMAP:
        set_hint(frame, hint_start, "KEY A: APPLY", REMAPPER_CONTROL_KEY_A, state);
        set_hint(frame, (uint8_t)(hint_start + 1u), "KEY Y: LOCK", REMAPPER_CONTROL_KEY_Y, state);
        break;
    default:
        set_hint(frame, hint_start, "KEY B: BACK", REMAPPER_CONTROL_KEY_B, state);
        set_hint(frame, (uint8_t)(hint_start + 1u), "KEY Y: LOCK", REMAPPER_CONTROL_KEY_Y, state);
        break;
    }
}

void remapper_ui_project(
    const remapper_interaction_state_t *state,
    remapper_ui_frame_t *frame)
{
    if (state == NULL || frame == NULL) return;
    if (state->screen == REMAPPER_UX_SCREEN_LEARN_THE_KEYS) {
        project_learn(state, frame);
        return;
    }

    const remapper_ux_screen_definition_t *definition =
        remapper_ux_screen_definition(state->screen);
    if (definition == NULL) {
        remapper_ui_frame_reset(frame, false, 8u);
        (void)remapper_ui_frame_set_text(frame, 0u, 0u, "INVALID SCREEN", REMAPPER_UI_TONE_TITLE);
        return;
    }

    const uint8_t hint_start = hint_start_for_screen(state->screen);
    remapper_ui_frame_reset(frame, false, hint_start);
    (void)remapper_ui_frame_set_text(frame, 0u, 0u, definition->title, REMAPPER_UI_TONE_TITLE);
    project_body(state, frame, definition);
    project_hints(state, frame, hint_start);
}
