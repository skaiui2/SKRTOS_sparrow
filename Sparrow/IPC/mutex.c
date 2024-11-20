#include "mutex.h"
#include "heap.h"

Class(Mutex_struct)
{
    uint8_t        value;
    uint32_t       original_priority;
    uint32_t       WaitTable;
    TaskHandle_t   owner;
};



Mutex_Handle mutex_creat(void)
{
    Mutex_struct *mutex = heap_malloc(sizeof (Mutex_struct) );
    *mutex = (Mutex_struct){
            .value = 1,
            .original_priority = 0UL,
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
        StateAdd(CurrentTCB,Block);
        mutex->WaitTable |= (1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        TaskDelay(Ticks);
        uint8_t MutexOwnerPriority = GetTaskPriority(mutex->owner);
        if( MutexOwnerPriority < CurrentTcbPriority) {
            StateRemove(mutex->owner, Ready);
            TaskPrioritySet(mutex->owner, CurrentTcbPriority + 1);
            StateAdd(mutex->owner, Ready);
        }
    }
    xExitCritical(xre);

    while(temp == schedule_PendSV){ }//It loops until the schedule is start.

    uint32_t xReturn = xEnterCritical();
    //Check whether the wake is due to delay or due to mutex availability
    if( CheckState(CurrentTCB,Block) ){//if true ,the task is Block!
        mutex->WaitTable &= ~(1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        StateRemove(CurrentTCB,Block);
        xExitCritical(xReturn);
        return false;
    }else{
        (mutex->value)--;
        xExitCritical(xReturn);
        return true;
    }
}


uint8_t mutex_unlock( Mutex_Handle mutex)
{
    uint32_t xre = xEnterCritical();
    uint8_t owner_priority = GetTaskPriority(mutex->owner);

    if (mutex->WaitTable) {
        uint8_t uxPriority =  GetTopTCBIndex(mutex->WaitTable);
        TaskHandle_t taskHandle = GetTaskHandle(uxPriority);
        mutex->WaitTable &= ~(1 << uxPriority );//it belongs to the IPC layer,can't use State port!
        StateRemove(taskHandle,Block);// Also synchronize with the total blocking state
        StateRemove(taskHandle,Delay);
        StateAdd(taskHandle, Ready);
        if(uxPriority > owner_priority){
            schedule();
        }
    }

    if(owner_priority != mutex->original_priority) {
        StateRemove(mutex->owner, Ready);
        TaskPrioritySet(mutex->owner, mutex->original_priority);
        StateAdd(mutex->owner, Ready);
    }

    (mutex->value)++;

    xExitCritical(xre);
    return true;
}


