#include "remapper/ux/ux_model.h"

#define ARRAY_COUNT(values) (sizeof(values) / sizeof((values)[0]))

static const remapper_ux_option_t home_options[] = {
    {
        .label = "STATUS",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_STATUS,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
    {
        .label = "MOUSE OPTIONS",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_MOUSE_OPTIONS,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
    {
        .label = "OTHER OPTIONS",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_OTHER_OPTIONS,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
    {
        .label = "LEARN THE KEYS",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_LEARN_THE_KEYS,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
};

static const remapper_ux_option_t mouse_options[] = {
    {
        .label = "PAIR MOUSE",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_PAIR_MOUSE,
        .command = {
            .kind = REMAPPER_COMMAND_START_PAIRING,
            .data = {.pairing_class = REMAPPER_DEVICE_CLASS_MOUSE},
        },
    },
    {
        .label = "PASSTHROUGH",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_PROFILE_PASSTHROUGH,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
    {
        .label = "DEFAULT REMAP",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_PROFILE_DEFAULT_REMAP,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
    {
        .label = "ESCAPE REMAP",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_PROFILE_ESCAPE_REMAP,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
    {
        .label = "CUSTOM REMAP",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_EDIT_CUSTOM_REMAP,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
};

static const remapper_ux_option_t edit_custom_options[] = {
    {
        .label = "LEFT",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_MAP_LEFT,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
    {
        .label = "RIGHT",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_MAP_RIGHT,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
    {
        .label = "MIDDLE",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_MAP_MIDDLE,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
    {
        .label = "FORWARD",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_MAP_FORWARD,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
    {
        .label = "BACKWARD",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_MAP_BACKWARD,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
};

static const remapper_ux_option_t other_options[] = {
    {
        .label = "PAIR KEYBOARD",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_PAIR_KEYBOARD,
        .command = {
            .kind = REMAPPER_COMMAND_START_PAIRING,
            .data = {.pairing_class = REMAPPER_DEVICE_CLASS_KEYBOARD},
        },
    },
    {
        .label = "PAIR COMPOSITE",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_PAIR_COMPOSITE,
        .command = {
            .kind = REMAPPER_COMMAND_START_PAIRING,
            .data = {.pairing_class = REMAPPER_DEVICE_CLASS_COMPOSITE},
        },
    },
    {
        .label = "SAVED DEVICES",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_SAVED_DEVICES,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
};

static const remapper_ux_option_t device_details_options[] = {
    {
        .label = "REMOVE DEVICE",
        .has_target = true,
        .target = REMAPPER_UX_SCREEN_REMOVE_DEVICE,
        .command = {.kind = REMAPPER_COMMAND_NONE},
    },
};

static const remapper_ux_screen_definition_t screen_definitions[REMAPPER_UX_SCREEN_COUNT] = {
    [REMAPPER_UX_SCREEN_LEARN_THE_KEYS] = {
        .id = REMAPPER_UX_SCREEN_LEARN_THE_KEYS,
        .title = "LEARN THE KEYS",
    },
    [REMAPPER_UX_SCREEN_HOME] = {
        .id = REMAPPER_UX_SCREEN_HOME,
        .title = "HOME",
        .options = home_options,
        .option_count = ARRAY_COUNT(home_options),
    },
    [REMAPPER_UX_SCREEN_STATUS] = {
        .id = REMAPPER_UX_SCREEN_STATUS,
        .title = "STATUS",
        .has_help = true,
    },
    [REMAPPER_UX_SCREEN_MOUSE_OPTIONS] = {
        .id = REMAPPER_UX_SCREEN_MOUSE_OPTIONS,
        .title = "MOUSE OPTIONS",
        .options = mouse_options,
        .option_count = ARRAY_COUNT(mouse_options),
    },
    [REMAPPER_UX_SCREEN_PAIR_MOUSE] = {
        .id = REMAPPER_UX_SCREEN_PAIR_MOUSE,
        .title = "PAIR MOUSE",
        .has_help = true,
    },
    [REMAPPER_UX_SCREEN_PROFILE_PASSTHROUGH] = {
        .id = REMAPPER_UX_SCREEN_PROFILE_PASSTHROUGH,
        .title = "PASSTHROUGH",
        .primary_command = {
            .kind = REMAPPER_COMMAND_APPLY_MOUSE_PROFILE,
            .data = {.profile = REMAPPER_MOUSE_PROFILE_PASSTHROUGH},
        },
    },
    [REMAPPER_UX_SCREEN_PROFILE_DEFAULT_REMAP] = {
        .id = REMAPPER_UX_SCREEN_PROFILE_DEFAULT_REMAP,
        .title = "DEFAULT REMAP",
        .primary_command = {
            .kind = REMAPPER_COMMAND_APPLY_MOUSE_PROFILE,
            .data = {.profile = REMAPPER_MOUSE_PROFILE_DEFAULT_REMAP},
        },
    },
    [REMAPPER_UX_SCREEN_PROFILE_ESCAPE_REMAP] = {
        .id = REMAPPER_UX_SCREEN_PROFILE_ESCAPE_REMAP,
        .title = "ESCAPE REMAP",
        .primary_command = {
            .kind = REMAPPER_COMMAND_APPLY_MOUSE_PROFILE,
            .data = {.profile = REMAPPER_MOUSE_PROFILE_ESCAPE_REMAP},
        },
    },
    [REMAPPER_UX_SCREEN_EDIT_CUSTOM_REMAP] = {
        .id = REMAPPER_UX_SCREEN_EDIT_CUSTOM_REMAP,
        .title = "EDIT CUSTOM REMAP",
        .options = edit_custom_options,
        .option_count = ARRAY_COUNT(edit_custom_options),
        .primary_command = {.kind = REMAPPER_COMMAND_COMMIT_CUSTOM_REMAP},
    },
    [REMAPPER_UX_SCREEN_MAP_LEFT] = {
        .id = REMAPPER_UX_SCREEN_MAP_LEFT,
        .title = "MAP LEFT",
        .dynamic_options = true,
        .primary_command = {
            .kind = REMAPPER_COMMAND_APPLY_CUSTOM_MAPPING,
            .data = {.custom_mapping = {.source = REMAPPER_MOUSE_SOURCE_LEFT, .target_token = 0u}},
        },
        .primary_command_uses_selected_value = true,
    },
    [REMAPPER_UX_SCREEN_MAP_RIGHT] = {
        .id = REMAPPER_UX_SCREEN_MAP_RIGHT,
        .title = "MAP RIGHT",
        .dynamic_options = true,
        .primary_command = {
            .kind = REMAPPER_COMMAND_APPLY_CUSTOM_MAPPING,
            .data = {.custom_mapping = {.source = REMAPPER_MOUSE_SOURCE_RIGHT, .target_token = 0u}},
        },
        .primary_command_uses_selected_value = true,
    },
    [REMAPPER_UX_SCREEN_MAP_MIDDLE] = {
        .id = REMAPPER_UX_SCREEN_MAP_MIDDLE,
        .title = "MAP MIDDLE",
        .dynamic_options = true,
        .primary_command = {
            .kind = REMAPPER_COMMAND_APPLY_CUSTOM_MAPPING,
            .data = {.custom_mapping = {.source = REMAPPER_MOUSE_SOURCE_MIDDLE, .target_token = 0u}},
        },
        .primary_command_uses_selected_value = true,
    },
    [REMAPPER_UX_SCREEN_MAP_FORWARD] = {
        .id = REMAPPER_UX_SCREEN_MAP_FORWARD,
        .title = "MAP FORWARD",
        .dynamic_options = true,
        .primary_command = {
            .kind = REMAPPER_COMMAND_APPLY_CUSTOM_MAPPING,
            .data = {.custom_mapping = {.source = REMAPPER_MOUSE_SOURCE_FORWARD, .target_token = 0u}},
        },
        .primary_command_uses_selected_value = true,
    },
    [REMAPPER_UX_SCREEN_MAP_BACKWARD] = {
        .id = REMAPPER_UX_SCREEN_MAP_BACKWARD,
        .title = "MAP BACKWARD",
        .dynamic_options = true,
        .primary_command = {
            .kind = REMAPPER_COMMAND_APPLY_CUSTOM_MAPPING,
            .data = {.custom_mapping = {.source = REMAPPER_MOUSE_SOURCE_BACKWARD, .target_token = 0u}},
        },
        .primary_command_uses_selected_value = true,
    },
    [REMAPPER_UX_SCREEN_OTHER_OPTIONS] = {
        .id = REMAPPER_UX_SCREEN_OTHER_OPTIONS,
        .title = "OTHER OPTIONS",
        .options = other_options,
        .option_count = ARRAY_COUNT(other_options),
    },
    [REMAPPER_UX_SCREEN_PAIR_KEYBOARD] = {
        .id = REMAPPER_UX_SCREEN_PAIR_KEYBOARD,
        .title = "PAIR KEYBOARD",
        .has_help = true,
    },
    [REMAPPER_UX_SCREEN_PAIR_COMPOSITE] = {
        .id = REMAPPER_UX_SCREEN_PAIR_COMPOSITE,
        .title = "PAIR COMPOSITE",
        .has_help = true,
    },
    [REMAPPER_UX_SCREEN_SAVED_DEVICES] = {
        .id = REMAPPER_UX_SCREEN_SAVED_DEVICES,
        .title = "SAVED DEVICES",
        .dynamic_options = true,
        .dynamic_has_target = true,
        .dynamic_target = REMAPPER_UX_SCREEN_DEVICE_DETAILS,
        .dynamic_command = {.kind = REMAPPER_COMMAND_SELECT_SAVED_DEVICE},
        .dynamic_command_uses_selected_value = true,
    },
    [REMAPPER_UX_SCREEN_DEVICE_DETAILS] = {
        .id = REMAPPER_UX_SCREEN_DEVICE_DETAILS,
        .title = "DEVICE DETAILS",
        .options = device_details_options,
        .option_count = ARRAY_COUNT(device_details_options),
    },
    [REMAPPER_UX_SCREEN_REMOVE_DEVICE] = {
        .id = REMAPPER_UX_SCREEN_REMOVE_DEVICE,
        .title = "REMOVE DEVICE",
        .primary_command = {.kind = REMAPPER_COMMAND_REMOVE_SELECTED_DEVICE},
    },
    [REMAPPER_UX_SCREEN_HELP] = {
        .id = REMAPPER_UX_SCREEN_HELP,
        .title = "HELP",
    },
};

bool remapper_ux_screen_is_valid(remapper_ux_screen_t screen)
{
    return screen >= REMAPPER_UX_SCREEN_LEARN_THE_KEYS && screen < REMAPPER_UX_SCREEN_COUNT;
}

const remapper_ux_screen_definition_t *remapper_ux_screen_definition(remapper_ux_screen_t screen)
{
    if (!remapper_ux_screen_is_valid(screen)) {
        return NULL;
    }

    return &screen_definitions[screen];
}

const remapper_ux_option_t *remapper_ux_option_at(remapper_ux_screen_t screen, size_t index)
{
    const remapper_ux_screen_definition_t *definition = remapper_ux_screen_definition(screen);
    if (definition == NULL || definition->dynamic_options || index >= definition->option_count) {
        return NULL;
    }

    return &definition->options[index];
}
