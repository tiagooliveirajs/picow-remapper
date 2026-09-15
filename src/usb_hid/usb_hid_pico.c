#include "remapper/usb_hid/usb_hid.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include "tusb.h"

#if defined(REMAPPER_USB_DEBUG_CDC) && REMAPPER_USB_DEBUG_CDC
#define REMAPPER_USB_DEBUG_RING_SIZE 4096u
#define REMAPPER_USB_DEBUG_LINE_SIZE 192u
static uint8_t g_debug_ring[REMAPPER_USB_DEBUG_RING_SIZE];
static size_t g_debug_head;
static size_t g_debug_tail;
static size_t g_debug_count;

static void debug_ring_push(const uint8_t *data, size_t length)
{
    for (size_t index = 0u; index < length; ++index) {
        if (g_debug_count == REMAPPER_USB_DEBUG_RING_SIZE) {
            g_debug_tail = (g_debug_tail + 1u) % REMAPPER_USB_DEBUG_RING_SIZE;
            --g_debug_count;
        }
        g_debug_ring[g_debug_head] = data[index];
        g_debug_head = (g_debug_head + 1u) % REMAPPER_USB_DEBUG_RING_SIZE;
        ++g_debug_count;
    }
}

static void debug_cdc_flush(void)
{
    if (!tud_cdc_connected() || g_debug_count == 0u) return;

    while (g_debug_count > 0u) {
        size_t contiguous = REMAPPER_USB_DEBUG_RING_SIZE - g_debug_tail;
        if (contiguous > g_debug_count) contiguous = g_debug_count;
        const uint32_t written = tud_cdc_write(&g_debug_ring[g_debug_tail], (uint32_t)contiguous);
        if (written == 0u) break;
        g_debug_tail = (g_debug_tail + (size_t)written) % REMAPPER_USB_DEBUG_RING_SIZE;
        g_debug_count -= (size_t)written;
    }
    tud_cdc_write_flush();
}
#endif

bool remapper_usb_hid_pico_init(void)
{
#if defined(REMAPPER_USB_DEBUG_CDC) && REMAPPER_USB_DEBUG_CDC
    g_debug_head = 0u;
    g_debug_tail = 0u;
    g_debug_count = 0u;
#endif
    return tud_init(0u);
}

void remapper_usb_hid_pico_task(void)
{
    tud_task();
#if defined(REMAPPER_USB_DEBUG_CDC) && REMAPPER_USB_DEBUG_CDC
    debug_cdc_flush();
#endif
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

void remapper_usb_hid_pico_debug_printf(const char *format, ...)
{
#if defined(REMAPPER_USB_DEBUG_CDC) && REMAPPER_USB_DEBUG_CDC
    if (format == NULL) return;
    char line[REMAPPER_USB_DEBUG_LINE_SIZE];
    va_list args;
    va_start(args, format);
    const int result = vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    if (result <= 0) return;
    size_t length = (size_t)result;
    if (length >= sizeof(line)) length = sizeof(line) - 1u;
    debug_ring_push((const uint8_t *)line, length);
#else
    (void)format;
#endif
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
