#include "remapper/usb_hid/usb_hid.h"

#include <stddef.h>
#include "tusb.h"

bool remapper_usb_hid_pico_init(void)
{
    return tud_init(0u);
}

void remapper_usb_hid_pico_task(void)
{
    tud_task();
}

bool remapper_usb_hid_pico_mounted(void)
{
    return tud_mounted();
}

bool remapper_usb_hid_pico_send_mouse(const remapper_usb_mouse_report_t *report)
{
    if (report == NULL || !tud_hid_n_ready(REMAPPER_USB_HID_MOUSE_INTERFACE)) return false;
    return tud_hid_n_report(
        REMAPPER_USB_HID_MOUSE_INTERFACE,
        0u,
        report,
        (uint16_t)sizeof(*report));
}

bool remapper_usb_hid_pico_send_keyboard(const remapper_usb_keyboard_report_t *report)
{
    if (report == NULL || !tud_hid_n_ready(REMAPPER_USB_HID_KEYBOARD_INTERFACE)) return false;
    return tud_hid_n_report(
        REMAPPER_USB_HID_KEYBOARD_INTERFACE,
        0u,
        report,
        (uint16_t)sizeof(*report));
}

uint16_t tud_hid_get_report_cb(
    uint8_t instance,
    uint8_t report_id,
    hid_report_type_t report_type,
    uint8_t *buffer,
    uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0u;
}

void tud_hid_set_report_cb(
    uint8_t instance,
    uint8_t report_id,
    hid_report_type_t report_type,
    const uint8_t *buffer,
    uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}
