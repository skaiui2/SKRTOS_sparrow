
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
    size_t block_size;
    char    used;
#define UnUse   0
#define Used    1
};

Class(mem_head){
    struct list_node cache_node;
    size_t AllSize;
};


mem_head TheHead = {
        .cache_node.next = NULL,
        .cache_node.prev = NULL,
        .AllSize = config_heap,
};

static  uint8_t AllHeap[config_heap];
static const size_t HeapStructSize = (sizeof(TR_node) + (size_t)(alignment_byte)) & (~alignment_byte);

struct radix_tree_root MemRadixTree;


#define PTR_SIZE    uint64_t
#define MIN_size     ((size_t) (HeapStructSize << 1))


void TR_init()
{
    TR_node *first_node;
    PTR_SIZE start_heap;
    radix_tree_init(&MemRadixTree);
    list_node_init(&(TheHead.cache_node));

    start_heap = (PTR_SIZE) AllHeap;
    if (start_heap & alignment_byte) {
        start_heap = (start_heap + alignment_byte) & (~alignment_byte);
        TheHead.AllSize -= (size_t)(start_heap - (PTR_SIZE)AllHeap);
    }

    first_node = (TR_node *)start_heap;
    *first_node = (TR_node) {
        .used = UnUse,
        .next_block = NULL,
        .block_size = TheHead.AllSize
    };
    list_add_next(&(TheHead.cache_node), &(first_node->link_node));
    radix_tree_insert(&MemRadixTree, TheHead.AllSize, first_node);
}


int mem_node_delete(TR_node *delete_node)
{
    TR_node *head = radix_tree_lookup(&MemRadixTree, delete_node->block_size);
    if (!head) {
        return -1;
    }

    TR_node *next_node = head->next_block;
    if (head == delete_node) {
        radix_tree_delete(&MemRadixTree, head->block_size);
        if (next_node) {
            radix_tree_insert(&MemRadixTree, next_node->block_size, next_node);
        }
    } else {
        TR_node *prev_node = head;
        while (next_node && (next_node != delete_node)) {
            prev_node = next_node;
            next_node = next_node->next_block;
        }

        if (!next_node) {
            return -1;
        }
        prev_node->next_block = next_node->next_block;
    }
    return 0;
}

int mem_node_insert(TR_node *insert_node)
{
    TR_node *head = radix_tree_lookup(&MemRadixTree, insert_node->block_size);
    if (head) {
        insert_node->next_block = head;
        radix_tree_delete(&MemRadixTree, head->block_size);
    } else {
        insert_node->next_block = NULL;
    }

    radix_tree_insert(&MemRadixTree, insert_node->block_size, insert_node);
    return 0;
}

void *TR_alloc(size_t WantSize)
{
    TR_node *use_node;
    TR_node *new_node;
    void *xReturn = NULL;

    if (WantSize) {
        WantSize += HeapStructSize;
    } else {
        goto free;
    }

    if ((TheHead.cache_node.prev == NULL)
    && (TheHead.cache_node.next == NULL)) {
        TR_init();
    }

    if (!MemRadixTree.rnode) {
        goto free;
    }

    if (WantSize & alignment_byte) {
        WantSize = (WantSize + alignment_byte) & (~alignment_byte);
    }

    use_node = radix_tree_lookup_upper_bound(&MemRadixTree, WantSize);
    if (!use_node) {
        goto free;
    }
    use_node->used = Used;
    if(mem_node_delete(use_node) == 0) {
        use_node->next_block = NULL;
    } else {
        goto free;
    }

    xReturn = (void *)((uint8_t *)use_node + HeapStructSize);
    if (use_node->block_size == WantSize) {
        TheHead.AllSize -= WantSize;
        return xReturn;
    }

    if ((use_node->block_size - WantSize) > MIN_size ) {
        new_node = (void *) (((uint8_t *) use_node) + WantSize);
        *new_node = (TR_node) {
                .used = UnUse,
                .next_block = NULL,
                .block_size = use_node->block_size - WantSize
        };
        use_node->block_size = WantSize;
        list_add_next(&(use_node->link_node), &(new_node->link_node));
        mem_node_insert(new_node);
    }

    TheHead.AllSize -= WantSize;

    free:
    return xReturn;
}


void TR_free(void *xReturn)
{
    TR_node *free_node;
    TR_node *adj_node;
    TR_node *insert_node;
    struct list_node *iter_node;
    if (!xReturn) {
        return;
    }

    uint8_t *xFree = (uint8_t *)xReturn;
    xFree -= HeapStructSize;
    free_node = (TR_node *)xFree;
    insert_node = free_node;
    TheHead.AllSize += free_node->block_size;

    iter_node = free_node->link_node.next;
    if (iter_node && (iter_node != &TheHead.cache_node)) {
        adj_node = container_of(iter_node, TR_node, link_node);
        if (adj_node->used == UnUse) {
            list_remove(&(adj_node->link_node));
            mem_node_delete(adj_node);
            free_node->block_size += adj_node->block_size;
            insert_node = free_node;
        }
    }

    iter_node = free_node->link_node.prev;
    if (iter_node && (iter_node != &TheHead.cache_node)) {
        adj_node = container_of(iter_node, TR_node, link_node);
        if(adj_node->used == UnUse) {
            list_remove(&(free_node->link_node));
            mem_node_delete(adj_node);
            adj_node->block_size += free_node->block_size;
            insert_node = adj_node;
        }
    }

    insert_node->used = UnUse;
    insert_node->next_block = NULL;
    mem_node_insert(insert_node);
}
