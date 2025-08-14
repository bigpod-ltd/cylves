#ifndef SYLVES_HASH_H
#define SYLVES_HASH_H

#include "types.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque hash map for mapping SylvesCell -> int (bucket index) and generic key->value if needed
typedef struct SylvesHash SylvesHash;

SylvesHash* sylves_hash_create(size_t capacity);
void sylves_hash_destroy(SylvesHash* h);
void sylves_hash_clear(SylvesHash* h);

// Map SylvesCell -> int
bool sylves_hash_set_int(SylvesHash* h, const SylvesCell* key, int value);
bool sylves_hash_get_int(const SylvesHash* h, const SylvesCell* key, int* out_value);
bool sylves_hash_remove(SylvesHash* h, const SylvesCell* key);

#ifdef __cplusplus
}
#endif

#endif // SYLVES_HASH_H

