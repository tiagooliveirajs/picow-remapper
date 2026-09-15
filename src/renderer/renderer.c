#include "remapper/renderer/renderer.h"

#include <string.h>

#define COLOR_BLACK        UINT16_C(0x0000)
#define COLOR_WHITE        UINT16_C(0xffff)
#define COLOR_MAGENTA      UINT16_C(0xf81f)
#define COLOR_CYAN         UINT16_C(0x07ff)
#define COLOR_YELLOW       UINT16_C(0xffe0)
#define COLOR_LIGHT_GRAY   UINT16_C(0xc618)
#define COLOR_DARK_MAGENTA UINT16_C(0x0801)

static const uint8_t k_alpha[26][7] = {
    {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11},{0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e},
    {0x0f,0x10,0x10,0x10,0x10,0x10,0x0f},{0x1e,0x11,0x11,0x11,0x11,0x11,0x1e},
    {0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f},{0x1f,0x10,0x10,0x1e,0x10,0x10,0x10},
    {0x0f,0x10,0x10,0x17,0x11,0x11,0x0f},{0x11,0x11,0x11,0x1f,0x11,0x11,0x11},
    {0x1f,0x04,0x04,0x04,0x04,0x04,0x1f},{0x07,0x02,0x02,0x02,0x12,0x12,0x0c},
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11},{0x10,0x10,0x10,0x10,0x10,0x10,0x1f},
    {0x11,0x1b,0x15,0x15,0x11,0x11,0x11},{0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    {0x0e,0x11,0x11,0x11,0x11,0x11,0x0e},{0x1e,0x11,0x11,0x1e,0x10,0x10,0x10},
    {0x0e,0x11,0x11,0x11,0x15,0x12,0x0d},{0x1e,0x11,0x11,0x1e,0x14,0x12,0x11},
    {0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e},{0x1f,0x04,0x04,0x04,0x04,0x04,0x04},
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0e},{0x11,0x11,0x11,0x11,0x11,0x0a,0x04},
    {0x11,0x11,0x11,0x15,0x15,0x15,0x0a},{0x11,0x11,0x0a,0x04,0x0a,0x11,0x11},
    {0x11,0x11,0x0a,0x04,0x04,0x04,0x04},{0x1f,0x01,0x02,0x04,0x08,0x10,0x1f},
};

static const uint8_t k_digit[10][7] = {
    {0x0e,0x11,0x13,0x15,0x19,0x11,0x0e},{0x04,0x0c,0x04,0x04,0x04,0x04,0x0e},
    {0x0e,0x11,0x01,0x02,0x04,0x08,0x1f},{0x1e,0x01,0x01,0x0e,0x01,0x01,0x1e},
    {0x02,0x06,0x0a,0x12,0x1f,0x02,0x02},{0x1f,0x10,0x10,0x1e,0x01,0x01,0x1e},
    {0x0e,0x10,0x10,0x1e,0x11,0x11,0x0e},{0x1f,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e},{0x0e,0x11,0x11,0x0f,0x01,0x01,0x0e},
};

static uint8_t glyph_row(char character, uint8_t row)
{
    if (row >= 7u) return 0u;
    if (character >= 'A' && character <= 'Z') return k_alpha[(uint8_t)(character - 'A')][row];
    if (character >= '0' && character <= '9') return k_digit[(uint8_t)(character - '0')][row];

    switch (character) {
    case '-': return row == 3u ? 0x1fu : 0u;
    case ':': return (row == 2u || row == 5u) ? 0x04u : 0u;
    case '.': return row == 6u ? 0x04u : 0u;
    case '=': return (row == 2u || row == 4u) ? 0x1fu : 0u;
    case '/': return row <= 4u ? (uint8_t)(1u << (4u - row)) : 0u;
    case '>': {
        static const uint8_t rows[7] = {0x10,0x08,0x04,0x02,0x04,0x08,0x10};
        return rows[row];
    }
    case '<': {
        static const uint8_t rows[7] = {0x01,0x02,0x04,0x08,0x04,0x02,0x01};
        return rows[row];
    }
    case '?': {
        static const uint8_t rows[7] = {0x0e,0x11,0x01,0x02,0x04,0x00,0x04};
        return rows[row];
    }
    case '!': return (row < 5u || row == 6u) ? 0x04u : 0u;
    default: return 0u;
    }
}

