#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

__attribute__((always_inline)) inline uword_t next_power_of_two(uword_t x)
{
    if (x <= 1)
        return 1;
    return (uword_t)1 << (PLATFORM_BITS - CLZ(x - 1));
}

__attribute__((always_inline)) inline uword_t hash_long(uword_t val)
{
    return val * GOLDEN_RATIO_PRIME;
}

static uword_t hashmap_hash(struct hashmap *map, void *key)
{
    switch (map->key_type) {
        case HASHMAP_KEY_STRING: {
            char *s = key;
            uword_t h = 0;
            while (*s)
                h = h * GOLDEN_RATIO_PRIME + (unsigned char)(*s++);
            return h;
        }
        case HASHMAP_KEY_INT:
        case HASHMAP_KEY_PTR:
            return hash_long((uword_t)key);
        default:
            return 0;
    }
}

static int hashmap_key_equal(struct hashmap *map, void *a, void *b)
{
    switch (map->key_type) {
        case HASHMAP_KEY_STRING:
            return strcmp((char *)a, (char *)b) == 0;
        case HASHMAP_KEY_INT:
        case HASHMAP_KEY_PTR:
            return a == b;
        default:
            return 0;
    }
}

void hashmap_init(struct hashmap *map, size_t bucket_count, int key_type)
{
    uword_t count = next_power_of_two((uword_t)bucket_count);
    map->bucket_count = count;
    map->key_type = key_type;

    map->buckets = malloc(sizeof(struct list_node) * count);
    for (uword_t i = 0; i < count; i++)
        list_node_init(&map->buckets[i]);
}

static struct list_node *hashmap_bucket(struct hashmap *map, void *key)
{
    uword_t h = hashmap_hash(map, key);
    return &map->buckets[h & (map->bucket_count - 1)];
}

void hashmap_put(struct hashmap *map, void *key, void *value)
{
    struct list_node *bucket = hashmap_bucket(map, key);
    struct list_node *p;

    for (p = bucket->next; p != bucket; p = p->next) {
        struct hashmap_entry *e = container_of(p, struct hashmap_entry, node);
        if (hashmap_key_equal(map, e->key, key)) {
            e->value = value;
            return;
        }
    }

    struct hashmap_entry *entry = malloc(sizeof(struct hashmap_entry));
    entry->key = key;
    entry->value = value;
    list_node_init(&entry->node);
    list_add_next(bucket, &entry->node);
}

void *hashmap_get(struct hashmap *map, void *key)
{
    struct list_node *bucket = hashmap_bucket(map, key);
    struct list_node *p;

    for (p = bucket->next; p != bucket; p = p->next) {
        struct hashmap_entry *e = container_of(p, struct hashmap_entry, node);
        if (hashmap_key_equal(map, e->key, key))
            return e->value;
    }
    return NULL;
}

int hashmap_remove(struct hashmap *map, void *key)
{
    struct list_node *bucket = hashmap_bucket(map, key);
    struct list_node *p;

    for (p = bucket->next; p != bucket; p = p->next) {
        struct hashmap_entry *e = container_of(p, struct hashmap_entry, node);
        if (hashmap_key_equal(map, e->key, key)) {
            list_remove(&e->node);
            free(e);
            return 1;
        }
    }
    return 0;
}
