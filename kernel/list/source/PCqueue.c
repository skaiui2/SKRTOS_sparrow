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

#include "PCqueue.h"
#include "sem.h"
#include "heap.h"

/* Oo_buffer is queue struct.
 * one Producer, one consumer.
 * use it write item for array.
 * Oo_insert function: lock the space, and write, then wake up remove task.
 * Oo_remove function: be wake up, then read from array, then unlock array.
 */

Class(Oo_buffer)
{
    uint8_t in;
    uint8_t out;
    uint8_t size;
    Semaphore_Handle item;
    Semaphore_Handle space;
    int buf[];
};



Oo_buffer_handle Oo_buffer_creat(uint8_t buffer_size)
{
    Oo_buffer_handle Oo_buffer1 = heap_malloc(sizeof (Oo_buffer) + sizeof(int) * buffer_size);
    *Oo_buffer1 = (Oo_buffer){
            .in  = 0,
            .out = 0,
            .size = buffer_size,
            .item = semaphore_creat(0),
            .space = semaphore_creat(buffer_size)
    };
    return Oo_buffer1;
}

void Oo_insert(Oo_buffer_handle Oo_buffer1, int object)
{
    semaphore_take(Oo_buffer1->space, 1);
    Oo_buffer1->buf[Oo_buffer1->in] =  object;
    Oo_buffer1->in = (Oo_buffer1->in + 1) % Oo_buffer1->size;
    semaphore_release(Oo_buffer1->item);
}

int Oo_remove(Oo_buffer_handle Oo_buffer1)
{
    semaphore_take(Oo_buffer1->item, 1);
    int item1 = Oo_buffer1->buf[Oo_buffer1->out];
    Oo_buffer1->out = (Oo_buffer1->out + 1) % Oo_buffer1->size;
    semaphore_release(Oo_buffer1->space);
    return item1;
}

void Oo_buffer_delete(Oo_buffer_handle Oo_buffer1)
{
    heap_free(Oo_buffer1);
}


/*
 * many producer, one consumer.
 *
 */

Class(Mo_buffer)
{
    uint8_t in;
    uint8_t out;
    uint8_t size;
    Semaphore_Handle item;
    Semaphore_Handle space;
    Semaphore_Handle guard;
    int buf[];
};



Mo_buffer_handle Mo_buffer_creat(uint8_t buffer_size)
{
    Mo_buffer_handle Mo_buffer1 = heap_malloc(sizeof (Oo_buffer) + sizeof(int) * buffer_size);
    *Mo_buffer1 = (Mo_buffer){
            .in  = 0,
            .out = 0,
            .size = buffer_size,
            .item = semaphore_creat(0),
            .space = semaphore_creat(buffer_size),
            .guard = semaphore_creat(1)
    };
    return Mo_buffer1;
}

void Mo_insert(Mo_buffer_handle Mo_buffer1, int object)
{
    semaphore_take(Mo_buffer1->space, 1);

    semaphore_take(Mo_buffer1->guard, 1);
    Mo_buffer1->buf[Mo_buffer1->in] = object;
    Mo_buffer1->in = (Mo_buffer1->in + 1) % Mo_buffer1->size;
    semaphore_release(Mo_buffer1->guard);

    semaphore_release(Mo_buffer1->item);
}

int Mo_remove(Mo_buffer_handle Mo_buffer1)
{
    semaphore_take(Mo_buffer1->item, 1);
    int item1 = Mo_buffer1->buf[Mo_buffer1->out];
    Mo_buffer1->out = (Mo_buffer1->out + 1) % Mo_buffer1->size;
    semaphore_release(Mo_buffer1->space);
    return item1;
}


void Mo_buffer_delete(Mo_buffer_handle Mo_buffer1)
{
    heap_free(Mo_buffer1);
}



/*
 * many producer, many consumer.
 *
 */

Class(Mm_buffer)
{
    uint8_t in;
    uint8_t out;
    uint8_t size;
    Semaphore_Handle item;
    Semaphore_Handle space;
    Semaphore_Handle guard;
    int buf[];
};



Mm_buffer_handle Mm_buffer_creat(uint8_t buffer_size)
{
    Mm_buffer_handle Mm_buffer1 = heap_malloc(sizeof (Oo_buffer) + sizeof(int) * buffer_size);
    *Mm_buffer1 = (Mm_buffer){
            .in  = 0,
            .out = 0,
            .size = buffer_size,
            .item = semaphore_creat(0),
            .space = semaphore_creat(buffer_size),
            .guard = semaphore_creat(1)
    };
    return Mm_buffer1;
}

void Mm_insert(Mm_buffer_handle Mm_buffer1, int object)
{
    semaphore_take(Mm_buffer1->space, 1);

    semaphore_take(Mm_buffer1->guard, 1);
    Mm_buffer1->buf[Mm_buffer1->in] =  object;
    Mm_buffer1->in = (Mm_buffer1->in + 1) % Mm_buffer1->size;
    semaphore_release(Mm_buffer1->guard);

    semaphore_release(Mm_buffer1->item);
}

int Mm_remove(Mm_buffer_handle Mm_buffer1)
{
    semaphore_take(Mm_buffer1->item, 1);

    semaphore_take(Mm_buffer1->guard, 1);
    int item1 = Mm_buffer1->buf[Mm_buffer1->out];
    Mm_buffer1->out = (Mm_buffer1->out + 1) % Mm_buffer1->size;
    semaphore_release(Mm_buffer1->guard);

    semaphore_release(Mm_buffer1->space);
    return item1;
}


void Mm_buffer_delete(Mm_buffer_handle Mm_buffer1)
{
    heap_free(Mm_buffer1);
}

