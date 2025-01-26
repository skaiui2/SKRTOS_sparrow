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

#include "sem.h"
#include "heap.h"
#include "port.h"

Class(Semaphore_struct)
{
    uint8_t value;
    TheList WaitList;
};


Semaphore_Handle semaphore_creat(uint8_t value)
{
    Semaphore_struct *xSemaphore = heap_malloc(sizeof (Semaphore_struct) );
    xSemaphore->value = value;
    ListInit(&(xSemaphore->WaitList));
    return xSemaphore;
}

void semaphore_delete(Semaphore_Handle semaphore)
{
    heap_free(semaphore);
}


extern uint8_t schedule_PendSV;
uint8_t semaphore_release( Semaphore_Handle semaphore)
{
    uint32_t xre = xEnterCritical();
    TaskHandle_t CurrentTCB = GetCurrentTCB();
    uint8_t CurrentTcbPriority = GetTaskPriority(CurrentTCB);

    if (semaphore->WaitList.count != 0) {
        TaskHandle_t SendTask = IPCHighestPriorityTask(&(semaphore->WaitList));
        DelayListRemove(SendTask);
        Remove_IPC(SendTask);
        TaskListAdd(SendTask,Ready);
        if(GetTaskPriority(SendTask) > CurrentTcbPriority ){
            schedule();
        }
    }
    (semaphore->value)++;

    xExitCritical(xre);
    return true;
}


uint8_t semaphore_take(Semaphore_Handle semaphore,uint32_t Ticks)
{
    uint32_t xre = xEnterCritical();
    TaskHandle_t CurrentTCB = GetCurrentTCB();

    if( semaphore->value > 0) {
        (semaphore->value)--;
        xExitCritical(xre);
        return true;
    }
    if(Ticks == 0 ){
        return false;
    }

    uint8_t volatile temp = schedule_PendSV;

    if(Ticks > 0){
        Insert_IPC(CurrentTCB, &(semaphore->WaitList));
        TaskDelay(Ticks);
    }
    xExitCritical(xre);

    while(temp == schedule_PendSV){ }//It loops until the schedule is start.

    uint32_t xReturn = xEnterCritical();
    //Check whether the wake is due to delay or due to semaphore availability
    if(!CheckIPCState(CurrentTCB)){//if true ,the task is Block!
        Remove_IPC(CurrentTCB);
        xExitCritical(xReturn);
        return false;
    }else{
        (semaphore->value)--;
        xExitCritical(xReturn);
        return true;
    }
}

