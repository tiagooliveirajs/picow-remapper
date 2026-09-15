#ifndef REMAPPER_RENDERER_ST7789_PICO_H
#define REMAPPER_RENDERER_ST7789_PICO_H

#include <stdbool.h>
#include "remapper/renderer/renderer.h"

bool remapper_st7789_pico_init(remapper_display_hal_t *display);
void remapper_st7789_pico_set_backlight(bool enabled);

#endif
