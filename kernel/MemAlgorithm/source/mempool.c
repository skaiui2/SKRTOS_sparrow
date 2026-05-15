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

#include "mempool.h"
#include "heap.h"

Class(PoolNode)
{
    uint8_t used;
    struct list_node free_node;
    PoolNode *next;
};

Class(PoolHead)
{
    PoolNode *head;
    struct list_node free_list;
    size_t BlockSize;
    size_t AllCount;
    uint8_t RemainNode;
};

static const size_t NodeStructSize = (sizeof(PoolNode) + (size_t)(alignment_byte)) &~(alignment_byte);
static const size_t HeadStructSize = (sizeof(PoolHead) + (size_t)(alignment_byte)) &~(alignment_byte);


void pool_apart(PoolHead *ThePool, uint8_t amount,size_t apart_size)
{
    PoolNode *prev_node;
    PoolNode *new_node;

    prev_node = ThePool->head;
    while(amount != 0) {
        new_node = (PoolNode *) (((size_t) prev_node) + apart_size);
        new_node->used = 0;
        list_add_next(&prev_node->free_node, &new_node->free_node);
        prev_node->next = new_node;
        prev_node = new_node;
        amount--;
    }
    new_node->next = NULL;
}


PoolHeadHandle memPool_creat(uint16_t size,uint8_t amount)
{
    size_t alignment_require_size;
    size_t apart_size = size;
    uint32_t all_size;
    void *start_address;
    PoolHead *ThePool;

    apart_size += NodeStructSize;
    if(apart_size & alignment_byte) {
        alignment_require_size = alignment_byte + 1 - (apart_size & alignment_byte);
        apart_size += alignment_require_size;
    }

    all_size = (uint32_t)( amount * ((uint32_t)apart_size));
    all_size += HeadStructSize;
    start_address =heap_malloc(all_size);
    if (start_address == NULL){
        return false;
    }
    ThePool = (PoolHead *)start_address;
    *ThePool = (PoolHead){
            .head = (PoolNode *)((size_t)start_address + HeadStructSize),
            .BlockSize = size,
            .AllCount = amount,
            .RemainNode = amount
    };
    ThePool->head->used = 0;
    list_node_init(&ThePool->free_list);
    PoolNode *first_node = (PoolNode *)ThePool->head;
    list_add_next(&ThePool->free_list, &first_node->free_node);
    pool_apart(ThePool,amount - 1,apart_size);

    return ThePool;
}


void *memPool_apl(PoolHeadHandle ThePool)
{
    PoolNode *use_node;
    void *xReturn = NULL;

    if (!ThePool) {
        return xReturn;
    }
    if (ThePool->free_list.next != &ThePool->free_list) {
        use_node = container_of(ThePool->free_list.next, PoolNode, free_node);
        list_remove(ThePool->free_list.next);
    } else {
        return xReturn;
    }

    if (use_node->used == 0) {
        use_node->used = 1;
        ThePool->RemainNode -= 1;
        xReturn = (void *) (((uint8_t *) use_node) + NodeStructSize);
    }
    return xReturn;
}



void memPool_free(PoolHeadHandle ThePool, void *xRet)
{
    PoolNode *FreeBlock;
    PoolNode *find_node;
    if (!xRet) {
        return;
    }

    void * xFree = (void*)((size_t)xRet - NodeStructSize);
    FreeBlock = (void*)xFree;
    FreeBlock->used = 0;
    find_node = FreeBlock->next;
    while (find_node && find_node->used != 0) {
        find_node = find_node->next;
    }

    if (find_node) {
        list_add_prev(&find_node->free_node, &FreeBlock->free_node);
    } else {
        list_add_prev(&ThePool->free_list, &FreeBlock->free_node);
    }
}

void memPool_delete(PoolHeadHandle ThePool)
{
    if (ThePool) {
        heap_free(ThePool);
    }
}
