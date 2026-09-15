#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "remapper/interaction/interaction.h"
#include "remapper/ux/ux_model.h"

static remapper_interaction_result_t send_event(
    remapper_interaction_state_t *state,
    remapper_control_t control,
    remapper_input_phase_t phase
)
{
    const remapper_input_event_t event = {
        .control = control,
        .phase = phase,
    };
    return remapper_interaction_handle_event(state, event);
}

static remapper_interaction_result_t click(
    remapper_interaction_state_t *state,
    remapper_control_t control
)
{
    const remapper_interaction_result_t pressed = send_event(state, control, REMAPPER_INPUT_PRESS);
    assert(pressed.consumed);
    assert(pressed.command.kind == REMAPPER_COMMAND_NONE);
    return send_event(state, control, REMAPPER_INPUT_RELEASE);
}

static void unlock_to_home(remapper_interaction_state_t *state)
{
    remapper_interaction_init(state);

    remapper_interaction_result_t result = click(state, REMAPPER_CONTROL_KEY_Y);
    assert(result.lock_changed);
    assert(state->locked);
    assert(state->screen == REMAPPER_UX_SCREEN_LEARN_THE_KEYS);

    result = click(state, REMAPPER_CONTROL_KEY_A);
    assert(result.lock_changed);
    assert(!state->locked);
    assert(state->screen == REMAPPER_UX_SCREEN_HOME);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);
}

static void move_down(remapper_interaction_state_t *state, unsigned count)
{
    for (unsigned index = 0u; index < count; ++index) {
        (void)click(state, REMAPPER_CONTROL_JOY_DOWN);
    }
}

static remapper_interaction_result_t activate_selected(remapper_interaction_state_t *state)
{
    return click(state, REMAPPER_CONTROL_JOY_RIGHT);
}

static void enter_mouse_options(remapper_interaction_state_t *state)
{
    unlock_to_home(state);
    move_down(state, 1u);
    const remapper_interaction_result_t result = activate_selected(state);
    assert(result.screen_changed);
    assert(state->screen == REMAPPER_UX_SCREEN_MOUSE_OPTIONS);
    assert(state->selected_option == 0u);
}

static void enter_other_options(remapper_interaction_state_t *state)
{
    unlock_to_home(state);
    move_down(state, 2u);
    const remapper_interaction_result_t result = activate_selected(state);
    assert(result.screen_changed);
    assert(state->screen == REMAPPER_UX_SCREEN_OTHER_OPTIONS);
    assert(state->selected_option == 0u);
}

static void test_model_metadata(void)
{
    const remapper_ux_screen_definition_t *home =
        remapper_ux_screen_definition(REMAPPER_UX_SCREEN_HOME);
    assert(home != NULL);
    assert(strcmp(home->title, "HOME") == 0);
    assert(home->option_count == 4u);

    const remapper_ux_option_t *forward =
        remapper_ux_option_at(REMAPPER_UX_SCREEN_EDIT_CUSTOM_REMAP, 3u);
    assert(forward != NULL);
    assert(strcmp(forward->label, "FORWARD") == 0);

    const remapper_ux_screen_definition_t *saved =
        remapper_ux_screen_definition(REMAPPER_UX_SCREEN_SAVED_DEVICES);
    assert(saved != NULL);
    assert(saved->dynamic_options);
    assert(saved->dynamic_has_target);
    assert(saved->dynamic_target == REMAPPER_UX_SCREEN_DEVICE_DETAILS);
}

static void test_learn_and_lock_policy(void)
{
    remapper_interaction_state_t state;
    remapper_interaction_init(&state);
    assert(state.screen == REMAPPER_UX_SCREEN_LEARN_THE_KEYS);
    assert(!state.locked);

    remapper_interaction_result_t result =
        send_event(&state, REMAPPER_CONTROL_JOY_DOWN, REMAPPER_INPUT_PRESS);
    assert(result.consumed);
    assert(result.visual_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_LEARN_THE_KEYS);
    assert(state.selected_option == 0u);
    assert(remapper_interaction_is_pressed(&state, REMAPPER_CONTROL_JOY_DOWN));

    result = send_event(&state, REMAPPER_CONTROL_JOY_DOWN, REMAPPER_INPUT_RELEASE);
    assert(result.consumed);
    assert(state.screen == REMAPPER_UX_SCREEN_LEARN_THE_KEYS);
    assert(state.selected_option == 0u);

    result = send_event(&state, REMAPPER_CONTROL_KEY_Y, REMAPPER_INPUT_PRESS);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);
    assert(!state.locked);
    assert(remapper_interaction_is_pressed(&state, REMAPPER_CONTROL_KEY_Y));

    result = send_event(&state, REMAPPER_CONTROL_KEY_Y, REMAPPER_INPUT_RELEASE);
    assert(result.lock_changed);
    assert(state.locked);

    result = send_event(&state, REMAPPER_CONTROL_KEY_A, REMAPPER_INPUT_PRESS);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);
    assert(state.locked);

    result = send_event(&state, REMAPPER_CONTROL_KEY_A, REMAPPER_INPUT_RELEASE);
    assert(result.lock_changed);
    assert(!state.locked);
    assert(state.screen == REMAPPER_UX_SCREEN_HOME);
    assert(state.selected_option == 0u);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);
}

