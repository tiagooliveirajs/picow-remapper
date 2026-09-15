#include "remapper/bt_runtime/bt_runtime.h"

#include <stdatomic.h>
#include <string.h>

static remapper_bt_runtime_message_t g_queue[REMAPPER_BT_RUNTIME_QUEUE_CAPACITY];
static atomic_uint g_write_sequence = ATOMIC_VAR_INIT(0u);
static atomic_uint g_read_sequence = ATOMIC_VAR_INIT(0u);
static atomic_bool g_overflowed = ATOMIC_VAR_INIT(false);

void remapper_bt_runtime_reset(void)
{
    atomic_store_explicit(&g_read_sequence, 0u, memory_order_relaxed);
    atomic_store_explicit(&g_write_sequence, 0u, memory_order_relaxed);
    atomic_store_explicit(&g_overflowed, false, memory_order_relaxed);
    memset(g_queue, 0, sizeof(g_queue));
}

bool remapper_bt_runtime_publish(
    uint16_t channel,
    uint16_t type,
    const void *payload,
    uint16_t length)
{
    if (length > REMAPPER_BT_RUNTIME_MESSAGE_PAYLOAD_SIZE ||
        (length > 0u && payload == NULL)) {
        return false;
    }

    const unsigned int write_sequence =
        atomic_load_explicit(&g_write_sequence, memory_order_relaxed);
    const unsigned int read_sequence =
        atomic_load_explicit(&g_read_sequence, memory_order_acquire);

    if ((unsigned int)(write_sequence - read_sequence) >=
        REMAPPER_BT_RUNTIME_QUEUE_CAPACITY) {
        atomic_store_explicit(&g_overflowed, true, memory_order_release);
        return false;
    }

    remapper_bt_runtime_message_t *message =
        &g_queue[write_sequence % REMAPPER_BT_RUNTIME_QUEUE_CAPACITY];
    message->channel = channel;
    message->type = type;
    message->length = length;

    if (length > 0u) memcpy(message->payload, payload, length);
    if (length < REMAPPER_BT_RUNTIME_MESSAGE_PAYLOAD_SIZE) {
        memset(
            &message->payload[length],
            0,
            REMAPPER_BT_RUNTIME_MESSAGE_PAYLOAD_SIZE - length);
    }

    atomic_store_explicit(
        &g_write_sequence,
        write_sequence + 1u,
        memory_order_release);
    return true;
}

bool remapper_bt_runtime_poll(remapper_bt_runtime_message_t *message)
{
    if (message == NULL) return false;

    const unsigned int read_sequence =
        atomic_load_explicit(&g_read_sequence, memory_order_relaxed);
    const unsigned int write_sequence =
        atomic_load_explicit(&g_write_sequence, memory_order_acquire);
    if (read_sequence == write_sequence) return false;

    *message = g_queue[read_sequence % REMAPPER_BT_RUNTIME_QUEUE_CAPACITY];
    atomic_store_explicit(
        &g_read_sequence,
        read_sequence + 1u,
        memory_order_release);
    return true;
}

bool remapper_bt_runtime_take_overflow(void)
{
    return atomic_exchange_explicit(&g_overflowed, false, memory_order_acq_rel);
}
