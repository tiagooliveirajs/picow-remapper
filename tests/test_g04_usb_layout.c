#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "remapper/renderer/renderer.h"
#include "remapper/usb_hid/usb_hid.h"

static uint16_t glyph_bottom(const remapper_ui_frame_t *frame, uint8_t row)
{
    return (uint16_t)(remapper_renderer_text_y(frame, row) + REMAPPER_RENDERER_GLYPH_HEIGHT);
}

static void test_learn_vertical_redistribution(void)
{
    remapper_ui_frame_t frame;
    remapper_ui_frame_reset(&frame, true, REMAPPER_RENDERER_TEXT_ROWS);

    const uint16_t expected_y[REMAPPER_RENDERER_TEXT_ROWS] = {
        8u, 39u, 64u, 89u, 114u, 139u, 164u, 189u, 214u,
    };

    assert(REMAPPER_RENDERER_GLYPH_HEIGHT == 14u);
    assert(REMAPPER_RENDERER_VERTICAL_GAP == 13u);
    assert(REMAPPER_RENDERER_TITLE_BODY_GAP == 17u);
    assert(REMAPPER_RENDERER_LEARN_LINE_GAP == 11u);
    assert(REMAPPER_RENDERER_LEARN_BOTTOM_GAP == 12u);

    for (uint8_t row = 0u; row < REMAPPER_RENDERER_TEXT_ROWS; ++row) {
        assert(remapper_renderer_text_y(&frame, row) == expected_y[row]);
    }

    assert(remapper_renderer_text_y(&frame, 0u) == 8u);
    assert((uint16_t)(remapper_renderer_text_y(&frame, 1u) - glyph_bottom(&frame, 0u)) == 17u);
    for (uint8_t row = 1u; row < REMAPPER_RENDERER_TEXT_ROWS - 1u; ++row) {
        assert((uint16_t)(remapper_renderer_text_y(&frame, (uint8_t)(row + 1u)) - glyph_bottom(&frame, row)) == 11u);
    }
    assert((uint16_t)(REMAPPER_RENDERER_HEIGHT - glyph_bottom(&frame, 8u)) == 12u);
    assert(remapper_renderer_separator_boundary_y(&frame) == REMAPPER_RENDERER_HEIGHT);
}

static void assert_standard_layout(uint8_t hint_start, uint8_t body_lines, uint16_t boundary)
{
    remapper_ui_frame_t frame;
    remapper_ui_frame_reset(&frame, false, hint_start);

    assert(remapper_renderer_text_y(&frame, 0u) == 8u);
    assert(remapper_renderer_text_y(&frame, 1u) == 39u);
    assert((uint16_t)(remapper_renderer_text_y(&frame, 1u) - glyph_bottom(&frame, 0u)) == 17u);

    for (uint8_t row = 1u; row < body_lines; ++row) {
        assert((uint16_t)(remapper_renderer_text_y(&frame, (uint8_t)(row + 1u)) - glyph_bottom(&frame, row)) == 12u);
    }

    assert(remapper_renderer_separator_boundary_y(&frame) == boundary);
    assert((uint16_t)(boundary - glyph_bottom(&frame, body_lines)) == 20u);

    assert((uint16_t)(remapper_renderer_text_y(&frame, hint_start) - boundary) == 11u);
    for (uint8_t row = hint_start; row < REMAPPER_RENDERER_TEXT_ROWS - 1u; ++row) {
        assert((uint16_t)(remapper_renderer_text_y(&frame, (uint8_t)(row + 1u)) - glyph_bottom(&frame, row)) == 12u);
    }
    assert((uint16_t)(REMAPPER_RENDERER_HEIGHT - glyph_bottom(&frame, 8u)) == 12u);

    assert((uint16_t)(boundary + (REMAPPER_RENDERER_HEIGHT - boundary)) == REMAPPER_RENDERER_HEIGHT);
}

static void test_standard_vertical_redistribution(void)
{
    assert(REMAPPER_RENDERER_BODY_LINE_GAP == 12u);
    assert(REMAPPER_RENDERER_BODY_BOTTOM_GAP == 20u);
    assert(REMAPPER_RENDERER_HINT_TOP_GAP == 11u);
    assert(REMAPPER_RENDERER_HINT_LINE_GAP == 12u);
    assert(REMAPPER_RENDERER_HINT_BOTTOM_GAP == 12u);

    assert_standard_layout(8u, 6u, 203u);
    assert_standard_layout(7u, 5u, 177u);
    assert_standard_layout(6u, 4u, 151u);

    remapper_ui_frame_t frame;
    remapper_ui_frame_reset(&frame, false, 6u);
    assert(remapper_renderer_text_y(&frame, 1u) == 39u);
    assert(remapper_renderer_text_y(&frame, 2u) == 65u);
    assert(remapper_renderer_text_y(&frame, 3u) == 91u);
    assert(remapper_renderer_text_y(&frame, 4u) == 117u);
    assert(remapper_renderer_text_y(&frame, 6u) == 162u);
    assert(remapper_renderer_text_y(&frame, 7u) == 188u);
    assert(remapper_renderer_text_y(&frame, 8u) == 214u);
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
    test_learn_vertical_redistribution();
    test_standard_vertical_redistribution();
    test_fixed_usb_identity();
    test_reports_exist_without_bluetooth_peer();
    return 0;
}
