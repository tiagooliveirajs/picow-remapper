#ifndef REMAPPER_DOMAIN_PRODUCT_TYPES_H
#define REMAPPER_DOMAIN_PRODUCT_TYPES_H

/*
 * Product-level types used by the host-testable UX/application command layer.
 * Transport, HID report and Pico-specific types deliberately do not belong here.
 */
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

#endif
