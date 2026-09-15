#ifndef REMAPPER_RENDERER_RENDERER_H
#define REMAPPER_RENDERER_RENDERER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define REMAPPER_RENDERER_WIDTH 240u
#define REMAPPER_RENDERER_HEIGHT 240u
#define REMAPPER_RENDERER_TEXT_ROWS 9u
#define REMAPPER_RENDERER_TEXT_COLS 21u
#define REMAPPER_RENDERER_GLYPH_SCALE 2u
#define REMAPPER_RENDERER_GLYPH_WIDTH 10u
#define REMAPPER_RENDERER_GLYPH_HEIGHT 14u
#define REMAPPER_RENDERER_CHAR_ADVANCE 11u
#define REMAPPER_RENDERER_LINE_ADVANCE 27u
#define REMAPPER_RENDERER_TEXT_X 7u
#define REMAPPER_RENDERER_TEXT_Y 8u
#define REMAPPER_RENDERER_VERTICAL_GAP (REMAPPER_RENDERER_LINE_ADVANCE - REMAPPER_RENDERER_GLYPH_HEIGHT)
#define REMAPPER_RENDERER_REGION_NUDGE (REMAPPER_RENDERER_VERTICAL_GAP / 4u)

typedef enum {
    REMAPPER_UI_TONE_TITLE = 0,
    REMAPPER_UI_TONE_STATIC,
    REMAPPER_UI_TONE_ACTIONABLE,
    REMAPPER_UI_TONE_EMPHASIZED,
    REMAPPER_UI_TONE_CURRENT,
} remapper_ui_tone_t;

typedef struct {
    char character;
    remapper_ui_tone_t tone;
} remapper_ui_cell_t;

typedef struct {
    remapper_ui_cell_t cells[REMAPPER_RENDERER_TEXT_ROWS][REMAPPER_RENDERER_TEXT_COLS];
    bool learn_background;
    uint8_t hint_start_row;
} remapper_ui_frame_t;

typedef struct {
    void *context;
    bool (*fill_rect)(void *context, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t rgb565);
    bool (*write_rgb565)(void *context, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pixels);
} remapper_display_hal_t;

void remapper_ui_frame_reset(remapper_ui_frame_t *frame, bool learn_background, uint8_t hint_start_row);
bool remapper_ui_frame_set_text(remapper_ui_frame_t *frame, uint8_t row, uint8_t column, const char *text, remapper_ui_tone_t tone);
bool remapper_ui_frame_set_tone_span(remapper_ui_frame_t *frame, uint8_t row, uint8_t column, uint8_t length, remapper_ui_tone_t tone);
uint16_t remapper_renderer_tone_rgb565(remapper_ui_tone_t tone);
uint16_t remapper_renderer_background_rgb565(const remapper_ui_frame_t *frame, uint8_t row);
uint16_t remapper_renderer_separator_boundary_y(const remapper_ui_frame_t *frame);
uint16_t remapper_renderer_text_y(const remapper_ui_frame_t *frame, uint8_t row);
bool remapper_renderer_render(const remapper_display_hal_t *display, const remapper_ui_frame_t *frame);

#endif
