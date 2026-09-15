#include <stdio.h>
#include <stdlib.h>

#include "remapper/ble_hogp/ble_hogp.h"

#define CHECK(expr)                                                             \
    do {                                                                        \
        if (!(expr)) {                                                          \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n",                    \
                    __FILE__, __LINE__, #expr);                                 \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

typedef struct {
    remapper_canonical_mouse_event_t events[8];
    size_t count;
} event_sink_t;

static bool sink_emit(
    void *context,
    const remapper_canonical_mouse_event_t *event)
{
    event_sink_t *sink = context;
    if (sink == NULL || sink->count >= 8u) return false;
    sink->events[sink->count++] = *event;
    return true;
}

/* Report ID 2: three buttons + padding + signed relative X/Y. */
static const uint8_t k_mouse_report_map[] = {
    0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x02,
    0x09, 0x01, 0xa1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x03,
    0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x05, 0x81, 0x01,
    0x05, 0x01, 0x09, 0x30, 0x09, 0x31,
    0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x02, 0x81, 0x06,
    0xc0, 0xc0,
};

static void configure(remapper_ble_hogp_parser_t *parser)
{
    const remapper_hid_source_t source =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1u);
    CHECK(remapper_ble_hogp_parser_configure(
        parser,
        source,
        k_mouse_report_map,
        sizeof(k_mouse_report_map)));
    CHECK(parser->report_count == 1u);
    CHECK(parser->reports[0].report_id == 2u);
    CHECK(parser->reports[0].input_bits == 24u);
}

static void assert_left_move(const event_sink_t *sink)
{
    CHECK(sink->count == 2u);
    CHECK(sink->events[0].type == REMAPPER_MOUSE_EVENT_BUTTON);
    CHECK(sink->events[0].data.button.button == REMAPPER_MOUSE_BUTTON_LEFT);
    CHECK(sink->events[0].data.button.pressed);
    CHECK(sink->events[1].type == REMAPPER_MOUSE_EVENT_MOVE);
    CHECK(sink->events[1].data.move.dx == 10);
    CHECK(sink->events[1].data.move.dy == -5);
}

static void test_canonical_payload_is_preserved(void)
{
    remapper_ble_hogp_parser_t parser;
    configure(&parser);

    const uint8_t payload[] = {0x01u, 10u, (uint8_t)-5};
    event_sink_t sink = {0};
    CHECK(remapper_ble_hogp_parser_parse_report(
        &parser, 2u, payload, sizeof(payload), sink_emit, &sink));
    assert_left_move(&sink);
}

static void test_btstack_duplicate_report_id_is_stripped(void)
{
    remapper_ble_hogp_parser_t parser;
    configure(&parser);

    /* Pico SDK 2.2.0 HIDS event framing seen in the proven Lift POC. */
    const uint8_t btstack_report[] = {0x02u, 0x01u, 10u, (uint8_t)-5};
    event_sink_t sink = {0};
    CHECK(remapper_ble_hogp_parser_parse_report(
        &parser,
        2u,
        btstack_report,
        sizeof(btstack_report),
        sink_emit,
        &sink));
    assert_left_move(&sink);
}

static void test_misaligned_extra_byte_is_rejected(void)
{
    remapper_ble_hogp_parser_t parser;
    configure(&parser);

    const uint8_t malformed[] = {0x7fu, 0x01u, 10u, (uint8_t)-5};
    event_sink_t sink = {0};
    CHECK(!remapper_ble_hogp_parser_parse_report(
        &parser,
        2u,
        malformed,
        sizeof(malformed),
        sink_emit,
        &sink));
    CHECK(sink.count == 0u);
}

int main(void)
{
    test_canonical_payload_is_preserved();
    test_btstack_duplicate_report_id_is_stripped();
    test_misaligned_extra_byte_is_rejected();
    puts("G06 BTstack HIDS framing tests passed");
    return 0;
}
