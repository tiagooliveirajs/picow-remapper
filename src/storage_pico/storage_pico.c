#include "remapper/storage_pico/storage_pico.h"

#if !defined(REMAPPER_PICO_STORAGE) || !REMAPPER_PICO_STORAGE

bool remapper_storage_pico_read(uint32_t key, void *data, size_t length)
{
    (void)key;
    (void)data;
    (void)length;
    return false;
}

bool remapper_storage_pico_write(uint32_t key, const void *data, size_t length)
{
    (void)key;
    (void)data;
    (void)length;
    return false;
}

#else

#include <stddef.h>
#include <string.h>

#include "hardware/flash.h"
#include "pico/flash.h"
#include "pico/platform.h"

#define STORAGE_MAGIC UINT32_C(0x4750524d) /* MRPG */
#define STORAGE_VERSION 1u
#define STORAGE_RECORD_COUNT 8u
#define STORAGE_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define STORAGE_FLASH_TIMEOUT_MS 1000u

typedef struct {
    uint32_t key;
    uint8_t length;
    uint8_t data[REMAPPER_STORAGE_PICO_VALUE_MAX];
} storage_record_t;

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t reserved[3];
    storage_record_t records[STORAGE_RECORD_COUNT];
    uint32_t checksum;
} storage_table_t;

_Static_assert(sizeof(storage_table_t) <= FLASH_PAGE_SIZE,
               "profile storage table must fit one flash page");

static uint32_t checksum_bytes(const uint8_t *data, size_t length)
{
    uint32_t value = UINT32_C(2166136261);
    for (size_t index = 0u; index < length; ++index) {
        value ^= data[index];
        value *= UINT32_C(16777619);
    }
    return value;
}

static uint32_t table_checksum(const storage_table_t *table)
{
    return checksum_bytes((const uint8_t *)table, offsetof(storage_table_t, checksum));
}

static bool table_load(storage_table_t *table)
{
    if (table == NULL) return false;
    const storage_table_t *flash_table =
        (const storage_table_t *)(XIP_BASE + STORAGE_FLASH_OFFSET);
    memcpy(table, flash_table, sizeof(*table));
    return table->magic == STORAGE_MAGIC &&
        table->version == STORAGE_VERSION &&
        table->checksum == table_checksum(table);
}

static void table_init(storage_table_t *table)
{
    memset(table, 0, sizeof(*table));
    table->magic = STORAGE_MAGIC;
    table->version = STORAGE_VERSION;
    table->checksum = table_checksum(table);
}

static int find_record(const storage_table_t *table, uint32_t key)
{
    for (unsigned int index = 0u; index < STORAGE_RECORD_COUNT; ++index) {
        if (table->records[index].length != 0u &&
            table->records[index].key == key) {
            return (int)index;
        }
    }
    return -1;
}

static int find_free_record(const storage_table_t *table)
{
    for (unsigned int index = 0u; index < STORAGE_RECORD_COUNT; ++index) {
        if (table->records[index].length == 0u) return (int)index;
    }
    return -1;
}

bool remapper_storage_pico_read(uint32_t key, void *data, size_t length)
{
    if (key == 0u || data == NULL || length == 0u ||
        length > REMAPPER_STORAGE_PICO_VALUE_MAX) {
        return false;
    }

    storage_table_t table;
    if (!table_load(&table)) return false;
    const int slot = find_record(&table, key);
    if (slot < 0 || table.records[slot].length != length) return false;
    memcpy(data, table.records[slot].data, length);
    return true;
}

typedef struct {
    uint32_t offset;
    uint8_t page[FLASH_PAGE_SIZE];
} flash_write_params_t;

static void flash_write_table(void *context)
{
    flash_write_params_t *params = context;
    flash_range_erase(params->offset, FLASH_SECTOR_SIZE);
    flash_range_program(params->offset, params->page, FLASH_PAGE_SIZE);
}

bool remapper_storage_pico_write(uint32_t key, const void *data, size_t length)
{
    if (key == 0u || data == NULL || length == 0u ||
        length > REMAPPER_STORAGE_PICO_VALUE_MAX) {
        return false;
    }

    storage_table_t table;
    if (!table_load(&table)) table_init(&table);

    int slot = find_record(&table, key);
    if (slot < 0) slot = find_free_record(&table);
    if (slot < 0) return false;

    storage_record_t *record = &table.records[slot];
    memset(record, 0, sizeof(*record));
    record->key = key;
    record->length = (uint8_t)length;
    memcpy(record->data, data, length);
    table.checksum = table_checksum(&table);

    flash_write_params_t params;
    params.offset = STORAGE_FLASH_OFFSET;
    memset(params.page, 0xff, sizeof(params.page));
    memcpy(params.page, &table, sizeof(table));

    return flash_safe_execute(
        flash_write_table,
        &params,
        STORAGE_FLASH_TIMEOUT_MS) == PICO_OK;
}

#endif
