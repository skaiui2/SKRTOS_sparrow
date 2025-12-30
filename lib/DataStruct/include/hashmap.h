#ifndef HASHMAP_H
#define HASHMAP_H

#include "link_list.h"
#include <stddef.h>
#include <stdint.h>

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#if UINTPTR_MAX == 0xffffffff
    #define PLATFORM_BITS 32
    typedef uint32_t uword_t;
    #define CLZ(x) __builtin_clz(x)
    #define GOLDEN_RATIO_PRIME 0x9e370001U
#elif UINTPTR_MAX == 0xffffffffffffffff
    #define PLATFORM_BITS 64
    typedef uint64_t uword_t;
    #define CLZ(x) __builtin_clzll(x)
    #define GOLDEN_RATIO_PRIME 0x9e37fffffffc0001ULL
#else
    #error "Unsupported platform"
#endif

#define HASHMAP_KEY_STRING 1
#define HASHMAP_KEY_INT    2
#define HASHMAP_KEY_PTR    3

struct hashmap_entry {
    struct list_node node;
    void *key;
    void *value;
};

struct hashmap {
    struct list_node *buckets;
    uword_t bucket_count;
    int key_type;
};

void hashmap_init(struct hashmap *map, size_t bucket_count, int key_type);
void hashmap_put(struct hashmap *map, void *key, void *value);
void *hashmap_get(struct hashmap *map, void *key);
int hashmap_remove(struct hashmap *map, void *key);

#endif
