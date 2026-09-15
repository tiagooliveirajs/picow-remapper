#ifndef REMAPPER_DOMAIN_COMMAND_H
#define REMAPPER_DOMAIN_COMMAND_H

#include <stdint.h>

#include "remapper/domain/product_types.h"

typedef enum {
    REMAPPER_COMMAND_NONE = 0,
    REMAPPER_COMMAND_START_PAIRING,
    REMAPPER_COMMAND_APPLY_MOUSE_PROFILE,
    REMAPPER_COMMAND_COMMIT_CUSTOM_REMAP,
    REMAPPER_COMMAND_APPLY_CUSTOM_MAPPING,
    REMAPPER_COMMAND_SELECT_SAVED_DEVICE,
    REMAPPER_COMMAND_REMOVE_SELECTED_DEVICE,
} remapper_command_kind_t;

typedef struct {
    remapper_command_kind_t kind;
    union {
        remapper_device_class_t pairing_class;
        remapper_mouse_profile_kind_t profile;
        struct {
            remapper_mouse_source_button_t source;
            uint32_t target_token;
        } custom_mapping;
        uint32_t saved_device_token;
    } data;
} remapper_command_t;

static inline remapper_command_t remapper_command_none(void)
{
    remapper_command_t command = {0};
    command.kind = REMAPPER_COMMAND_NONE;
    return command;
}

#endif
