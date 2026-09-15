#include "remapper/usb_hid/usb_hid.h"

#include <stddef.h>

_Static_assert(sizeof(remapper_usb_mouse_report_t) == 5u, "fixed USB mouse report must remain five bytes");
_Static_assert(sizeof(remapper_usb_keyboard_report_t) == 8u, "fixed USB keyboard report must remain eight bytes");

static const remapper_usb_hid_identity_t k_identity = {
    .vid = REMAPPER_USB_VID,
    .pid = REMAPPER_USB_PID,
    .bcd_device = REMAPPER_USB_BCD_DEVICE,
    .interface_count = REMAPPER_USB_HID_INTERFACE_COUNT,
    .mouse_interface = REMAPPER_USB_HID_MOUSE_INTERFACE,
    .keyboard_interface = REMAPPER_USB_HID_KEYBOARD_INTERFACE,
};

const remapper_usb_hid_identity_t *remapper_usb_hid_identity(void)
{
    return &k_identity;
}

void remapper_usb_hid_build_mouse_report(
    remapper_usb_mouse_report_t *report,
    uint8_t buttons,
    int8_t x,
    int8_t y,
    int8_t wheel,
    int8_t pan)
{
    if (report == NULL) return;
    report->buttons = buttons;
    report->x = x;
    report->y = y;
    report->wheel = wheel;
    report->pan = pan;
}

void remapper_usb_hid_build_keyboard_report(
    remapper_usb_keyboard_report_t *report,
    uint8_t modifiers,
    const uint8_t keycodes[REMAPPER_USB_HID_KEYCODE_COUNT])
{
    if (report == NULL) return;

    report->modifiers = modifiers;
    report->reserved = 0u;
    for (size_t index = 0u; index < REMAPPER_USB_HID_KEYCODE_COUNT; ++index) {
        report->keycodes[index] = keycodes == NULL ? 0u : keycodes[index];
    }
}
