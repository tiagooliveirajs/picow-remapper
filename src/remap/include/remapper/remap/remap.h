#ifndef REMAPPER_REMAP_REMAP_H
#define REMAPPER_REMAP_REMAP_H

#include <stdbool.h>

#include "remapper/domain/hid.h"
#include "remapper/domain/product_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    remapper_mouse_profile_config_t profile;
} remapper_remap_t;

typedef struct {
    bool has_mouse;
    bool has_keyboard;
    remapper_canonical_mouse_event_t mouse;
    remapper_canonical_keyboard_event_t keyboard;
} remapper_remap_result_t;

void remapper_remap_init(remapper_remap_t *remap);
void remapper_remap_set_profile(
    remapper_remap_t *remap,
    const remapper_mouse_profile_config_t *profile);
bool remapper_remap_process_mouse(
    const remapper_remap_t *remap,
    const remapper_canonical_mouse_event_t *input,
    remapper_remap_result_t *result);

#ifdef __cplusplus
}
#endif

#endif
