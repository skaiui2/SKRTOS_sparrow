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

#include "RWlock.h"
#include "sem.h"
#include "heap.h"


/*
 * many reader, many writer.
 *
 */

Class(rwlock)
{
    Semaphore_Handle read;
    Semaphore_Handle write;
    Semaphore_Handle W_guard;
    Semaphore_Handle C_guard;
    int active_reader;
    int reading_reader;
    int active_writer;
    int writing_writer;
};

rwlock_handle rwlock_creat(void)
{
    rwlock_handle rwlock1 = heap_malloc(sizeof (rwlock));
    *rwlock1 = (rwlock){
            .read = semaphore_creat(0),
            .write = semaphore_creat(0),
            .W_guard = semaphore_creat(1),
            .C_guard = semaphore_creat(1),
            .active_reader = 0,
            .reading_reader = 0,
            .active_writer = 0,
            .writing_writer = 0
    };
    return rwlock1;
}

void read_acquire(rwlock_handle rwlock1)
{
    semaphore_take(rwlock1->C_guard, 1);
    rwlock1->active_reader += 1;
    if(rwlock1->active_writer == 0){
        rwlock1->reading_reader += 1;
        semaphore_release(rwlock1->read);
    }
    semaphore_release(rwlock1->C_guard);
    semaphore_take(rwlock1->read, 1);
}

void read_release(rwlock_handle rwlock1)
{
    semaphore_take(rwlock1->C_guard, 1);
    rwlock1->reading_reader -= 1;
    rwlock1->active_reader -= 1;
    if(rwlock1->reading_reader == 0){
        while(rwlock1->writing_writer < rwlock1->active_writer){
            rwlock1->writing_writer += 1;
            semaphore_release(rwlock1->write);
        }
    }
    semaphore_release(rwlock1->C_guard);
}

void write_acquire(rwlock_handle rwlock1)
{
    semaphore_take(rwlock1->C_guard, 1);
    rwlock1->active_writer += 1;
    if(rwlock1->reading_reader == 0){
        rwlock1->writing_writer += 1;
        semaphore_release(rwlock1->write);
    }
    semaphore_release(rwlock1->C_guard);
    semaphore_take(rwlock1->write, 1);

    semaphore_take(rwlock1->W_guard, 1);
}


void write_release(rwlock_handle rwlock1)
{
    semaphore_release(rwlock1->W_guard);

    semaphore_take(rwlock1->C_guard, 1);
    rwlock1->writing_writer -= 1;
    rwlock1->active_writer -= 1;
    if(rwlock1->active_writer == 0){
        while(rwlock1->reading_reader < rwlock1->active_reader){
            rwlock1->reading_reader += 1;
            semaphore_release(rwlock1->read);
        }
    }
    semaphore_release(rwlock1->C_guard);
}

void rwlock_delete(rwlock_handle rwlock1)
{
    heap_free(rwlock1);
}
