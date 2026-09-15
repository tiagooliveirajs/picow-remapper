#ifndef REMAPPER_INTERACTION_INTERACTION_H
#define REMAPPER_INTERACTION_INTERACTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "remapper/domain/command.h"
#include "remapper/ux/ux_model.h"

#define REMAPPER_INTERACTION_HISTORY_DEPTH 8u

typedef enum {
    REMAPPER_CONTROL_JOY_UP = 0,
    REMAPPER_CONTROL_JOY_DOWN,
    REMAPPER_CONTROL_JOY_LEFT,
    REMAPPER_CONTROL_JOY_RIGHT,
    REMAPPER_CONTROL_JOY_PRESS,
    REMAPPER_CONTROL_KEY_A,
    REMAPPER_CONTROL_KEY_B,
    REMAPPER_CONTROL_KEY_X,
    REMAPPER_CONTROL_KEY_Y,
    REMAPPER_CONTROL_COUNT,
} remapper_control_t;

typedef enum {
    REMAPPER_INPUT_PRESS = 0,
    REMAPPER_INPUT_RELEASE,
} remapper_input_phase_t;

typedef struct {
    remapper_control_t control;
    remapper_input_phase_t phase;
} remapper_input_event_t;

typedef struct {
    bool consumed;
    bool visual_changed;
    bool selection_changed;
    bool screen_changed;
    bool lock_changed;
    remapper_command_t command;
} remapper_interaction_result_t;

typedef struct {
    remapper_ux_screen_t screen;
    remapper_ux_screen_t history[REMAPPER_INTERACTION_HISTORY_DEPTH];
    uint8_t history_depth;
    uint8_t selected_option;
    uint32_t pressed_mask;
    bool locked;

    remapper_ux_screen_t help_return_screen;
    uint8_t help_return_selection;

    uint8_t dynamic_option_count;
    uint32_t dynamic_option_values[REMAPPER_UX_MAX_DYNAMIC_OPTIONS];
} remapper_interaction_state_t;

void remapper_interaction_init(remapper_interaction_state_t *state);
remapper_interaction_result_t remapper_interaction_handle_event(
    remapper_interaction_state_t *state,
    remapper_input_event_t event
);

bool remapper_interaction_set_dynamic_options(
    remapper_interaction_state_t *state,
    const uint32_t *values,
    size_t count
);

size_t remapper_interaction_option_count(const remapper_interaction_state_t *state);
bool remapper_interaction_is_pressed(
    const remapper_interaction_state_t *state,
    remapper_control_t control
);
bool remapper_interaction_selection_emphasized(const remapper_interaction_state_t *state);

#endif
