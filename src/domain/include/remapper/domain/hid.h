#ifndef REMAPPER_DOMAIN_HID_H
#define REMAPPER_DOMAIN_HID_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    REMAPPER_HID_SOURCE_INVALID = 0,
    REMAPPER_HID_SOURCE_MOUSE,
    REMAPPER_HID_SOURCE_KEYBOARD,
    REMAPPER_HID_SOURCE_COMPOSITE,
    REMAPPER_HID_SOURCE_SYNTHETIC_REMAP,
    REMAPPER_HID_SOURCE_KIND_COUNT,
} remapper_hid_source_kind_t;

typedef struct {
    uint16_t kind;
    uint16_t instance;
} remapper_hid_source_t;

static inline remapper_hid_source_t remapper_hid_source_make(
    remapper_hid_source_kind_t kind,
    uint16_t instance)
{
    const remapper_hid_source_t source = {(uint16_t)kind, instance};
    return source;
}

static inline bool remapper_hid_source_is_valid(remapper_hid_source_t source)
{
    return source.kind > (uint16_t)REMAPPER_HID_SOURCE_INVALID &&
           source.kind < (uint16_t)REMAPPER_HID_SOURCE_KIND_COUNT;
}

static inline bool remapper_hid_source_equal(remapper_hid_source_t a,
                                              remapper_hid_source_t b)
{
    return a.kind == b.kind && a.instance == b.instance;
}

typedef enum {
    REMAPPER_MOUSE_BUTTON_LEFT = 0,
    REMAPPER_MOUSE_BUTTON_RIGHT,
    REMAPPER_MOUSE_BUTTON_MIDDLE,
    REMAPPER_MOUSE_BUTTON_BACK,
    REMAPPER_MOUSE_BUTTON_FORWARD,
    REMAPPER_MOUSE_BUTTON_6,
    REMAPPER_MOUSE_BUTTON_7,
    REMAPPER_MOUSE_BUTTON_8,
    REMAPPER_MOUSE_BUTTON_COUNT,
} remapper_mouse_button_t;

typedef enum {
    REMAPPER_MOUSE_EVENT_BUTTON = 0,
    REMAPPER_MOUSE_EVENT_MOVE,
    REMAPPER_MOUSE_EVENT_WHEEL,
} remapper_mouse_event_type_t;

typedef struct {
    remapper_hid_source_t source;
    remapper_mouse_event_type_t type;
    union {
        struct {
            remapper_mouse_button_t button;
            bool pressed;
        } button;
        struct {
            int16_t dx;
            int16_t dy;
        } move;
        struct {
            int16_t vertical;
            int16_t horizontal;
        } wheel;
    } data;
} remapper_canonical_mouse_event_t;

/* Canonical keys use the USB HID Keyboard/Keypad Usage ID namespace only as
 * logical key identity. Remote report IDs, offsets and descriptor layouts stay
 * inside transport adapters and never enter this domain API. */
typedef uint8_t remapper_key_t;

enum {
    REMAPPER_KEY_A = 0x04,
    REMAPPER_KEY_ENTER = 0x28,
    REMAPPER_KEY_ESCAPE = 0x29,
    REMAPPER_KEY_SPACE = 0x2c,
};

typedef enum {
    REMAPPER_MOD_LEFT_CTRL = 0,
    REMAPPER_MOD_LEFT_SHIFT,
    REMAPPER_MOD_LEFT_ALT,
    REMAPPER_MOD_LEFT_GUI,
    REMAPPER_MOD_RIGHT_CTRL,
    REMAPPER_MOD_RIGHT_SHIFT,
    REMAPPER_MOD_RIGHT_ALT,
    REMAPPER_MOD_RIGHT_GUI,
    REMAPPER_MOD_COUNT,
} remapper_modifier_t;

typedef enum {
    REMAPPER_KEYBOARD_EVENT_KEY = 0,
    REMAPPER_KEYBOARD_EVENT_MODIFIER,
} remapper_keyboard_event_type_t;

typedef struct {
    remapper_hid_source_t source;
    remapper_keyboard_event_type_t type;
    union {
        struct {
            remapper_key_t key;
            bool pressed;
        } key;
        struct {
            remapper_modifier_t modifier;
            bool pressed;
        } modifier;
    } data;
} remapper_canonical_keyboard_event_t;

#ifdef __cplusplus
}
#endif

#endif
