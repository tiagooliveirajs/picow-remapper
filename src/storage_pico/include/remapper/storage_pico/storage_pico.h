#ifndef REMAPPER_STORAGE_PICO_STORAGE_PICO_H
#define REMAPPER_STORAGE_PICO_STORAGE_PICO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REMAPPER_STORAGE_PICO_VALUE_MAX 16u

bool remapper_storage_pico_read(uint32_t key, void *data, size_t length);
bool remapper_storage_pico_write(uint32_t key, const void *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif
