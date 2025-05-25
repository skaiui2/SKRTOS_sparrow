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

#include "membit.h"
#include "heap.h"

__attribute__( ( always_inline ) ) inline uint8_t log2_low_ctz(uint32_t Table)
{
    return __builtin_ctz(Table);
}


Class(PoolHead)
{
    uint32_t bitmask;
    size_t BlockSize;
};

static const size_t HeadStructSize = (sizeof(PoolHead) + (size_t)(alignment_byte)) &~(alignment_byte);


PoolHeadHandle mempool_creat(uint16_t size,uint8_t amount)
{
    PoolHead *ThePool;
    size_t all_size;
    if (size & alignment_byte) {
        size += alignment_byte;
        size &= (~alignment_byte);
    }
    all_size = size * amount;
    all_size += HeadStructSize;

    ThePool = heap_malloc(all_size);
    if (ThePool == NULL){
        return false;
    }

    ThePool->BlockSize = size;
    ThePool->bitmask = 0;
    ThePool->bitmask |= ((1 << amount) - 1);
    return ThePool;
}


void *mempool_alloc(PoolHeadHandle ThePool)
{
    void *address = NULL;
    uint8_t index;
    if (!ThePool) {
        return address;
    }

    if (!ThePool->bitmask) {
        return address;
    }

    index = log2_low_ctz(ThePool->bitmask);
    ThePool->bitmask &= ~(1 << index);
    address = (void *)((size_t)ThePool + HeadStructSize);
    address = address + (ThePool->BlockSize * index);

    return address;
}



void mempool_free(PoolHeadHandle ThePool, void *address)
{
    uint8_t index;
    if (!address) {
        return;
    }

    index = ((size_t)address - (size_t)ThePool - (size_t)HeadStructSize) / (size_t)ThePool->BlockSize;
    ThePool->bitmask |= (1 << index);
}

void mempool_delete(PoolHeadHandle ThePool)
{
    if (!ThePool) {
        return;
    }
    heap_free(ThePool);
}