void remapper_ui_frame_reset(remapper_ui_frame_t *frame, bool learn_background, uint8_t hint_start_row)
{
    if (frame == NULL) return;
    if (hint_start_row > REMAPPER_RENDERER_TEXT_ROWS) hint_start_row = REMAPPER_RENDERER_TEXT_ROWS;

    frame->learn_background = learn_background;
    frame->hint_start_row = hint_start_row;
    for (size_t row = 0u; row < REMAPPER_RENDERER_TEXT_ROWS; ++row) {
        for (size_t column = 0u; column < REMAPPER_RENDERER_TEXT_COLS; ++column) {
            frame->cells[row][column].character = ' ';
            frame->cells[row][column].tone = REMAPPER_UI_TONE_ACTIONABLE;
        }
    }
}

bool remapper_ui_frame_set_text(
    remapper_ui_frame_t *frame,
    uint8_t row,
    uint8_t column,
    const char *text,
    remapper_ui_tone_t tone)
{
    if (frame == NULL || text == NULL || row >= REMAPPER_RENDERER_TEXT_ROWS || column >= REMAPPER_RENDERER_TEXT_COLS) {
        return false;
    }

    size_t index = 0u;
    while (text[index] != '\0' && (size_t)column + index < REMAPPER_RENDERER_TEXT_COLS) {
        frame->cells[row][(size_t)column + index].character = text[index];
        frame->cells[row][(size_t)column + index].tone = tone;
        ++index;
    }
    return text[index] == '\0';
}

bool remapper_ui_frame_set_tone_span(
    remapper_ui_frame_t *frame,
    uint8_t row,
    uint8_t column,
    uint8_t length,
    remapper_ui_tone_t tone)
{
    if (frame == NULL || row >= REMAPPER_RENDERER_TEXT_ROWS || column >= REMAPPER_RENDERER_TEXT_COLS ||
        (size_t)column + length > REMAPPER_RENDERER_TEXT_COLS) {
        return false;
    }

    for (size_t index = 0u; index < length; ++index) {
        frame->cells[row][(size_t)column + index].tone = tone;
    }
    return true;
}

uint16_t remapper_renderer_tone_rgb565(remapper_ui_tone_t tone)
{
    switch (tone) {
    case REMAPPER_UI_TONE_TITLE: return COLOR_MAGENTA;
    case REMAPPER_UI_TONE_STATIC: return COLOR_YELLOW;
    case REMAPPER_UI_TONE_ACTIONABLE: return COLOR_LIGHT_GRAY;
    case REMAPPER_UI_TONE_EMPHASIZED: return COLOR_WHITE;
    case REMAPPER_UI_TONE_CURRENT: return COLOR_CYAN;
    default: return COLOR_LIGHT_GRAY;
    }
}

uint16_t remapper_renderer_background_rgb565(const remapper_ui_frame_t *frame, uint8_t row)
{
    if (frame == NULL) return COLOR_BLACK;
    if (frame->learn_background) return COLOR_DARK_MAGENTA;
    return row >= frame->hint_start_row ? COLOR_DARK_MAGENTA : COLOR_BLACK;
}

uint16_t remapper_renderer_separator_boundary_y(const remapper_ui_frame_t *frame)
{
    if (frame == NULL || frame->learn_background || frame->hint_start_row == 0u ||
        frame->hint_start_row >= REMAPPER_RENDERER_TEXT_ROWS) {
        return REMAPPER_RENDERER_HEIGHT;
    }

    const uint16_t separator_row = (uint16_t)(frame->hint_start_row - 1u);
    const uint16_t boundary = (uint16_t)(
        REMAPPER_RENDERER_TEXT_Y +
        separator_row * REMAPPER_RENDERER_LINE_ADVANCE +
        REMAPPER_RENDERER_LINE_ADVANCE / 2u);
    return boundary < REMAPPER_RENDERER_HEIGHT ? boundary : REMAPPER_RENDERER_HEIGHT;
}

