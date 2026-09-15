#ifndef REMAPPER_TUSB_CONFIG_H
#define REMAPPER_TUSB_CONFIG_H

#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined by the Pico SDK TinyUSB integration
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

#define CFG_TUD_ENABLED 1

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE 64
#endif

#ifndef CFG_TUD_HID
#define CFG_TUD_HID 2
#endif

#ifndef CFG_TUD_CDC
#if defined(REMAPPER_USB_DEBUG_CDC) && REMAPPER_USB_DEBUG_CDC
#define CFG_TUD_CDC 1
#else
#define CFG_TUD_CDC 0
#endif
#endif

#if CFG_TUD_CDC
#ifndef CFG_TUD_CDC_RX_BUFSIZE
#define CFG_TUD_CDC_RX_BUFSIZE 256
#endif
#ifndef CFG_TUD_CDC_TX_BUFSIZE
#define CFG_TUD_CDC_TX_BUFSIZE 256
#endif
#endif

#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0
#define CFG_TUD_HID_EP_BUFSIZE 16

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

#endif
