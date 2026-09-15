#include "remapper/profiles/profiles.h"

#include <string.h>

#define PROFILE_SCHEMA_VERSION 1u

static bool source_valid(remapper_mouse_source_button_t source)
{
    return source >= REMAPPER_MOUSE_SOURCE_LEFT &&
        source <= REMAPPER_MOUSE_SOURCE_BACKWARD;
}

static bool custom_target_valid(remapper_mouse_target_t target)
{
    return target >= REMAPPER_MOUSE_TARGET_LEFT &&
        target <= REMAPPER_MOUSE_TARGET_BACKWARD;
}

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

static void identity_targets(remapper_mouse_target_t targets[REMAPPER_PROFILE_SOURCE_COUNT])
{
    for (unsigned int source = 0u; source < REMAPPER_PROFILE_SOURCE_COUNT; ++source) {
        targets[source] = identity_target((remapper_mouse_source_button_t)source);
    }
}

void remapper_profiles_init(remapper_profiles_t *profiles)
{
    if (profiles == NULL) return;
    memset(profiles, 0, sizeof(*profiles));
    profiles->active_kind = REMAPPER_MOUSE_PROFILE_PASSTHROUGH;
    identity_targets(profiles->custom_targets);
    identity_targets(profiles->draft_targets);
}

static void fill_targets(
    remapper_mouse_profile_kind_t kind,
    const remapper_profiles_t *profiles,
    remapper_mouse_target_t targets[REMAPPER_PROFILE_SOURCE_COUNT])
{
    identity_targets(targets);

    switch (kind) {
    case REMAPPER_MOUSE_PROFILE_DEFAULT_REMAP:
        targets[REMAPPER_MOUSE_SOURCE_LEFT] = REMAPPER_MOUSE_TARGET_ESCAPE;
        targets[REMAPPER_MOUSE_SOURCE_RIGHT] = REMAPPER_MOUSE_TARGET_BACKWARD;
        targets[REMAPPER_MOUSE_SOURCE_MIDDLE] = REMAPPER_MOUSE_TARGET_FORWARD;
        targets[REMAPPER_MOUSE_SOURCE_FORWARD] = REMAPPER_MOUSE_TARGET_LEFT;
        targets[REMAPPER_MOUSE_SOURCE_BACKWARD] = REMAPPER_MOUSE_TARGET_RIGHT;
        break;
    case REMAPPER_MOUSE_PROFILE_ESCAPE_REMAP:
        targets[REMAPPER_MOUSE_SOURCE_LEFT] = REMAPPER_MOUSE_TARGET_ESCAPE;
        break;
    case REMAPPER_MOUSE_PROFILE_CUSTOM_REMAP:
        if (profiles != NULL) {
            memcpy(targets, profiles->custom_targets,
                   sizeof(profiles->custom_targets));
        }
        break;
    case REMAPPER_MOUSE_PROFILE_PASSTHROUGH:
    default:
        break;
    }
}

void remapper_profiles_configure_active(
    const remapper_profiles_t *profiles,
    remapper_mouse_profile_config_t *config)
{
    if (profiles == NULL || config == NULL) return;
    config->kind = profiles->active_kind;
    fill_targets(config->kind, profiles, config->targets);
}

bool remapper_profiles_build_preset(
    remapper_mouse_profile_kind_t kind,
    const remapper_profiles_t *profiles,
    remapper_mouse_profile_config_t *config)
{
    if (config == NULL || kind < REMAPPER_MOUSE_PROFILE_PASSTHROUGH ||
        kind > REMAPPER_MOUSE_PROFILE_ESCAPE_REMAP) {
        return false;
    }
    config->kind = kind;
    fill_targets(kind, profiles, config->targets);
    return true;
}

void remapper_profiles_activate(
    remapper_profiles_t *profiles,
    const remapper_mouse_profile_config_t *config)
{
    if (profiles == NULL || config == NULL ||
        config->kind < REMAPPER_MOUSE_PROFILE_PASSTHROUGH ||
        config->kind > REMAPPER_MOUSE_PROFILE_CUSTOM_REMAP) {
        return;
    }

    if (config->kind == REMAPPER_MOUSE_PROFILE_CUSTOM_REMAP) {
        for (unsigned int index = 0u; index < REMAPPER_PROFILE_SOURCE_COUNT; ++index) {
            if (!custom_target_valid(config->targets[index])) return;
        }
        memcpy(profiles->custom_targets, config->targets,
               sizeof(profiles->custom_targets));
    }
    profiles->active_kind = config->kind;
    profiles->draft_valid = false;
}