static void test_release_driven_navigation_and_back(void)
{
    remapper_interaction_state_t state;
    unlock_to_home(&state);

    remapper_interaction_result_t result =
        send_event(&state, REMAPPER_CONTROL_JOY_DOWN, REMAPPER_INPUT_PRESS);
    assert(state.selected_option == 0u);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);

    result = send_event(&state, REMAPPER_CONTROL_JOY_DOWN, REMAPPER_INPUT_RELEASE);
    assert(result.selection_changed);
    assert(state.selected_option == 1u);

    result = send_event(&state, REMAPPER_CONTROL_JOY_RIGHT, REMAPPER_INPUT_PRESS);
    assert(state.screen == REMAPPER_UX_SCREEN_HOME);
    assert(remapper_interaction_selection_emphasized(&state));

    result = send_event(&state, REMAPPER_CONTROL_JOY_RIGHT, REMAPPER_INPUT_RELEASE);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_MOUSE_OPTIONS);
    assert(state.selected_option == 0u);

    result = send_event(&state, REMAPPER_CONTROL_KEY_A, REMAPPER_INPUT_PRESS);
    assert(state.screen == REMAPPER_UX_SCREEN_MOUSE_OPTIONS);
    assert(!remapper_interaction_selection_emphasized(&state));
    assert(result.command.kind == REMAPPER_COMMAND_NONE);

    result = send_event(&state, REMAPPER_CONTROL_KEY_A, REMAPPER_INPUT_RELEASE);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);
    assert(remapper_interaction_selection_emphasized(&state));

    result = click(&state, REMAPPER_CONTROL_KEY_B);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_HOME);
    assert(state.selected_option == 0u);

    result = click(&state, REMAPPER_CONTROL_KEY_B);
    assert(!result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_HOME);
}

static void test_help_owns_context(void)
{
    remapper_interaction_state_t state;
    unlock_to_home(&state);

    remapper_interaction_result_t result = activate_selected(&state);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_STATUS);

    result = send_event(&state, REMAPPER_CONTROL_KEY_X, REMAPPER_INPUT_PRESS);
    assert(state.screen == REMAPPER_UX_SCREEN_STATUS);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);

    result = send_event(&state, REMAPPER_CONTROL_KEY_X, REMAPPER_INPUT_RELEASE);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_HELP);
    assert(!state.locked);

    result = send_event(&state, REMAPPER_CONTROL_KEY_Y, REMAPPER_INPUT_PRESS);
    assert(state.screen == REMAPPER_UX_SCREEN_HELP);
    assert(!state.locked);

    result = send_event(&state, REMAPPER_CONTROL_KEY_Y, REMAPPER_INPUT_RELEASE);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_STATUS);
    assert(!state.locked);
}

static void test_pairing_command_on_navigation_release(void)
{
    remapper_interaction_state_t state;
    enter_mouse_options(&state);

    remapper_interaction_result_t result =
        send_event(&state, REMAPPER_CONTROL_JOY_PRESS, REMAPPER_INPUT_PRESS);
    assert(state.screen == REMAPPER_UX_SCREEN_MOUSE_OPTIONS);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);

    result = send_event(&state, REMAPPER_CONTROL_JOY_PRESS, REMAPPER_INPUT_RELEASE);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_PAIR_MOUSE);
    assert(result.command.kind == REMAPPER_COMMAND_START_PAIRING);
    assert(result.command.data.pairing_class == REMAPPER_DEVICE_CLASS_MOUSE);
}

static void test_profile_apply_on_key_a_release(void)
{
    remapper_interaction_state_t state;
    enter_mouse_options(&state);
    move_down(&state, 1u);

    remapper_interaction_result_t result = activate_selected(&state);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_PROFILE_PASSTHROUGH);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);

    result = send_event(&state, REMAPPER_CONTROL_KEY_A, REMAPPER_INPUT_PRESS);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);

    result = send_event(&state, REMAPPER_CONTROL_KEY_A, REMAPPER_INPUT_RELEASE);
    assert(result.command.kind == REMAPPER_COMMAND_APPLY_MOUSE_PROFILE);
    assert(result.command.data.profile == REMAPPER_MOUSE_PROFILE_PASSTHROUGH);
    assert(state.screen == REMAPPER_UX_SCREEN_PROFILE_PASSTHROUGH);
}

