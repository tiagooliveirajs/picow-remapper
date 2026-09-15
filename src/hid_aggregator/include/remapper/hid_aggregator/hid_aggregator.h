#ifndef REMAPPER_HID_AGGREGATOR_HID_AGGREGATOR_H
#define REMAPPER_HID_AGGREGATOR_HID_AGGREGATOR_H

#include <stdbool.h>
#include <stdint.h>

#include "remapper/domain/hid.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REMAPPER_HID_AGGREGATOR_MAX_SOURCES 16u
#define REMAPPER_HID_KEY_COUNT 256u
#define REMAPPER_HID_KEY_BITMAP_BYTES (REMAPPER_HID_KEY_COUNT / 8u)

typedef struct {
    bool active;
    remapper_hid_source_t id;
    uint8_t mouse_buttons;
    uint8_t key_bitmap[REMAPPER_HID_KEY_BITMAP_BYTES];
    uint8_t modifiers;
} remapper_hid_source_state_t;

typedef struct {
    remapper_hid_source_state_t sources[REMAPPER_HID_AGGREGATOR_MAX_SOURCES];
    uint8_t mouse_button_refs[REMAPPER_MOUSE_BUTTON_COUNT];
    uint8_t key_refs[REMAPPER_HID_KEY_COUNT];
    uint8_t modifier_refs[REMAPPER_MOD_COUNT];
    int32_t pending_dx;
    int32_t pending_dy;
    int32_t pending_wheel_vertical;
    int32_t pending_wheel_horizontal;
} remapper_hid_aggregator_t;

typedef struct {
    uint8_t mouse_buttons;
    int32_t dx;
    int32_t dy;
    int32_t wheel_vertical;
    int32_t wheel_horizontal;
    uint8_t key_bitmap[REMAPPER_HID_KEY_BITMAP_BYTES];
    uint8_t modifiers;
} remapper_hid_output_state_t;

void remapper_hid_aggregator_init(remapper_hid_aggregator_t *aggregator);

bool remapper_hid_aggregator_apply_mouse(
    remapper_hid_aggregator_t *aggregator,
    const remapper_canonical_mouse_event_t *event);

bool remapper_hid_aggregator_apply_keyboard(
    remapper_hid_aggregator_t *aggregator,
    const remapper_canonical_keyboard_event_t *event);

/* Release only the persistent ownership held by this source. */
bool remapper_hid_aggregator_release_source(
    remapper_hid_aggregator_t *aggregator,
    remapper_hid_source_t source);

/* Snapshot aggregate state without consuming relative deltas. */
void remapper_hid_aggregator_snapshot(
    const remapper_hid_aggregator_t *aggregator,
    remapper_hid_output_state_t *out_state);

/* Return aggregate state and consume only relative motion/wheel deltas. */
void remapper_hid_aggregator_take_output(
    remapper_hid_aggregator_t *aggregator,
    remapper_hid_output_state_t *out_state);

bool remapper_hid_output_mouse_button_is_down(
    const remapper_hid_output_state_t *state,
    remapper_mouse_button_t button);

bool remapper_hid_output_key_is_down(
    const remapper_hid_output_state_t *state,
    remapper_key_t key);

#ifdef __cplusplus
}
#endif

#endif
