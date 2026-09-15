#ifndef REMAPPER_BT_RUNTIME_BT_RUNTIME_H
#define REMAPPER_BT_RUNTIME_BT_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REMAPPER_BT_RUNTIME_MESSAGE_PAYLOAD_SIZE 32u
#define REMAPPER_BT_RUNTIME_QUEUE_CAPACITY 128u

typedef struct {
    uint16_t channel;
    uint16_t type;
    uint16_t length;
    uint8_t payload[REMAPPER_BT_RUNTIME_MESSAGE_PAYLOAD_SIZE];
} remapper_bt_runtime_message_t;

typedef void (*remapper_bt_runtime_session_setup_fn)(void);

void remapper_bt_runtime_reset(void);
bool remapper_bt_runtime_publish(
    uint16_t channel,
    uint16_t type,
    const void *payload,
    uint16_t length);
bool remapper_bt_runtime_poll(remapper_bt_runtime_message_t *message);
bool remapper_bt_runtime_take_overflow(void);

/* Pico implementation: starts the single BTstack owner on Core1. */
bool remapper_bt_runtime_start(remapper_bt_runtime_session_setup_fn session_setup);

#ifdef __cplusplus
}
#endif

#endif
