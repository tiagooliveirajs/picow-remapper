#include "remapper/interaction/interaction.h"

#include <string.h>

static bool control_is_valid(remapper_control_t control)
{
    return control >= REMAPPER_CONTROL_JOY_UP && control < REMAPPER_CONTROL_COUNT;
}

static uint32_t control_bit(remapper_control_t control)
{
    if (!control_is_valid(control)) {
        return 0u;
    }

    return UINT32_C(1) << (uint32_t)control;
}

static remapper_interaction_result_t result_none(void)
{
    remapper_interaction_result_t result = {0};
    result.command = remapper_command_none();
    return result;
}

static void clear_dynamic_options(remapper_interaction_state_t *state)
{
    state->dynamic_option_count = 0u;
    memset(state->dynamic_option_values, 0, sizeof(state->dynamic_option_values));
}

static void reset_selection_for_screen(remapper_interaction_state_t *state)
{
    state->selected_option = 0u;
    clear_dynamic_options(state);
}

static void push_history(remapper_interaction_state_t *state, remapper_ux_screen_t screen)
{
    if (state->history_depth < REMAPPER_INTERACTION_HISTORY_DEPTH) {
        state->history[state->history_depth] = screen;
        state->history_depth++;
        return;
    }

    for (size_t index = 1u; index < REMAPPER_INTERACTION_HISTORY_DEPTH; ++index) {
        state->history[index - 1u] = state->history[index];
    }
    state->history[REMAPPER_INTERACTION_HISTORY_DEPTH - 1u] = screen;
}

static void navigate_push(
    remapper_interaction_state_t *state,
    remapper_ux_screen_t target,
    remapper_interaction_result_t *result
)
{
    if (!remapper_ux_screen_is_valid(target) || state->screen == target) {
        return;
    }

    push_history(state, state->screen);
    state->screen = target;
    reset_selection_for_screen(state);
    result->screen_changed = true;
    result->visual_changed = true;
}

static void navigate_home(
    remapper_interaction_state_t *state,
    remapper_interaction_result_t *result
)
{
    if (state->screen != REMAPPER_UX_SCREEN_HOME) {
        state->screen = REMAPPER_UX_SCREEN_HOME;
        result->screen_changed = true;
    }

    state->history_depth = 0u;
    reset_selection_for_screen(state);
    result->visual_changed = true;
}

static void navigate_back(
    remapper_interaction_state_t *state,
    remapper_interaction_result_t *result
)
{
    remapper_ux_screen_t target = REMAPPER_UX_SCREEN_HOME;

    if (state->screen == REMAPPER_UX_SCREEN_HOME) {
        return;
    }

    if (state->history_depth > 0u) {
        state->history_depth--;
        target = state->history[state->history_depth];
    }

    state->screen = target;
    reset_selection_for_screen(state);
    result->screen_changed = true;
    result->visual_changed = true;
}

static bool selected_dynamic_value(
    const remapper_interaction_state_t *state,
    uint32_t *value
)
{
    if (state->dynamic_option_count == 0u || state->selected_option >= state->dynamic_option_count) {
        return false;
    }

    *value = state->dynamic_option_values[state->selected_option];
    return true;
}

static bool bind_selected_value(remapper_command_t *command, uint32_t value)
{
    switch (command->kind) {
    case REMAPPER_COMMAND_SELECT_SAVED_DEVICE:
        command->data.saved_device_token = value;
        return true;
    case REMAPPER_COMMAND_APPLY_CUSTOM_MAPPING:
        command->data.custom_mapping.target_token = value;
        return true;
    default:
        return false;
    }
}

static bool prepare_command(
    const remapper_interaction_state_t *state,
    remapper_command_t command_template,
    bool uses_selected_value,
    remapper_command_t *command
)
{
    if (command_template.kind == REMAPPER_COMMAND_NONE) {
        return false;
    }

    *command = command_template;
    if (!uses_selected_value) {
        return true;
    }

    uint32_t value = 0u;
    if (!selected_dynamic_value(state, &value)) {
        *command = remapper_command_none();
        return false;
    }

    if (!bind_selected_value(command, value)) {
        *command = remapper_command_none();
        return false;
    }

    return true;
}

