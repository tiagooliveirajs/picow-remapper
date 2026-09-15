#ifndef REMAPPER_PROFILES_PROFILES_H
#define REMAPPER_PROFILES_PROFILES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "remapper/domain/product_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REMAPPER_PROFILE_SOURCE_COUNT REMAPPER_MOUSE_SOURCE_COUNT
#define REMAPPER_PROFILE_SERIALIZED_SIZE 8u

typedef struct {
    remapper_mouse_profile_kind_t active_kind;
    remapper_mouse_target_t custom_targets[REMAPPER_PROFILE_SOURCE_COUNT];
    remapper_mouse_target_t draft_targets[REMAPPER_PROFILE_SOURCE_COUNT];
    bool draft_valid;
} remapper_profiles_t;

void remapper_profiles_init(remapper_profiles_t *profiles);
void remapper_profiles_configure_active(
    const remapper_profiles_t *profiles,
    remapper_mouse_profile_config_t *config);
bool remapper_profiles_build_preset(
    remapper_mouse_profile_kind_t kind,
    const remapper_profiles_t *profiles,
    remapper_mouse_profile_config_t *config);
void remapper_profiles_activate(
    remapper_profiles_t *profiles,
    const remapper_mouse_profile_config_t *config);

bool remapper_profiles_draft_set(
    remapper_profiles_t *profiles,
    remapper_mouse_source_button_t source,
    remapper_mouse_target_t target);
bool remapper_profiles_custom_candidate(
    const remapper_profiles_t *profiles,
    remapper_mouse_profile_config_t *config);

remapper_mouse_target_t remapper_profiles_target_for_source(
    const remapper_mouse_profile_config_t *config,
    remapper_mouse_source_button_t source);
bool remapper_profiles_requires_forward_held_fix(
    const remapper_mouse_profile_config_t *config);

bool remapper_profiles_serialize(
    const remapper_profiles_t *profiles,
    uint8_t out[REMAPPER_PROFILE_SERIALIZED_SIZE]);
bool remapper_profiles_restore(
    remapper_profiles_t *profiles,
    const uint8_t data[REMAPPER_PROFILE_SERIALIZED_SIZE]);

const char *remapper_profiles_target_name(remapper_mouse_target_t target);

#ifdef __cplusplus
}
#endif

#endif
