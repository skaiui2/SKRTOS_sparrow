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

#include "mempool_dy.h"
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
        new_node = heap_malloc(apart_size);
        new_node->used = 0;
        list_add_next(&prev_node->free_node, &new_node->free_node);
        prev_node->next = new_node;
        prev_node = new_node;
        amount--;
    }
    if (new_node) {
        new_node->next = NULL;
    }
}


PoolHeadHandle memPool_creat(uint16_t size,uint8_t amount)
{
    PoolHead *ThePool;

    ThePool = heap_malloc(HeadStructSize);
    if (ThePool == NULL){
        return false;
    }
    *ThePool = (PoolHead){
            .BlockSize = size,
            .AllCount = amount,
            .RemainNode = amount
    };
    list_node_init(&ThePool->free_list);
    size += NodeStructSize;
    ThePool->head = heap_malloc(size);
    PoolNode *first_node = ThePool->head;
    *first_node = (PoolNode) {
        .used = 0,
    };
    list_add_next(&ThePool->free_list, &first_node->free_node);
    pool_apart(ThePool,amount - 1,size);

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

    if(use_node->used == 0) {
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

void memPool_delete_node(PoolHeadHandle ThePool, void *address)
{
    PoolNode *free_node;
    PoolNode *prev_node;
    PoolNode *next_node;
    if (!address) {
        return;
    }
    address -= NodeStructSize;
    free_node = address;
    next_node = free_node->next;
    list_remove(&free_node->free_node);
    prev_node = address - (ThePool->BlockSize + NodeStructSize);
    prev_node->next = next_node;

    heap_free(address);
}

void memPool_delete_all(PoolHeadHandle ThePool)
{
    PoolNode *next_node;
    PoolNode *free_node;
    if (ThePool) {
        free_node = ThePool->head;
        for (uint16_t amount = ThePool->AllCount; amount > 0; amount--) {
            next_node = free_node->next;
            heap_free(free_node);
            free_node = next_node;
        }
        heap_free(ThePool);
    }

}