bool remapper_profiles_draft_set(
    remapper_profiles_t *profiles,
    remapper_mouse_source_button_t source,
    remapper_mouse_target_t target)
{
    if (profiles == NULL || !source_valid(source) || !custom_target_valid(target)) {
        return false;
    }

    if (!profiles->draft_valid) {
        memcpy(profiles->draft_targets, profiles->custom_targets,
               sizeof(profiles->draft_targets));
        profiles->draft_valid = true;
    }
    profiles->draft_targets[source] = target;
    return true;
}

bool remapper_profiles_custom_candidate(
    const remapper_profiles_t *profiles,
    remapper_mouse_profile_config_t *config)
{
    if (profiles == NULL || config == NULL) return false;
    config->kind = REMAPPER_MOUSE_PROFILE_CUSTOM_REMAP;
    memcpy(config->targets,
           profiles->draft_valid ? profiles->draft_targets : profiles->custom_targets,
           sizeof(config->targets));
    return true;
}

remapper_mouse_target_t remapper_profiles_target_for_source(
    const remapper_mouse_profile_config_t *config,
    remapper_mouse_source_button_t source)
{
    if (config == NULL || !source_valid(source)) return REMAPPER_MOUSE_TARGET_LEFT;
    return config->targets[source];
}

bool remapper_profiles_requires_forward_held_fix(
    const remapper_mouse_profile_config_t *config)
{
    return config != NULL &&
        remapper_profiles_target_for_source(config, REMAPPER_MOUSE_SOURCE_FORWARD) !=
            REMAPPER_MOUSE_TARGET_FORWARD;
}

static uint8_t serialized_checksum(const uint8_t *data)
{
    uint8_t value = 0x5au;
    for (size_t index = 0u; index < REMAPPER_PROFILE_SERIALIZED_SIZE - 1u; ++index) {
        value = (uint8_t)((value << 1u) | (value >> 7u));
        value ^= data[index];
    }
    return value;
}

bool remapper_profiles_serialize(
    const remapper_profiles_t *profiles,
    uint8_t out[REMAPPER_PROFILE_SERIALIZED_SIZE])
{
    if (profiles == NULL || out == NULL ||
        profiles->active_kind < REMAPPER_MOUSE_PROFILE_PASSTHROUGH ||
        profiles->active_kind > REMAPPER_MOUSE_PROFILE_CUSTOM_REMAP) {
        return false;
    }

    out[0] = PROFILE_SCHEMA_VERSION;
    out[1] = (uint8_t)profiles->active_kind;
    for (size_t index = 0u; index < REMAPPER_PROFILE_SOURCE_COUNT; ++index) {
        if (!custom_target_valid(profiles->custom_targets[index])) return false;
        out[index + 2u] = (uint8_t)profiles->custom_targets[index];
    }
    out[REMAPPER_PROFILE_SERIALIZED_SIZE - 1u] = serialized_checksum(out);
    return true;
}

bool remapper_profiles_restore(
    remapper_profiles_t *profiles,
    const uint8_t data[REMAPPER_PROFILE_SERIALIZED_SIZE])
{
    if (profiles == NULL || data == NULL || data[0] != PROFILE_SCHEMA_VERSION ||
        data[1] > (uint8_t)REMAPPER_MOUSE_PROFILE_CUSTOM_REMAP ||
        data[REMAPPER_PROFILE_SERIALIZED_SIZE - 1u] != serialized_checksum(data)) {
        return false;
    }

    remapper_profiles_t candidate;
    remapper_profiles_init(&candidate);
    candidate.active_kind = (remapper_mouse_profile_kind_t)data[1];
    for (size_t index = 0u; index < REMAPPER_PROFILE_SOURCE_COUNT; ++index) {
        const remapper_mouse_target_t target =
            (remapper_mouse_target_t)data[index + 2u];
        if (!custom_target_valid(target)) return false;
        candidate.custom_targets[index] = target;
        candidate.draft_targets[index] = target;
    }
    *profiles = candidate;
    return true;
}

const char *remapper_profiles_target_name(remapper_mouse_target_t target)
{
    switch (target) {
    case REMAPPER_MOUSE_TARGET_LEFT: return "LEFT";
    case REMAPPER_MOUSE_TARGET_RIGHT: return "RIGHT";
    case REMAPPER_MOUSE_TARGET_MIDDLE: return "MIDDLE";
    case REMAPPER_MOUSE_TARGET_FORWARD: return "FORWARD";
    case REMAPPER_MOUSE_TARGET_BACKWARD: return "BACKWARD";
    case REMAPPER_MOUSE_TARGET_ESCAPE: return "ESCAPE";
    case REMAPPER_MOUSE_TARGET_COUNT: break;
    }
    return "?";
}
