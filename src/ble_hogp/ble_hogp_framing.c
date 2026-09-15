#include "remapper/ble_hogp/ble_hogp.h"

/*
 * Pico SDK 2.2.0's BTstack HIDS client exposes the Report ID in event
 * metadata and, for input notifications, may also keep that same Report ID
 * as the first byte of the returned report buffer. The canonical parser owns
 * Report IDs separately, so passing that transport framing through shifts all
 * HID fields by one byte (buttons become the ID, X becomes buttons, etc.).
 *
 * Keep the transport quirk at this adapter boundary. The payload parser is
 * compiled under the internal symbol below; this wrapper accepts either the
 * descriptor-sized canonical payload or the descriptor-sized payload prefixed
 * by one duplicated Report ID. Any other length is rejected instead of being
 * interpreted with shifted fields.
 */
extern bool remapper_ble_hogp_parser_parse_report_payload(
    remapper_ble_hogp_parser_t *parser,
    uint8_t report_id,
    const uint8_t *report,
    size_t report_len,
    remapper_ble_hogp_emit_fn emit,
    void *context);

static const remapper_ble_hogp_report_state_t *find_report_state(
    const remapper_ble_hogp_parser_t *parser,
    uint8_t report_id)
{
    if (parser == NULL) return NULL;
    for (size_t index = 0u; index < parser->report_count; ++index) {
        if (parser->reports[index].report_id == report_id) {
            return &parser->reports[index];
        }
    }
    return NULL;
}

bool remapper_ble_hogp_parser_parse_report(
    remapper_ble_hogp_parser_t *parser,
    uint8_t report_id,
    const uint8_t *report,
    size_t report_len,
    remapper_ble_hogp_emit_fn emit,
    void *context)
{
    if (parser == NULL || report == NULL || emit == NULL) return false;

    const remapper_ble_hogp_report_state_t *state =
        find_report_state(parser, report_id);
    if (state == NULL) return false;

    const size_t expected_len = ((size_t)state->input_bits + 7u) / 8u;
    const uint8_t *payload = report;
    size_t payload_len = report_len;

    if (report_len == expected_len) {
        /* Already canonical: Report ID arrived only in event metadata. */
    } else if (report_len == expected_len + 1u &&
               report_len > 0u &&
               report[0] == report_id) {
        /* BTstack HIDS framing: duplicated Report ID precedes the payload. */
        payload = report + 1u;
        payload_len = report_len - 1u;
    } else {
        return false;
    }

    return remapper_ble_hogp_parser_parse_report_payload(
        parser,
        report_id,
        payload,
        payload_len,
        emit,
        context);
}
