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
    uint32_t xBlock;
};


Semaphore_Handle semaphore_creat(uint8_t value)
{
    Semaphore_struct *xSemaphore = heap_malloc(sizeof (Semaphore_struct) );
    xSemaphore->value = value;
    xSemaphore->xBlock = 0;
    return xSemaphore;
}

void semaphore_delete(Semaphore_Handle semaphore)
{
    heap_free(semaphore);
}

/**In accordance with the principle of interfaces,
 * the IPC layer needs to write its own functions to obtain the highest priority,
 * here for convenience, choose to directly use the scheduling layer functions.
 * */
#define  GetTopTCBIndex    FindHighestPriority

extern uint8_t schedule_count;
uint8_t semaphore_release( Semaphore_Handle semaphore)
{
    uint32_t xre = EnterCritical();
    TaskHandle_t CurrentTCB = GetCurrentTCB();
    uint8_t CurrentTcbPriority = GetTaskPriority(CurrentTCB);

    if (semaphore->xBlock) {
        uint8_t uxPriority =  GetTopTCBIndex(semaphore->xBlock);
        TaskHandle_t taskHandle = GetTaskHandle(uxPriority);
        semaphore->xBlock &= ~(1 << uxPriority );//it belongs to the IPC layer,can't use State port!
        TableRemove(taskHandle,Block);// Also synchronize with the total blocking state
        TableRemove(taskHandle,Delay);
        TableAdd(taskHandle, Ready);
        if(uxPriority > CurrentTcbPriority){
            schedule();
        }
    }
    (semaphore->value)++;

    ExitCritical(xre);
    return true;
}


uint8_t semaphore_take(Semaphore_Handle semaphore,uint32_t Ticks)
{
    uint32_t xre = EnterCritical();
    TaskHandle_t CurrentTCB = GetCurrentTCB();
    uint8_t CurrentTcbPriority = GetTaskPriority(CurrentTCB);

    if( semaphore->value > 0) {
        (semaphore->value)--;
        ExitCritical(xre);
        return true;
    }

    if(Ticks == 0 ){
        return false;
    }

    uint8_t volatile temp = schedule_count;

    if(Ticks > 0){
        TableAdd(CurrentTCB,Block);
        semaphore->xBlock |= (1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        TaskDelay(Ticks);
    }
    ExitCritical(xre);

    while(temp == schedule_count){ }//It loops until the schedule is start.

    uint32_t xReturn = EnterCritical();
    //Check whether the wake is due to delay or due to semaphore availability
    if( CheckState(CurrentTCB,Block) ){//if true ,the task is Block!
        semaphore->xBlock &= ~(1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        TableRemove(CurrentTCB,Block);
        ExitCritical(xReturn);
        return false;
    }else{
        (semaphore->value)--;
        ExitCritical(xReturn);
        return true;
    }
}

