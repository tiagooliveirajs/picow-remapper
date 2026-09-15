#ifndef REMAPPER_USB_HID_USB_HID_H
#define REMAPPER_USB_HID_USB_HID_H

#include <stdbool.h>
#include <stdint.h>

#define REMAPPER_USB_VID UINT16_C(0xcafe)
#define REMAPPER_USB_PID UINT16_C(0x4010)
#define REMAPPER_USB_BCD_DEVICE UINT16_C(0x0100)
#define REMAPPER_USB_HID_INTERFACE_COUNT 2u
#define REMAPPER_USB_HID_MOUSE_INTERFACE 0u
#define REMAPPER_USB_HID_KEYBOARD_INTERFACE 1u
#define REMAPPER_USB_HID_KEYCODE_COUNT 6u
#define REMAPPER_USB_MANUFACTURER "PicoW Remapper"
#define REMAPPER_USB_PRODUCT "PicoW Remapper Mouse + Keyboard"

typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint16_t bcd_device;
    uint8_t interface_count;
    uint8_t mouse_interface;
    uint8_t keyboard_interface;
} remapper_usb_hid_identity_t;

typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;
    int8_t pan;
} remapper_usb_mouse_report_t;

typedef struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keycodes[REMAPPER_USB_HID_KEYCODE_COUNT];
} remapper_usb_keyboard_report_t;

const remapper_usb_hid_identity_t *remapper_usb_hid_identity(void);
void remapper_usb_hid_build_mouse_report(
    remapper_usb_mouse_report_t *report,
    uint8_t buttons,
    int8_t x,
    int8_t y,
    int8_t wheel,
    int8_t pan);
void remapper_usb_hid_build_keyboard_report(
    remapper_usb_keyboard_report_t *report,
    uint8_t modifiers,
    const uint8_t keycodes[REMAPPER_USB_HID_KEYCODE_COUNT]);

bool remapper_usb_hid_pico_init(void);
void remapper_usb_hid_pico_task(void);
bool remapper_usb_hid_pico_mounted(void);
bool remapper_usb_hid_pico_send_mouse(const remapper_usb_mouse_report_t *report);
bool remapper_usb_hid_pico_send_keyboard(const remapper_usb_keyboard_report_t *report);

#endif
