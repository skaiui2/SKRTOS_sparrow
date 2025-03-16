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
    uint32_t       WaitTable;
    TaskHandle_t   owner;
};



Mutex_Handle mutex_creat(void)
{
    Mutex_struct *mutex = heap_malloc(sizeof (Mutex_struct) );
    *mutex = (Mutex_struct){
            .value = 1,
            .WaitTable = 0UL,
            .owner = NULL
    };
    return mutex;
}



void mutex_delete(Mutex_Handle mutex)
{
    heap_free(mutex);
}


/**In accordance with the principle of interfaces,
 * the IPC layer needs to write its own functions to obtain the highest priority,
 * here for convenience, choose to directly use the scheduling layer functions.
 * */
#define  GetTopTCBIndex    FindHighestPriority

extern uint8_t schedule_count;


uint8_t mutex_lock(Mutex_Handle mutex,uint32_t Ticks)
{
    uint32_t xre = EnterCritical();
    TaskHandle_t CurrentTCB = GetCurrentTCB();
    uint8_t CurrentTcbPriority = GetTaskPriority(CurrentTCB);
    if( mutex->value > 0) {
        mutex->owner = CurrentTCB;
        (mutex->value)--;
        ExitCritical(xre);
        return true;
    }

    if(Ticks == 0 ){
        return false;
    }

    uint8_t volatile temp = schedule_count;

    if(Ticks > 0){
        TableAdd(CurrentTCB,Block);
        mutex->WaitTable |= (1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        TaskDelay(Ticks);
        uint8_t MutexOwnerPriority = GetTaskPriority(mutex->owner);
        if( MutexOwnerPriority < CurrentTcbPriority) {
            TableRemove(mutex->owner, Ready);
            PreemptiveCPU(MutexOwnerPriority);
        }
    }
    ExitCritical(xre);

    while(temp == schedule_count){ }//It loops until the schedule is start.

    uint32_t xReturn = EnterCritical();
    //Check whether the wake is due to delay or due to mutex availability
    if( CheckState(CurrentTCB,Block) ){//if true ,the task is Block!
        mutex->WaitTable &= ~(1 << CurrentTcbPriority);
        TableRemove(CurrentTCB,Block);
        ExitCritical(xReturn);
        return false;
    }else{
        (mutex->value)--;
        ExitCritical(xReturn);
        return true;
    }
}


uint8_t mutex_unlock( Mutex_Handle mutex)
{
    uint32_t xre = EnterCritical();

    if (mutex->WaitTable) {
        uint8_t uxPriority =  GetTopTCBIndex(mutex->WaitTable);
        TaskHandle_t taskHandle = GetTaskHandle(uxPriority);
        mutex->WaitTable &= ~(1 << uxPriority );
        TableRemove(taskHandle,Block);
        TableRemove(taskHandle,Delay);
        TableAdd(taskHandle, Ready);
    }
    (mutex->value)++;

    ExitCritical(xre);
    return true;
}


