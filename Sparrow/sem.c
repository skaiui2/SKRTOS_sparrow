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

extern uint8_t PendSV;
uint8_t semaphore_release( Semaphore_Handle semaphore)
{
    uint32_t xre = xEnterCritical();

    if (semaphore->xBlock) {
        uint8_t uxPriority =  GetTopTCBIndex(semaphore->xBlock);
        TaskHandle_t taskHandle = GetTaskHandle(uxPriority);
        semaphore->xBlock &= ~(1 << uxPriority );//it belongs to the IPC layer,can't use State port!
        StateRemove(taskHandle,Block);// Also synchronize with the total blocking state
        StateRemove(taskHandle,Delay);
        StateAdd(taskHandle, Ready);
    }
    semaphore->value++;
    schedule();

    xExitCritical(xre);
    return true;
}

extern TaskHandle_t  schedule_currentTCB;
uint8_t semaphore_take(Semaphore_Handle semaphore,uint32_t Ticks)
{
    uint32_t xre = xEnterCritical();

    if( semaphore->value > 0) {
        semaphore->value--;
        schedule();
        xExitCritical(xre);
        return true;
    }

    if(Ticks == 0 ){
        return false;
    }

    uint8_t volatile temp = PendSV;
    if(Ticks > 0){
        uint8_t uxPriority = GetTaskPriority(schedule_currentTCB);
        TaskHandle_t taskHandle = GetTaskHandle(uxPriority);
        StateAdd(taskHandle,Block);
        semaphore->xBlock |= (1 << uxPriority);//it belongs to the IPC layer,can't use State port!
        TaskDelay(Ticks);
    }
    xExitCritical(xre);

    while(temp == PendSV){ }//It loops until the schedule is start.

    uint32_t xReturn = xEnterCritical();
    //Check whether the wake is due to delay or due to semaphore availability
    uint8_t uxPriority = GetTaskPriority(schedule_currentTCB);
    TaskHandle_t taskHandle = GetTaskHandle(uxPriority);
    if( CheckState(taskHandle,Block) ){//if true ,the task is Block!
        semaphore->xBlock &= ~(1 << uxPriority);//it belongs to the IPC layer,can't use State port!
        StateRemove(taskHandle,Block);
        xExitCritical(xReturn);
        return false;
    }else{
        semaphore->value--;
        schedule();
        xExitCritical(xReturn);
        return true;
    }
}

