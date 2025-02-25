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

#include "mutex.h"
#include "heap.h"
#include "port.h"

Class(Mutex_struct)
{
    uint8_t        value;
    uint32_t       original_priority;
    rb_root       WaitTree;
    TaskHandle_t   owner;
};



Mutex_Handle mutex_creat(void)
{
    Mutex_struct *mutex = heap_malloc(sizeof (Mutex_struct) );
    *mutex = (Mutex_struct){
            .value = 1,
            .original_priority = 0UL,
            .owner = NULL
    };
    rb_root_init(&(mutex->WaitTree));
    return mutex;
}



void mutex_delete(Mutex_Handle mutex)
{
    heap_free(mutex);
}



extern uint8_t schedule_PendSV;


uint8_t mutex_lock(Mutex_Handle mutex,uint32_t Ticks)
{
    uint32_t xre = xEnterCritical();
    TaskHandle_t CurrentTCB = GetCurrentTCB();
    uint8_t CurrentTcbPriority = GetTaskPriority(CurrentTCB);
    if( mutex->value > 0) {
        mutex->original_priority = CurrentTcbPriority;
        mutex->owner = CurrentTCB;
        (mutex->value)--;
        xExitCritical(xre);
        return true;
    }

    if(Ticks == 0 ){
        return false;
    }

    uint8_t volatile temp = schedule_PendSV;

    if(Ticks > 0){
        Insert_IPC(CurrentTCB, &(mutex->WaitTree));
        TaskDelay(Ticks);
        uint8_t MutexOwnerPriority = GetTaskPriority(mutex->owner);
        if( MutexOwnerPriority < CurrentTcbPriority) {
            TaskPrioritySet(mutex->owner, CurrentTcbPriority);
        }
    }
    xExitCritical(xre);

    while(temp == schedule_PendSV){ }//It loops until the schedule is start.

    uint32_t xReturn = xEnterCritical();
    //Check whether the wake is due to delay or due to mutex availability
    if(!CheckIPCState(CurrentTCB)){//if true ,the task is Block!
        Remove_IPC(CurrentTCB);
        xExitCritical(xReturn);
        return false;
    }else{
        mutex->original_priority = CurrentTcbPriority;
        mutex->owner = CurrentTCB;
        (mutex->value)--;
        xExitCritical(xReturn);
        return true;
    }
}


uint8_t mutex_unlock( Mutex_Handle mutex)
{
    uint32_t xre = xEnterCritical();
    TaskHandle_t CurrentTCB = GetCurrentTCB();
    uint8_t owner_priority = GetTaskPriority(mutex->owner);
    uint8_t CurrentTcbPriority = GetTaskPriority(CurrentTCB);

    if (mutex->WaitTree.count != 0) {
        TaskHandle_t WaitTask = IPCHighestPriorityTask(&(mutex->WaitTree));
        DelayTreeRemove(WaitTask);
        Remove_IPC(WaitTask);
        TaskTreeAdd(WaitTask,Ready);
        if(GetTaskPriority(WaitTask) > CurrentTcbPriority ){
            schedule();
        }
    }

    if(owner_priority != mutex->original_priority) {
        TaskPrioritySet(mutex->owner, mutex->original_priority);
    }

    (mutex->value)++;

    xExitCritical(xre);
    return true;
}