size_t remapper_interaction_option_count(const remapper_interaction_state_t *state)
{
    if (state == NULL) {
        return 0u;
    }

    const remapper_ux_screen_definition_t *definition = remapper_ux_screen_definition(state->screen);
    if (definition == NULL) {
        return 0u;
    }

    if (definition->dynamic_options) {
        return state->dynamic_option_count;
    }

    return definition->option_count;
}

static void move_selection(
    remapper_interaction_state_t *state,
    int direction,
    remapper_interaction_result_t *result
)
{
    const size_t option_count = remapper_interaction_option_count(state);
    if (option_count == 0u) {
        return;
    }

    const uint8_t previous = state->selected_option;
    if (direction < 0) {
        if (state->selected_option > 0u) {
            state->selected_option--;
        }
    } else if ((size_t)state->selected_option + 1u < option_count) {
        state->selected_option++;
    }

    if (state->selected_option != previous) {
        result->selection_changed = true;
        result->visual_changed = true;
    }
}

static void activate_selected_option(
    remapper_interaction_state_t *state,
    remapper_interaction_result_t *result
)
{
    const remapper_ux_screen_definition_t *definition = remapper_ux_screen_definition(state->screen);
    const size_t option_count = remapper_interaction_option_count(state);
    if (definition == NULL || option_count == 0u || state->selected_option >= option_count) {
        return;
    }

    if (definition->dynamic_options) {
        remapper_command_t command = remapper_command_none();
        const bool has_command = prepare_command(
            state,
            definition->dynamic_command,
            definition->dynamic_command_uses_selected_value,
            &command
        );

        if (definition->dynamic_has_target) {
            navigate_push(state, definition->dynamic_target, result);
        }
        if (has_command) {
            result->command = command;
        }
        return;
    }

    const remapper_ux_option_t *option = remapper_ux_option_at(state->screen, state->selected_option);
    if (option == NULL) {
        return;
    }

    const remapper_command_t command = option->command;
    if (option->has_target) {
        navigate_push(state, option->target, result);
    }
    if (command.kind != REMAPPER_COMMAND_NONE) {
        result->command = command;
    }
}

static void activate_primary_action(
    remapper_interaction_state_t *state,
    remapper_interaction_result_t *result
)
{
    const remapper_ux_screen_definition_t *definition = remapper_ux_screen_definition(state->screen);
    if (definition == NULL) {
        return;
    }

    remapper_command_t command = remapper_command_none();
    if (prepare_command(
            state,
            definition->primary_command,
            definition->primary_command_uses_selected_value,
            &command
        )) {
        result->command = command;
    }
}

static void enter_help(
    remapper_interaction_state_t *state,
    remapper_interaction_result_t *result
)
{
    const remapper_ux_screen_definition_t *definition = remapper_ux_screen_definition(state->screen);
    if (definition == NULL || !definition->has_help) {
        return;
    }

    state->help_return_screen = state->screen;
    state->help_return_selection = state->selected_option;
    state->screen = REMAPPER_UX_SCREEN_HELP;
    state->selected_option = 0u;
    result->screen_changed = true;
    result->visual_changed = true;
}

static void leave_help(
    remapper_interaction_state_t *state,
    remapper_interaction_result_t *result
)
{
    remapper_ux_screen_t target = state->help_return_screen;
    if (!remapper_ux_screen_is_valid(target) || target == REMAPPER_UX_SCREEN_HELP) {
        target = REMAPPER_UX_SCREEN_HOME;
    }

    state->screen = target;
    state->selected_option = state->help_return_selection;
    state->pressed_mask = 0u;
    result->screen_changed = true;
    result->visual_changed = true;
}

void remapper_interaction_init(remapper_interaction_state_t *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->screen = REMAPPER_UX_SCREEN_LEARN_THE_KEYS;
    state->help_return_screen = REMAPPER_UX_SCREEN_HOME;
}

