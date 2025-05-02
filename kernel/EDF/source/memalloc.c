
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


#include "memalloc.h"
#include "rbtree.h"
#include "link_list.h"

#define MIN_size     ((size_t) (HeapStructSize << 1))

Class(heap_node){
    struct    list_node link_node;
    rb_node   iter_node;
    char    used;
    #define UnUse   0
    #define Used    1
};

Class(xheap){
    struct list_node cache_node;
    size_t AllSize;
};

xheap TheHeap = {
        .cache_node.next = NULL,
        .cache_node.prev = NULL,
        .AllSize = config_heap,
};

static  uint8_t AllHeap[config_heap];
static const size_t HeapStructSize = (sizeof(heap_node) + (size_t)(alignment_byte)) & (~alignment_byte);

static rb_root MemTree;
static void InsertFreeBlock(heap_node* free_node);

/*
 *
 *
 */
void mem_init( void )
{
    heap_node *first_node;
    PTR_SIZE start_heap;

    rb_root_init(&MemTree);
    list_node_init(&(TheHeap.cache_node));

    start_heap = (PTR_SIZE) AllHeap;
    if((start_heap & alignment_byte) != 0){
        start_heap += alignment_byte ;
        start_heap &= ~alignment_byte;
        TheHeap.AllSize -=  (size_t)(start_heap - (PTR_SIZE)AllHeap);
    }

    first_node = (heap_node *)start_heap;

    list_node_init(&(first_node->link_node));
    list_add_next(&(TheHeap.cache_node), &(first_node->link_node));

    rb_node_init(&(first_node->iter_node));
    first_node->iter_node.value = TheHeap.AllSize;
    first_node->used = UnUse;
    rb_Insert_node(&MemTree, &(first_node->iter_node));
}


/*
 *
 *
 */
void *mem_malloc(size_t WantSize)
{
    heap_node *use_node;
    heap_node *new_node;
    rb_node   *find_node;
    size_t alignment_require_size;
    size_t block_size;
    void *xReturn = NULL;

    if (WantSize) {
        WantSize += HeapStructSize;
    } else {
        goto free;
    }
    if((WantSize & alignment_byte) != 0x00) {
        alignment_require_size = (alignment_byte + 1) - (WantSize & alignment_byte);
        WantSize += alignment_require_size;
    }
    if(TheHeap.cache_node.prev == NULL) {
        mem_init();
    }

    if (((find_node = MemTree.last_node) == NULL)
        || (find_node->value < WantSize)) {
        goto free;
    }
    if ((find_node = MemTree.first_node) == NULL) {
        goto free;
    }

    while (find_node->value < WantSize) {
        find_node = rb_next(find_node);
        if (find_node == NULL) {
            goto free;
        }
    }

    block_size = find_node->value;
    rb_remove_node(&MemTree, find_node);

    use_node = container_of(find_node, heap_node, iter_node);
    use_node->used = Used;

    xReturn = (void*)((uint8_t *)use_node + HeapStructSize);

    if ((find_node->value - WantSize) > MIN_size) {
        new_node = (void *) (((uint8_t *) use_node) + WantSize);
        new_node->used = UnUse;
        use_node->iter_node.value = WantSize;

        list_add_next(&(use_node->link_node), &(new_node->link_node));

        new_node->iter_node.value = block_size - WantSize;
        rb_Insert_node(&MemTree, &(new_node->iter_node));
    }
    TheHeap.AllSize -= use_node->iter_node.value;

    free:
    return xReturn;
}


void mem_free(void *xReturn)
{
    heap_node *free_node;
    uint8_t *xFree = (uint8_t *)xReturn;
    xFree -= HeapStructSize;
    free_node = (void*)xFree;
    free_node->used = UnUse;
    TheHeap.AllSize += free_node->iter_node.value;

    heap_node *adj_node;
    heap_node *insert_node;
    struct list_node *iter_node;
    insert_node = free_node;

    iter_node = free_node->link_node.next;
    if (iter_node != &(TheHeap.cache_node)) {
        adj_node = container_of(iter_node, heap_node, link_node);
        if(adj_node->used == UnUse) {
            list_remove(&(adj_node->link_node));
            free_node->iter_node.value += adj_node->iter_node.value;
            rb_remove_node(&MemTree, &(adj_node->iter_node));
            insert_node = free_node;
        }
    }

    iter_node = free_node->link_node.prev;
    if (iter_node != &(TheHeap.cache_node)) {
        adj_node = container_of(iter_node, heap_node, link_node);
        if(adj_node->used == UnUse) {
            list_remove(&(free_node->link_node));
            adj_node->iter_node.value += free_node->iter_node.value;
            rb_remove_node(&MemTree, &(adj_node->iter_node));
            insert_node = adj_node;
        }
    }

    rb_Insert_node(&MemTree, &(insert_node->iter_node));
}
