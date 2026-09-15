#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "remapper/ble_hogp/ble_hogp.h"
#include "remapper/bt_runtime/bt_runtime.h"
#include "remapper/hid_aggregator/hid_aggregator.h"
#include "remapper/usb_hid/usb_hid.h"

#define CHECK(expr)                                                             \
    do {                                                                        \
        if (!(expr)) {                                                          \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n",                    \
                    __FILE__, __LINE__, #expr);                                 \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

typedef struct {
    remapper_canonical_mouse_event_t events[32];
    size_t count;
} event_sink_t;

static bool sink_emit(
    void *context,
    const remapper_canonical_mouse_event_t *event)
{
    event_sink_t *sink = context;
    if (sink == NULL || sink->count >= 32u) return false;
    sink->events[sink->count++] = *event;
    return true;
}

/* Keyboard report ID 1 followed by a five-button Mouse report ID 2. */
static const uint8_t k_composite_report_map[] = {
    0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x85, 0x01,
    0x05, 0x07, 0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00,
    0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x08, 0x81, 0x01, 0xc0,

    0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x02,
    0x09, 0x01, 0xa1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x05, 0x15, 0x00,
    0x25, 0x01, 0x95, 0x05, 0x75, 0x01, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x03, 0x81, 0x01,
    0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x38,
    0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x03,
    0x81, 0x06,
    0x05, 0x0c, 0x0a, 0x38, 0x02, 0x15, 0x81, 0x25, 0x7f,
    0x75, 0x08, 0x95, 0x01, 0x81, 0x06,
    0xc0, 0xc0,
};

static void test_report_map_to_canonical_mouse(void)
{
    remapper_ble_hogp_parser_t parser;
    const remapper_hid_source_t source =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1u);

    CHECK(remapper_ble_hogp_parser_configure(
        &parser,
        source,
        k_composite_report_map,
        sizeof(k_composite_report_map)));
    CHECK(remapper_ble_hogp_parser_has_mouse(&parser));
    CHECK(parser.report_count == 2u);
    CHECK(parser.field_count == 9u);

    event_sink_t sink = {0};
    const uint8_t keyboard_report[2] = {0u, 0u};
    CHECK(remapper_ble_hogp_parser_parse_report(
        &parser,
        1u,
        keyboard_report,
        sizeof(keyboard_report),
        sink_emit,
        &sink));
    CHECK(sink.count == 0u);

    const uint8_t mouse_report[5] = {
        0x11u,
        10u,
        (uint8_t)-5,
        1u,
        (uint8_t)-2,
    };
    CHECK(remapper_ble_hogp_parser_parse_report(
        &parser,
        2u,
        mouse_report,
        sizeof(mouse_report),
        sink_emit,
        &sink));

    CHECK(sink.count == 4u);
    CHECK(sink.events[0].type == REMAPPER_MOUSE_EVENT_BUTTON);
    CHECK(sink.events[0].data.button.button == REMAPPER_MOUSE_BUTTON_LEFT);
    CHECK(sink.events[0].data.button.pressed);
    CHECK(sink.events[1].type == REMAPPER_MOUSE_EVENT_BUTTON);
    CHECK(sink.events[1].data.button.button == REMAPPER_MOUSE_BUTTON_FORWARD);
    CHECK(sink.events[1].data.button.pressed);
    CHECK(sink.events[2].type == REMAPPER_MOUSE_EVENT_MOVE);
    CHECK(sink.events[2].data.move.dx == 10);
    CHECK(sink.events[2].data.move.dy == -5);
    CHECK(sink.events[3].type == REMAPPER_MOUSE_EVENT_WHEEL);
    CHECK(sink.events[3].data.wheel.vertical == 1);
    CHECK(sink.events[3].data.wheel.horizontal == -2);

    const size_t before_repeat = sink.count;
    CHECK(remapper_ble_hogp_parser_parse_report(
        &parser,
        2u,
        mouse_report,
        sizeof(mouse_report),
        sink_emit,
        &sink));
    CHECK(sink.count == before_repeat + 2u);
    CHECK(sink.events[before_repeat].type == REMAPPER_MOUSE_EVENT_MOVE);
    CHECK(sink.events[before_repeat + 1u].type == REMAPPER_MOUSE_EVENT_WHEEL);

    const uint8_t released_report[5] = {0u, 0u, 0u, 0u, 0u};
    const size_t before_release = sink.count;
    CHECK(remapper_ble_hogp_parser_parse_report(
        &parser,
        2u,
        released_report,
        sizeof(released_report),
        sink_emit,
        &sink));
    CHECK(sink.count == before_release + 2u);
    CHECK(sink.events[before_release].type == REMAPPER_MOUSE_EVENT_BUTTON);
    CHECK(!sink.events[before_release].data.button.pressed);
    CHECK(sink.events[before_release + 1u].type == REMAPPER_MOUSE_EVENT_BUTTON);
    CHECK(!sink.events[before_release + 1u].data.button.pressed);

    CHECK(!remapper_ble_hogp_parser_parse_report(
        &parser,
        2u,
        mouse_report,
        1u,
        sink_emit,
        &sink));
}

static void test_non_mouse_descriptor_is_rejected(void)
{
    remapper_ble_hogp_parser_t parser;
    const remapper_hid_source_t source =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1u);
    CHECK(!remapper_ble_hogp_parser_configure(
        &parser,
        source,
        k_composite_report_map,
        31u));

    const uint8_t malformed[] = {0x05u};
    CHECK(!remapper_ble_hogp_parser_configure(
        &parser,
        source,
        malformed,
        sizeof(malformed)));
}

