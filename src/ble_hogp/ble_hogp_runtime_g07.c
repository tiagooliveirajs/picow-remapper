#include "remapper/ble_hogp/ble_hogp.h"

#include <string.h>

bool remapper_ble_hogp_decode_runtime_message(
    const remapper_bt_runtime_message_t *message,
    remapper_ble_hogp_event_t *event)
{
    if (message == NULL || event == NULL ||
        message->channel != REMAPPER_BLE_HOGP_RUNTIME_CHANNEL) {
        return false;
    }

    memset(event, 0, sizeof(*event));
    switch (message->type) {
    case REMAPPER_BLE_HOGP_MESSAGE_CONNECTED:
        if (message->length != sizeof(remapper_ble_hogp_peer_t)) return false;
        event->type = REMAPPER_BLE_HOGP_EVENT_CONNECTED;
        memcpy(&event->peer, message->payload, sizeof(event->peer));
        return true;
    case REMAPPER_BLE_HOGP_MESSAGE_DISCONNECTED:
        if (message->length != 0u) return false;
        event->type = REMAPPER_BLE_HOGP_EVENT_DISCONNECTED;
        return true;
    case REMAPPER_BLE_HOGP_MESSAGE_MOUSE:
        if (message->length != sizeof(remapper_canonical_mouse_event_t)) {
            return false;
        }
        event->type = REMAPPER_BLE_HOGP_EVENT_MOUSE;
        memcpy(&event->mouse, message->payload, sizeof(event->mouse));
        return true;
    default:
        return false;
    }
}
