//
// Created by el on 2024/11/21.
//

#include "sem_app.h"
#include "sem.h"
#include "schedule.h"
#include "heap.h"

/* Oo_buffer is queue struct.This is a template that use sem.
 * one Producer, one consumer.
 * use it write item for array.
 * Oo_insert function: lock the space, and write, then wake up remove task.
 * Oo_remove function: be wake up, then read from array, then unlock array.
 */
#define BufferSIZE 5
Class(Oo_buffer)
{
    int buf[BufferSIZE];
    int in;
    int out;
    Semaphore_Handle item;
    Semaphore_Handle space;
};



Oo_buffer_handle Oo_buffer_creat(void)
{
    Oo_buffer_handle Oo_buffer1 = heap_malloc(sizeof (Oo_buffer));
    *Oo_buffer1 = (Oo_buffer){
            .buf = {0,0,0,0,0},
            .in  = 0,
            .out = 0,
            .item = semaphore_creat(0),
            .space = semaphore_creat(BufferSIZE)
    };
    return Oo_buffer1;
}

void Oo_insert(Oo_buffer_handle Oo_buffer1, int object)
{
    semaphore_take(Oo_buffer1->space, 1);
    Oo_buffer1->buf[Oo_buffer1->in] =  object;
    Oo_buffer1->in = (Oo_buffer1->in + 1) % BufferSIZE;
    semaphore_release(Oo_buffer1->item);
}

int Oo_remove(Oo_buffer_handle Oo_buffer1)
{
    semaphore_take(Oo_buffer1->item, 1);
    int item1 = Oo_buffer1->buf[Oo_buffer1->out];
    Oo_buffer1->out = (Oo_buffer1->out + 1) % BufferSIZE;
    semaphore_release(Oo_buffer1->space);
    return item1;
}

/* many producer, one consumer.
 * 
 */

Class(Mo_buffer)
{
    int buf[BufferSIZE];
    int in;
    int out;
    Semaphore_Handle item;
    Semaphore_Handle space;
    Semaphore_Handle guard;
};



Mo_buffer_handle Mo_buffer_creat(void)
{
    Mo_buffer_handle Mo_buffer1 = heap_malloc(sizeof (Mo_buffer));
    *Mo_buffer1 = (Mo_buffer){
            .buf = {0,0,0,0,0},
            .in  = 0,
            .out = 0,
            .item = semaphore_creat(0),
            .space = semaphore_creat(BufferSIZE),
            .guard = semaphore_creat(1)
    };
    return Mo_buffer1;
}

void Mo_insert(Mo_buffer_handle Mo_buffer1, int object)
{
    semaphore_take(Mo_buffer1->space, 1);

    semaphore_take(Mo_buffer1->guard, 1),
    Mo_buffer1->buf[Mo_buffer1->in] =  object;
    Mo_buffer1->in = (Mo_buffer1->in + 1) % BufferSIZE;
    semaphore_release(Mo_buffer1->guard);

    semaphore_release(Mo_buffer1->item);
}

int Mo_remove(Mo_buffer_handle Mo_buffer1)
{
    semaphore_take(Mo_buffer1->item, 1);
    int item1 = Mo_buffer1->buf[Mo_buffer1->out];
    Mo_buffer1->out = (Mo_buffer1->out + 1) % BufferSIZE;
    semaphore_release(Mo_buffer1->space);
    return item1;
}



/* many producer, many consumer.
 *
 */

Class(Mm_buffer)
{
    int buf[BufferSIZE];
    int in;
    int out;
    Semaphore_Handle item;
    Semaphore_Handle space;
    Semaphore_Handle guard;
};



Mm_buffer_handle Mm_buffer_creat(void)
{
    Mm_buffer_handle Mm_buffer1 = heap_malloc(sizeof (Mm_buffer));
    *Mm_buffer1 = (Mm_buffer){
            .buf = {0,0,0,0,0},
            .in  = 0,
            .out = 0,
            .item = semaphore_creat(0),
            .space = semaphore_creat(BufferSIZE),
            .guard = semaphore_creat(1),
    };
    return Mm_buffer1;
}

void Mm_insert(Mm_buffer_handle Mm_buffer1, int object)
{
    semaphore_take(Mm_buffer1->space, 1);

    semaphore_take(Mm_buffer1->guard, 1),
            Mm_buffer1->buf[Mm_buffer1->in] =  object;
    Mm_buffer1->in = (Mm_buffer1->in + 1) % BufferSIZE;
    semaphore_release(Mm_buffer1->guard);

    semaphore_release(Mm_buffer1->item);
}

int Mm_remove(Mm_buffer_handle Mm_buffer1)
{
    semaphore_take(Mm_buffer1->item, 1);

    semaphore_take(Mm_buffer1->guard, 1);
    int item1 = Mm_buffer1->buf[Mm_buffer1->out];
    Mm_buffer1->out = (Mm_buffer1->out + 1) % BufferSIZE;
    semaphore_release(Mm_buffer1->guard);

    semaphore_release(Mm_buffer1->space);
    return item1;
}

/* many reader, many writer.
 *
 */

Class(MrMw_control)
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

MrMw_control_handle MrOw_creat(void)
{
    MrMw_control_handle MrMw_control1 = heap_malloc(sizeof (MrMw_control));
    *MrMw_control1 = (MrMw_control){
        .read = semaphore_creat(0),
        .write = semaphore_creat(0),
        .W_guard = semaphore_creat(1),
        .C_guard = semaphore_creat(1),
        .active_reader = 0,
        .reading_reader = 0,
        .active_writer = 0,
        .writing_writer = 0
    };
    return MrMw_control1;
}

void read_acquire(MrMw_control_handle MrMw_control1)
{
    semaphore_take(MrMw_control1->C_guard, 1);
    MrMw_control1->active_reader += 1;
    if(MrMw_control1->active_writer == 0){
        MrMw_control1->reading_reader += 1;
        semaphore_release(MrMw_control1->read);
    }
    semaphore_release(MrMw_control1->C_guard);
    semaphore_take(MrMw_control1->read, 1);
}

void read_release(MrMw_control_handle MrMw_control1)
{
    semaphore_take(MrMw_control1->C_guard, 1);
    MrMw_control1->reading_reader -= 1;
    MrMw_control1->active_reader -= 1;
    if(MrMw_control1->reading_reader == 0){
        while(MrMw_control1->writing_writer < MrMw_control1->active_writer){
            MrMw_control1->writing_writer += 1;
            semaphore_release(MrMw_control1->write);
        }
    }
    semaphore_release(MrMw_control1->C_guard);
}

void write_acquire(MrMw_control_handle MrMw_control1)
{
    semaphore_take(MrMw_control1->C_guard, 1);
    MrMw_control1->active_writer -= 1;
    if(MrMw_control1->reading_reader == 0){
        MrMw_control1->writing_writer += 1;
        semaphore_release(MrMw_control1->write);
    }
    semaphore_release(MrMw_control1->C_guard);
    semaphore_take(MrMw_control1->write, 1);

    semaphore_take(MrMw_control1->W_guard, 1);
}


void write_release(MrMw_control_handle MrMw_control1)
{
    semaphore_release(MrMw_control1->W_guard);

    semaphore_take(MrMw_control1->C_guard, 1);
    MrMw_control1->writing_writer -= 1;
    MrMw_control1->active_writer -= 1;
    if(MrMw_control1->active_writer == 0){
        while(MrMw_control1->reading_reader < MrMw_control1->active_reader){
            MrMw_control1->reading_reader += 1;
            semaphore_release(MrMw_control1->read);
        }
    }
    semaphore_release(MrMw_control1->C_guard);
}


































