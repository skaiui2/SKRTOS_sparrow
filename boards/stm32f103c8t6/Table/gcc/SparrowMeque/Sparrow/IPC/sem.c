//
// Created by el on 2024/11/9.
//

#include "sem.h"
#include "heap.h"

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

extern uint8_t schedule_PendSV;
uint8_t semaphore_release( Semaphore_Handle semaphore)
{
    uint32_t xre = xEnterCritical();
    TaskHandle_t CurrentTCB = GetCurrentTCB();
    uint8_t CurrentTcbPriority = GetTaskPriority(CurrentTCB);

    if (semaphore->xBlock) {
        uint8_t uxPriority =  GetTopTCBIndex(semaphore->xBlock);
        TaskHandle_t taskHandle = GetTaskHandle(uxPriority);
        semaphore->xBlock &= ~(1 << uxPriority );//it belongs to the IPC layer,can't use State port!
        StateRemove(taskHandle,Block);// Also synchronize with the total blocking state
        StateRemove(taskHandle,Delay);
        StateAdd(taskHandle, Ready);
        if(uxPriority > CurrentTcbPriority){
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
    uint8_t CurrentTcbPriority = GetTaskPriority(CurrentTCB);

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
        StateAdd(CurrentTCB,Block);
        semaphore->xBlock |= (1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        TaskDelay(Ticks);
    }
    xExitCritical(xre);

    while(temp == schedule_PendSV){ }//It loops until the schedule is start.

    uint32_t xReturn = xEnterCritical();
    //Check whether the wake is due to delay or due to semaphore availability
    if( CheckState(CurrentTCB,Block) ){//if true ,the task is Block!
        semaphore->xBlock &= ~(1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        StateRemove(CurrentTCB,Block);
        xExitCritical(xReturn);
        return false;
    }else{
        (semaphore->value)--;
        xExitCritical(xReturn);
        return true;
    }
}

