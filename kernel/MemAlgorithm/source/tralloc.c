
/*
 * MIT License
 *
 * Copyright (c) 2024 skaiui2

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *  https://github.com/skaiui2/SKRTOS_sparrow
 */


#include "tralloc.h"
#include "link_list.h"


__attribute__( ( always_inline ) ) inline uint8_t log2_clz(uint32_t Table)
{
    return 31 - __builtin_clz(Table);
}


#define MAX_COUNT   16

#define MIN_size     ((size_t) (HeapStructSize << 1))

Class(TR_node){
    struct  list_node link_node;
    TR_node   *next_block;
    uint16_t block_size;
    char    used;
#define UnUse   0
#define Used    1
};

Class(mem_head){
    struct list_node cache_node;
    size_t AllSize;
};


static mem_head TheHead = {
        .cache_node.next = NULL,
        .cache_node.prev = NULL,
        .AllSize = config_heap,
};

static  uint8_t AllHeap[config_heap];
static const size_t HeapStructSize = (sizeof(TR_node) + (size_t)(alignment_byte)) & (~alignment_byte);

struct radix_tree_root FirstLevel[MAX_COUNT];


void TR_init()
{
    TR_node *first_node;
    PTR_SIZE start_heap;

    for(uint8_t i = 0; i < MAX_COUNT; i++) {
        radix_tree_init(&FirstLevel[i]);
    }
    list_node_init(&(TheHead.cache_node));

    start_heap = (PTR_SIZE) AllHeap;
    if (start_heap & alignment_byte) {
        start_heap += alignment_byte ;
        start_heap &= ~alignment_byte;
        TheHead.AllSize -=  (size_t)(start_heap - (PTR_SIZE)AllHeap);
    }

    first_node = (TR_node *)start_heap;
    *first_node = (TR_node) {
        .used = UnUse,
        .block_size = TheHead.AllSize
    };
    list_add_next(&(TheHead.cache_node), &(first_node->link_node));

    uint8_t fl = log2_clz((uint32_t)TheHead.AllSize);
    radix_tree_insert(&FirstLevel[fl], TheHead.AllSize, first_node);
}


void *TR_alloc(uint16_t WantSize)
{
    TR_node *use_node;
    TR_node *new_node;
    TR_node *next_node;
    void *xReturn;
    uint8_t fl;

    if (WantSize) {
        WantSize += HeapStructSize;
    } else {
        return NULL;
    }

    if (TheHead.cache_node.prev == NULL) {
        TR_init();
    }

    if (WantSize & alignment_byte) {
        WantSize += alignment_byte;
        WantSize = WantSize & (~alignment_byte);
    }

    fl = log2_clz(WantSize);
    while (!FirstLevel[fl].rnode) {
        fl++;
    }
    if (fl >= (MAX_COUNT - 1)) {
        return NULL;
    }

    if (fl != log2_clz(WantSize)) {
        use_node = radix_tree_root_left(&FirstLevel[fl]);
    } else {
        use_node = radix_tree_lookup_upper_bound(&FirstLevel[fl], WantSize);
    }
    if (!use_node) {
        return NULL;
    }
    use_node->used = Used;
    next_node = use_node->next_block;
    radix_tree_delete(&FirstLevel[fl], use_node->block_size);
    if (next_node) {
        radix_tree_insert(&FirstLevel[fl], next_node->block_size, next_node);
    }

    if (use_node->block_size == WantSize) {
        TheHead.AllSize -= WantSize;
        return use_node;
    }

    xReturn = (void *)((uint8_t *)use_node + HeapStructSize);
    if( (use_node->block_size - WantSize) > MIN_size ) {
        new_node = (void *) (((uint8_t *) use_node) + WantSize);
        *new_node = (TR_node) {
                .used = UnUse,
                .block_size = use_node->block_size - WantSize
        };
        use_node->block_size = WantSize;
        list_add_next(&(use_node->link_node), &(new_node->link_node));
        fl = log2_clz((uint32_t) new_node->block_size);
        radix_tree_insert(&FirstLevel[fl], new_node->block_size, new_node);
    }

    TheHead.AllSize -= WantSize;
    return xReturn;
}


void TR_free(void *xReturn)
{
    TR_node *free_node;
    uint8_t fl;
    uint8_t *xFree = (uint8_t *)xReturn;

    xFree -= HeapStructSize;
    free_node = (TR_node *)xFree;
    TheHead.AllSize += free_node->block_size;

    TR_node *adj_node;
    TR_node *insert_node;
    struct list_node *iter_node;
    insert_node = free_node;

    iter_node = free_node->link_node.next;
    if (iter_node != &(TheHead.cache_node)) {
        adj_node = container_of(iter_node, TR_node, link_node);
        if(adj_node->used == UnUse) {
            list_remove(&(adj_node->link_node));
            fl = log2_clz((uint32_t)adj_node->block_size);
            radix_tree_delete(&FirstLevel[fl], adj_node->block_size);
            free_node->block_size += adj_node->block_size;
            insert_node = free_node;
        }
    }

    iter_node = free_node->link_node.prev;
    if (iter_node != &(TheHead.cache_node)) {
        adj_node = container_of(iter_node, TR_node, link_node);
        if(adj_node->used == UnUse) {
            list_remove(&(free_node->link_node));
            fl = log2_clz((uint32_t)adj_node->block_size);
            radix_tree_delete(&FirstLevel[fl], adj_node->block_size);
            adj_node->block_size += free_node->block_size;
            insert_node = adj_node;
        }
    }

    insert_node->used = UnUse;
    insert_node->next_block = NULL;
    fl = log2_clz((uint32_t)insert_node->block_size);
    radix_tree_insert(&FirstLevel[fl], insert_node->block_size, insert_node);
}
