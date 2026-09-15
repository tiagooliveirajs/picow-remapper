#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "remapper/renderer/renderer.h"
#include "remapper/usb_hid/usb_hid.h"

static void test_optical_vertical_centering(void)
{
    remapper_ui_frame_t frame;
    remapper_ui_frame_reset(&frame, false, 7u);

    assert(REMAPPER_RENDERER_VERTICAL_GAP == 13u);
    assert(REMAPPER_RENDERER_REGION_NUDGE == 3u);
    assert(remapper_renderer_separator_boundary_y(&frame) == 183u);

    assert(remapper_renderer_text_y(&frame, 0u) == 8u);
    assert(remapper_renderer_text_y(&frame, 1u) == 38u);
    assert(remapper_renderer_text_y(&frame, 5u) == 146u);
    assert(remapper_renderer_text_y(&frame, 6u) == 170u);
    assert(remapper_renderer_text_y(&frame, 7u) == 194u);
    assert(remapper_renderer_text_y(&frame, 8u) == 221u);

    remapper_ui_frame_reset(&frame, true, REMAPPER_RENDERER_TEXT_ROWS);
    assert(remapper_renderer_text_y(&frame, 0u) == 8u);
    assert(remapper_renderer_text_y(&frame, 1u) == 35u);
    assert(remapper_renderer_text_y(&frame, 7u) == 197u);
}

static void test_fixed_usb_identity(void)
{
    const remapper_usb_hid_identity_t *identity = remapper_usb_hid_identity();
    assert(identity != NULL);
    assert(identity->vid == REMAPPER_USB_VID);
    assert(identity->pid == REMAPPER_USB_PID);
    assert(identity->bcd_device == REMAPPER_USB_BCD_DEVICE);
    assert(identity->interface_count == 2u);
    assert(identity->mouse_interface == 0u);
    assert(identity->keyboard_interface == 1u);
    assert(identity->mouse_interface != identity->keyboard_interface);
}

static void test_reports_exist_without_bluetooth_peer(void)
{
    remapper_usb_mouse_report_t mouse;
    remapper_usb_hid_build_mouse_report(&mouse, 0x11u, 4, -3, 1, -1);
    assert(mouse.buttons == 0x11u);
    assert(mouse.x == 4);
    assert(mouse.y == -3);
    assert(mouse.wheel == 1);
    assert(mouse.pan == -1);

    const uint8_t keycodes[REMAPPER_USB_HID_KEYCODE_COUNT] = {0x29u, 0u, 0u, 0u, 0u, 0u};
    remapper_usb_keyboard_report_t keyboard;
    remapper_usb_hid_build_keyboard_report(&keyboard, 0x02u, keycodes);
    assert(keyboard.modifiers == 0x02u);
    assert(keyboard.reserved == 0u);
    assert(keyboard.keycodes[0] == 0x29u);
    for (size_t index = 1u; index < REMAPPER_USB_HID_KEYCODE_COUNT; ++index) {
        assert(keyboard.keycodes[index] == 0u);
    }

    remapper_usb_hid_build_keyboard_report(&keyboard, 0u, NULL);
    assert(keyboard.modifiers == 0u);
    for (size_t index = 0u; index < REMAPPER_USB_HID_KEYCODE_COUNT; ++index) {
        assert(keyboard.keycodes[index] == 0u);
    }
}

int main(void)
{
    test_optical_vertical_centering();
    test_fixed_usb_identity();
    test_reports_exist_without_bluetooth_peer();
    return 0;
}