bool remapper_interaction_set_dynamic_options(
    remapper_interaction_state_t *state,
    const uint32_t *values,
    size_t count
)
{
    if (state == NULL || count > REMAPPER_UX_MAX_DYNAMIC_OPTIONS || (count > 0u && values == NULL)) {
        return false;
    }

    const remapper_ux_screen_definition_t *definition = remapper_ux_screen_definition(state->screen);
    if (definition == NULL || !definition->dynamic_options) {
        return false;
    }

    clear_dynamic_options(state);
    for (size_t index = 0u; index < count; ++index) {
        state->dynamic_option_values[index] = values[index];
    }
    state->dynamic_option_count = (uint8_t)count;

    if (state->selected_option >= state->dynamic_option_count) {
        state->selected_option = 0u;
    }

    return true;
}

bool remapper_interaction_is_pressed(
    const remapper_interaction_state_t *state,
    remapper_control_t control
)
{
    if (state == NULL) {
        return false;
    }

    const uint32_t bit = control_bit(control);
    return bit != 0u && (state->pressed_mask & bit) != 0u;
}

bool remapper_interaction_selection_emphasized(const remapper_interaction_state_t *state)
{
    if (state == NULL || state->locked || state->screen == REMAPPER_UX_SCREEN_HELP) {
        return false;
    }
    if (remapper_interaction_option_count(state) == 0u) {
        return false;
    }

    const uint32_t hint_mask =
        control_bit(REMAPPER_CONTROL_KEY_A) |
        control_bit(REMAPPER_CONTROL_KEY_B) |
        control_bit(REMAPPER_CONTROL_KEY_X) |
        control_bit(REMAPPER_CONTROL_KEY_Y);

    return (state->pressed_mask & hint_mask) == 0u;
}

remapper_interaction_result_t remapper_interaction_handle_event(
    remapper_interaction_state_t *state,
    remapper_input_event_t event
)
{
    remapper_interaction_result_t result = result_none();
    if (state == NULL || !control_is_valid(event.control)) {
        return result;
    }

    const uint32_t bit = control_bit(event.control);
    if (event.phase == REMAPPER_INPUT_PRESS) {
        result.consumed = true;
        if ((state->pressed_mask & bit) == 0u) {
            state->pressed_mask |= bit;
            result.visual_changed = true;
        }
        return result;
    }

    if (event.phase != REMAPPER_INPUT_RELEASE || (state->pressed_mask & bit) == 0u) {
        return result;
    }

    result.consumed = true;
    result.visual_changed = true;
    state->pressed_mask &= ~bit;

    if (state->locked) {
        state->locked = false;
        state->pressed_mask = 0u;
        result.lock_changed = true;
        navigate_home(state, &result);
        return result;
    }

    if (state->screen == REMAPPER_UX_SCREEN_HELP) {
        leave_help(state, &result);
        return result;
    }

    if (state->screen == REMAPPER_UX_SCREEN_LEARN_THE_KEYS) {
        if (event.control == REMAPPER_CONTROL_KEY_Y) {
            state->locked = true;
            state->pressed_mask = 0u;
            result.lock_changed = true;
        }
        return result;
    }

    switch (event.control) {
    case REMAPPER_CONTROL_JOY_UP:
        move_selection(state, -1, &result);
        break;
    case REMAPPER_CONTROL_JOY_DOWN:
        move_selection(state, 1, &result);
        break;
    case REMAPPER_CONTROL_JOY_LEFT:
        navigate_home(state, &result);
        break;
    case REMAPPER_CONTROL_JOY_RIGHT:
    case REMAPPER_CONTROL_JOY_PRESS:
        activate_selected_option(state, &result);
        break;
    case REMAPPER_CONTROL_KEY_A:
        activate_primary_action(state, &result);
        break;
    case REMAPPER_CONTROL_KEY_B:
        navigate_back(state, &result);
        break;
    case REMAPPER_CONTROL_KEY_X:
        enter_help(state, &result);
        break;
    case REMAPPER_CONTROL_KEY_Y:
        state->locked = true;
        state->pressed_mask = 0u;
        result.lock_changed = true;
        break;
    case REMAPPER_CONTROL_COUNT:
        break;
    }

    return result;
}