uint16_t remapper_renderer_text_y(const remapper_ui_frame_t *frame, uint8_t row)
{
    if (row >= REMAPPER_RENDERER_TEXT_ROWS) return REMAPPER_RENDERER_HEIGHT;

    const uint16_t base = (uint16_t)(REMAPPER_RENDERER_TEXT_Y + (uint16_t)row * REMAPPER_RENDERER_LINE_ADVANCE);
    if (frame == NULL || frame->learn_background || row == 0u || frame->hint_start_row == 0u ||
        frame->hint_start_row > REMAPPER_RENDERER_TEXT_ROWS) {
        return base;
    }

    const uint8_t separator_row = (uint8_t)(frame->hint_start_row - 1u);
    if (row > 0u && row < separator_row) {
        return (uint16_t)(base + REMAPPER_RENDERER_REGION_NUDGE);
    }
    if (row >= frame->hint_start_row) {
        return (uint16_t)(base - REMAPPER_RENDERER_REGION_NUDGE);
    }
    return base;
}

static bool draw_cell(
    const remapper_display_hal_t *display,
    const remapper_ui_frame_t *frame,
    uint8_t row,
    uint8_t column)
{
    const remapper_ui_cell_t *cell = &frame->cells[row][column];
    if (cell->character == ' ') return true;

    uint16_t pixels[REMAPPER_RENDERER_GLYPH_WIDTH * REMAPPER_RENDERER_GLYPH_HEIGHT];
    const uint16_t foreground = remapper_renderer_tone_rgb565(cell->tone);
    const uint16_t background = remapper_renderer_background_rgb565(frame, row);
    size_t out = 0u;

    for (uint8_t py = 0u; py < REMAPPER_RENDERER_GLYPH_HEIGHT; ++py) {
        const uint8_t bits = glyph_row(cell->character, (uint8_t)(py / REMAPPER_RENDERER_GLYPH_SCALE));
        for (uint8_t px = 0u; px < REMAPPER_RENDERER_GLYPH_WIDTH; ++px) {
            const uint8_t source_x = (uint8_t)(px / REMAPPER_RENDERER_GLYPH_SCALE);
            const bool on = (bits & (uint8_t)(1u << (4u - source_x))) != 0u;
            pixels[out++] = on ? foreground : background;
        }
    }

    const uint16_t x = (uint16_t)(REMAPPER_RENDERER_TEXT_X + (uint16_t)column * REMAPPER_RENDERER_CHAR_ADVANCE);
    const uint16_t y = remapper_renderer_text_y(frame, row);
    return display->write_rgb565(
        display->context,
        x,
        y,
        REMAPPER_RENDERER_GLYPH_WIDTH,
        REMAPPER_RENDERER_GLYPH_HEIGHT,
        pixels);
}

bool remapper_renderer_render(const remapper_display_hal_t *display, const remapper_ui_frame_t *frame)
{
    if (display == NULL || frame == NULL || display->fill_rect == NULL || display->write_rgb565 == NULL) return false;

    if (frame->learn_background) {
        if (!display->fill_rect(
                display->context,
                0u,
                0u,
                REMAPPER_RENDERER_WIDTH,
                REMAPPER_RENDERER_HEIGHT,
                COLOR_DARK_MAGENTA)) {
            return false;
        }
    } else {
        const uint16_t boundary = remapper_renderer_separator_boundary_y(frame);
        if (boundary > 0u && !display->fill_rect(
                display->context,
                0u,
                0u,
                REMAPPER_RENDERER_WIDTH,
                boundary,
                COLOR_BLACK)) {
            return false;
        }
        if (boundary < REMAPPER_RENDERER_HEIGHT && !display->fill_rect(
                display->context,
                0u,
                boundary,
                REMAPPER_RENDERER_WIDTH,
                (uint16_t)(REMAPPER_RENDERER_HEIGHT - boundary),
                COLOR_DARK_MAGENTA)) {
            return false;
        }
    }

    for (uint8_t row = 0u; row < REMAPPER_RENDERER_TEXT_ROWS; ++row) {
        for (uint8_t column = 0u; column < REMAPPER_RENDERER_TEXT_COLS; ++column) {
            if (!draw_cell(display, frame, row, column)) return false;
        }
    }
    return true;
}
