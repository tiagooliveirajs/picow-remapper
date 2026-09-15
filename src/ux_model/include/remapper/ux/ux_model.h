#ifndef REMAPPER_UX_MODEL_H
#define REMAPPER_UX_MODEL_H

#include <stdbool.h>
#include <stddef.h>

#include "remapper/domain/command.h"

#define REMAPPER_UX_MAX_DYNAMIC_OPTIONS 8u

typedef enum {
    REMAPPER_UX_SCREEN_LEARN_THE_KEYS = 0,
    REMAPPER_UX_SCREEN_HOME,
    REMAPPER_UX_SCREEN_STATUS,
    REMAPPER_UX_SCREEN_MOUSE_OPTIONS,
    REMAPPER_UX_SCREEN_PAIR_MOUSE,
    REMAPPER_UX_SCREEN_PROFILE_PASSTHROUGH,
    REMAPPER_UX_SCREEN_PROFILE_DEFAULT_REMAP,
    REMAPPER_UX_SCREEN_PROFILE_ESCAPE_REMAP,
    REMAPPER_UX_SCREEN_EDIT_CUSTOM_REMAP,
    REMAPPER_UX_SCREEN_MAP_LEFT,
    REMAPPER_UX_SCREEN_MAP_RIGHT,
    REMAPPER_UX_SCREEN_MAP_MIDDLE,
    REMAPPER_UX_SCREEN_MAP_FORWARD,
    REMAPPER_UX_SCREEN_MAP_BACKWARD,
    REMAPPER_UX_SCREEN_OTHER_OPTIONS,
    REMAPPER_UX_SCREEN_PAIR_KEYBOARD,
    REMAPPER_UX_SCREEN_PAIR_COMPOSITE,
    REMAPPER_UX_SCREEN_SAVED_DEVICES,
    REMAPPER_UX_SCREEN_DEVICE_DETAILS,
    REMAPPER_UX_SCREEN_REMOVE_DEVICE,
    REMAPPER_UX_SCREEN_HELP,
    REMAPPER_UX_SCREEN_COUNT,
} remapper_ux_screen_t;

typedef struct {
    const char *label;
    bool has_target;
    remapper_ux_screen_t target;
    remapper_command_t command;
} remapper_ux_option_t;

typedef struct {
    remapper_ux_screen_t id;
    const char *title;
    const remapper_ux_option_t *options;
    size_t option_count;
    bool dynamic_options;
    bool has_help;

    bool dynamic_has_target;
    remapper_ux_screen_t dynamic_target;
    remapper_command_t dynamic_command;
    bool dynamic_command_uses_selected_value;

    remapper_command_t primary_command;
    bool primary_command_uses_selected_value;
} remapper_ux_screen_definition_t;

bool remapper_ux_screen_is_valid(remapper_ux_screen_t screen);
const remapper_ux_screen_definition_t *remapper_ux_screen_definition(remapper_ux_screen_t screen);
const remapper_ux_option_t *remapper_ux_option_at(remapper_ux_screen_t screen, size_t index);

#endif
