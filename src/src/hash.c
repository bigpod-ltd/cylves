#include "sylves/hash.h"
#include "sylves/memory.h"
#include <string.h>
#include <stdint.h>

// Open addressing hash map specialized for SylvesCell->int
// Production-ready: linear probing, tombstones, growth, 64-bit mix hash.

typedef struct {
    SylvesCell key;
    int value;
    uint8_t state; // 0=empty, 1=filled, 2=tombstone
} Entry;

struct SylvesHash {
    Entry* entries;
    size_t capacity; // power of two
    size_t size;
};

static inline uint64_t mix64(uint64_t x){
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31; return x;
}

static uint64_t cell_hash_key(const SylvesCell* c){
    uint64_t x = (uint32_t)c->x;
    uint64_t y = (uint32_t)c->y;
    uint64_t z = (uint32_t)c->z;
    uint64_t h = x * 0x9e3779b97f4a7c15ULL ^ y * 0xc2b2ae3d27d4eb4fULL ^ z * 0x165667b19e3779f9ULL;
    return mix64(h);
}

static bool cell_eq(const SylvesCell* a, const SylvesCell* b){
    return a->x==b->x && a->y==b->y && a->z==b->z;
}

static bool ensure_capacity(SylvesHash* h, size_t min_free);

SylvesHash* sylves_hash_create(size_t capacity){
    if (capacity < 16) capacity = 16;
    // round to power of two
    size_t cap = 1; while (cap < capacity) cap <<= 1;
    SylvesHash* h = (SylvesHash*)sylves_alloc(sizeof(SylvesHash));
    if(!h) return NULL;
    h->entries = (Entry*)sylves_calloc(cap, sizeof(Entry));
    if(!h->entries){ sylves_free(h); return NULL; }
    h->capacity = cap;
    h->size = 0;
    return h;
}

void sylves_hash_destroy(SylvesHash* h){
    if(!h) return;
    sylves_free(h->entries);
    sylves_free(h);
}

void sylves_hash_clear(SylvesHash* h){
    if(!h) return;
    memset(h->entries, 0, h->capacity * sizeof(Entry));
    h->size = 0;
}

static bool insert_impl(SylvesHash* h, const SylvesCell* key, int value, bool replace){
    if (!ensure_capacity(h, 1)) return false;
    uint64_t mask = (uint64_t)h->capacity - 1;
    uint64_t idx = cell_hash_key(key) & mask;
    ssize_t tomb = -1;
    for(;;){
        Entry* e = &h->entries[idx];
        if(e->state == 0){
            size_t use = (tomb >= 0) ? (size_t)tomb : (size_t)idx;
            Entry* t = &h->entries[use];
            t->key = *key; t->value = value; t->state = 1; h->size++;
            return true;
        } else if(e->state == 2){
            if (tomb < 0) tomb = (ssize_t)idx;
        } else if(e->state == 1){
            if(cell_eq(&e->key, key)){
                if (replace){ e->value = value; }
                return true;
            }
        }
        idx = (idx + 1) & mask;
    }
}

static bool ensure_capacity(SylvesHash* h, size_t min_free){
    if ((h->size + min_free) * 2 < h->capacity) return true;
    size_t newcap = h->capacity << 1;
    Entry* old = h->entries;
    size_t oldcap = h->capacity;
    Entry* ne = (Entry*)sylves_calloc(newcap, sizeof(Entry));
    if(!ne) return false;
    h->entries = ne; h->capacity = newcap; h->size = 0;
    for(size_t i=0;i<oldcap;i++){
        if(old[i].state == 1){ insert_impl(h, &old[i].key, old[i].value, true); }
    }
    sylves_free(old);
    return true;
}

bool sylves_hash_set_int(SylvesHash* h, const SylvesCell* key, int value){
    return insert_impl(h, key, value, true);
}

bool sylves_hash_get_int(const SylvesHash* hc, const SylvesCell* key, int* out_value){
    SylvesHash* h = (SylvesHash*)hc;
    if(h->capacity == 0) return false;
    uint64_t mask = (uint64_t)h->capacity - 1;
    uint64_t idx = cell_hash_key(key) & mask;
    for(;;){
        Entry* e = &h->entries[idx];
        if(e->state == 0) return false;
        if(e->state == 1 && cell_eq(&e->key, key)){
            if(out_value) *out_value = e->value;
            return true;
        }
        idx = (idx + 1) & mask;
    }
}

bool sylves_hash_remove(SylvesHash* h, const SylvesCell* key){
    if(h->capacity == 0) return false;
    uint64_t mask = (uint64_t)h->capacity - 1;
    uint64_t idx = cell_hash_key(key) & mask;
    for(;;){
        Entry* e = &h->entries[idx];
        if(e->state == 0) return false;
        if(e->state == 1 && cell_eq(&e->key, key)){
            e->state = 2; h->size--; return true;
        }
        idx = (idx + 1) & mask;
    }
}