static void test_custom_draft_mapping_and_commit(void)
{
    remapper_interaction_state_t state;
    enter_mouse_options(&state);
    move_down(&state, 4u);

    remapper_interaction_result_t result = activate_selected(&state);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_EDIT_CUSTOM_REMAP);

    result = activate_selected(&state);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_MAP_LEFT);

    const uint32_t mapping_targets[] = {11u, 22u, 33u};
    assert(remapper_interaction_set_dynamic_options(
        &state,
        mapping_targets,
        sizeof(mapping_targets) / sizeof(mapping_targets[0])
    ));
    assert(remapper_interaction_option_count(&state) == 3u);

    move_down(&state, 1u);
    assert(state.selected_option == 1u);

    result = send_event(&state, REMAPPER_CONTROL_KEY_A, REMAPPER_INPUT_PRESS);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);
    assert(!remapper_interaction_selection_emphasized(&state));
    assert(state.screen == REMAPPER_UX_SCREEN_MAP_LEFT);

    result = send_event(&state, REMAPPER_CONTROL_KEY_A, REMAPPER_INPUT_RELEASE);
    assert(result.command.kind == REMAPPER_COMMAND_APPLY_CUSTOM_MAPPING);
    assert(result.command.data.custom_mapping.source == REMAPPER_MOUSE_SOURCE_LEFT);
    assert(result.command.data.custom_mapping.target_token == 22u);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_EDIT_CUSTOM_REMAP);
    assert(state.selected_option == 0u);

    result = click(&state, REMAPPER_CONTROL_KEY_A);
    assert(result.command.kind == REMAPPER_COMMAND_COMMIT_CUSTOM_REMAP);
}

static void test_saved_device_projection_and_remove_flow(void)
{
    remapper_interaction_state_t state;
    enter_other_options(&state);
    move_down(&state, 2u);

    remapper_interaction_result_t result = activate_selected(&state);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_SAVED_DEVICES);

    const uint32_t device_tokens[] = {101u, 202u};
    assert(remapper_interaction_set_dynamic_options(
        &state,
        device_tokens,
        sizeof(device_tokens) / sizeof(device_tokens[0])
    ));

    move_down(&state, 1u);
    assert(state.selected_option == 1u);

    result = activate_selected(&state);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_DEVICE_DETAILS);
    assert(result.command.kind == REMAPPER_COMMAND_SELECT_SAVED_DEVICE);
    assert(result.command.data.saved_device_token == 202u);

    result = activate_selected(&state);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_REMOVE_DEVICE);

    result = send_event(&state, REMAPPER_CONTROL_KEY_A, REMAPPER_INPUT_PRESS);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);

    result = send_event(&state, REMAPPER_CONTROL_KEY_A, REMAPPER_INPUT_RELEASE);
    assert(result.command.kind == REMAPPER_COMMAND_REMOVE_SELECTED_DEVICE);

    result = click(&state, REMAPPER_CONTROL_KEY_B);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_DEVICE_DETAILS);
}

static void test_home_left_clamping_and_unmatched_release(void)
{
    remapper_interaction_state_t state;
    enter_mouse_options(&state);
    move_down(&state, 4u);
    assert(state.selected_option == 4u);

    remapper_interaction_result_t result = click(&state, REMAPPER_CONTROL_JOY_DOWN);
    assert(!result.selection_changed);
    assert(state.selected_option == 4u);

    result = click(&state, REMAPPER_CONTROL_JOY_LEFT);
    assert(result.screen_changed);
    assert(state.screen == REMAPPER_UX_SCREEN_HOME);
    assert(state.selected_option == 0u);

    result = click(&state, REMAPPER_CONTROL_JOY_UP);
    assert(!result.selection_changed);
    assert(state.selected_option == 0u);

    result = send_event(&state, REMAPPER_CONTROL_JOY_RIGHT, REMAPPER_INPUT_RELEASE);
    assert(!result.consumed);
    assert(!result.screen_changed);
    assert(result.command.kind == REMAPPER_COMMAND_NONE);
    assert(state.screen == REMAPPER_UX_SCREEN_HOME);
}

int main(void)
{
    test_model_metadata();
    test_learn_and_lock_policy();
    test_release_driven_navigation_and_back();
    test_help_owns_context();
    test_pairing_command_on_navigation_release();
    test_profile_apply_on_key_a_release();
    test_custom_draft_mapping_and_commit();
    test_saved_device_projection_and_remove_flow();
    test_home_left_clamping_and_unmatched_release();
    return 0;
}
