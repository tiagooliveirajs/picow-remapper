#ifndef REMAPPER_DOMAIN_PRODUCT_TYPES_H
#define REMAPPER_DOMAIN_PRODUCT_TYPES_H

/*
 * Product-level types used by the host-testable UX/application command layer.
 * Transport, HID report and Pico-specific types deliberately do not belong here.
 */
#define REMAPPER_MOUSE_SOURCE_COUNT 5u

typedef enum {
    REMAPPER_DEVICE_CLASS_NONE = 0,
    REMAPPER_DEVICE_CLASS_MOUSE,
    REMAPPER_DEVICE_CLASS_KEYBOARD,
    REMAPPER_DEVICE_CLASS_COMPOSITE,
} remapper_device_class_t;

typedef enum {
    REMAPPER_MOUSE_PROFILE_PASSTHROUGH = 0,
    REMAPPER_MOUSE_PROFILE_DEFAULT_REMAP,
    REMAPPER_MOUSE_PROFILE_ESCAPE_REMAP,
    REMAPPER_MOUSE_PROFILE_CUSTOM_REMAP,
} remapper_mouse_profile_kind_t;

typedef enum {
    REMAPPER_MOUSE_SOURCE_LEFT = 0,
    REMAPPER_MOUSE_SOURCE_RIGHT,
    REMAPPER_MOUSE_SOURCE_MIDDLE,
    REMAPPER_MOUSE_SOURCE_FORWARD,
    REMAPPER_MOUSE_SOURCE_BACKWARD,
} remapper_mouse_source_button_t;

/* Stable product-level mapping tokens used by Custom Remap dynamic options. */
typedef enum {
    REMAPPER_MOUSE_TARGET_LEFT = 0,
    REMAPPER_MOUSE_TARGET_RIGHT,
    REMAPPER_MOUSE_TARGET_MIDDLE,
    REMAPPER_MOUSE_TARGET_FORWARD,
    REMAPPER_MOUSE_TARGET_BACKWARD,
    REMAPPER_MOUSE_TARGET_ESCAPE,
    REMAPPER_MOUSE_TARGET_COUNT,
} remapper_mouse_target_t;

typedef struct {
    remapper_mouse_profile_kind_t kind;
    remapper_mouse_target_t targets[REMAPPER_MOUSE_SOURCE_COUNT];
} remapper_mouse_profile_config_t;

#endif