static void test_runtime_queue_and_decode(void)
{
    remapper_bt_runtime_reset();
    CHECK(!remapper_bt_runtime_take_overflow());

    remapper_canonical_mouse_event_t mouse = {0};
    mouse.source = remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1u);
    mouse.type = REMAPPER_MOUSE_EVENT_MOVE;
    mouse.data.move.dx = 3;
    mouse.data.move.dy = -4;

    CHECK(remapper_bt_runtime_publish(
        REMAPPER_BLE_HOGP_RUNTIME_CHANNEL,
        REMAPPER_BLE_HOGP_MESSAGE_MOUSE,
        &mouse,
        (uint16_t)sizeof(mouse)));

    remapper_bt_runtime_message_t message;
    CHECK(remapper_bt_runtime_poll(&message));

    remapper_ble_hogp_event_t event;
    CHECK(remapper_ble_hogp_decode_runtime_message(&message, &event));
    CHECK(event.type == REMAPPER_BLE_HOGP_EVENT_MOUSE);
    CHECK(event.mouse.data.move.dx == 3);
    CHECK(event.mouse.data.move.dy == -4);
    CHECK(!remapper_bt_runtime_poll(&message));

    remapper_bt_runtime_reset();
    for (unsigned int index = 0u;
         index < REMAPPER_BT_RUNTIME_QUEUE_CAPACITY;
         ++index) {
        CHECK(remapper_bt_runtime_publish(1u, 1u, NULL, 0u));
    }
    CHECK(!remapper_bt_runtime_publish(1u, 1u, NULL, 0u));
    CHECK(remapper_bt_runtime_take_overflow());
    CHECK(!remapper_bt_runtime_take_overflow());
}

static void test_aggregator_disconnect_and_chunked_relative_output(void)
{
    remapper_hid_aggregator_t aggregator;
    remapper_hid_output_state_t output;
    remapper_hid_aggregator_init(&aggregator);

    const remapper_hid_source_t source =
        remapper_hid_source_make(REMAPPER_HID_SOURCE_MOUSE, 1u);

    remapper_canonical_mouse_event_t event = {0};
    event.source = source;
    event.type = REMAPPER_MOUSE_EVENT_BUTTON;
    event.data.button.button = REMAPPER_MOUSE_BUTTON_LEFT;
    event.data.button.pressed = true;
    CHECK(remapper_hid_aggregator_apply_mouse(&aggregator, &event));

    event.type = REMAPPER_MOUSE_EVENT_MOVE;
    event.data.move.dx = 300;
    event.data.move.dy = -260;
    CHECK(remapper_hid_aggregator_apply_mouse(&aggregator, &event));

    event.type = REMAPPER_MOUSE_EVENT_WHEEL;
    event.data.wheel.vertical = 130;
    event.data.wheel.horizontal = -129;
    CHECK(remapper_hid_aggregator_apply_mouse(&aggregator, &event));

    remapper_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(output.mouse_buttons == 0x01u);
    CHECK(output.dx == 300);
    CHECK(output.dy == -260);
    CHECK(output.wheel_vertical == 130);
    CHECK(output.wheel_horizontal == -129);

    CHECK(remapper_hid_aggregator_consume_relative(
        &aggregator,
        127,
        -128,
        127,
        -128));
    remapper_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(output.dx == 173);
    CHECK(output.dy == -132);
    CHECK(output.wheel_vertical == 3);
    CHECK(output.wheel_horizontal == -1);

    CHECK(!remapper_hid_aggregator_consume_relative(
        &aggregator,
        -1,
        0,
        0,
        0));
    remapper_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(output.dx == 173);

    CHECK(remapper_hid_aggregator_release_source(&aggregator, source));
    remapper_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(output.mouse_buttons == 0u);
    CHECK(output.dx == 173);

    remapper_usb_mouse_report_t report;
    remapper_usb_hid_build_mouse_report(
        &report,
        output.mouse_buttons,
        127,
        -128,
        3,
        -1);
    CHECK(report.buttons == 0u);
    CHECK(report.x == 127);
    CHECK(report.y == -128);
    CHECK(report.wheel == 3);
    CHECK(report.pan == -1);
}

int main(void)
{
    test_report_map_to_canonical_mouse();
    test_non_mouse_descriptor_is_rejected();
    test_runtime_queue_and_decode();
    test_aggregator_disconnect_and_chunked_relative_output();

    puts("G06 BLE Mouse passthrough host tests passed");
    return 0;
}
