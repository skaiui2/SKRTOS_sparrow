
#include <string.h>
#include "mequeue.h"
#include "heap.h"


Class(Queue_struct)
{
    uint8_t *startPoint;
    uint8_t *endPoint;
    uint8_t *readPoint;
    uint8_t *writePoint;
    uint8_t MessageNumber;
    uint32_t SendTable;
    uint32_t ReceiveTable;
    uint32_t NodeSize;
    uint32_t NodeNumber;
};



Queue_struct* queue_creat(uint32_t queue_length,uint32_t queue_size)
{
    size_t  Qsize = (size_t)( queue_length * queue_size);
    Queue_struct *queue = heap_malloc(sizeof (Queue_struct) + Qsize);
    uint8_t *message_start = (uint8_t *)queue + sizeof(Queue_struct);

    *queue = (Queue_struct){
            .startPoint = message_start,
            .endPoint   = (uint8_t *)(message_start + Qsize),
            .readPoint  = (uint8_t *)( message_start + ( queue_length - 1) * queue_size ),
            .writePoint = message_start,
            .MessageNumber = 0UL,
            .SendTable    = 0UL,
            .ReceiveTable = 0UL,
            .NodeNumber  = queue_length,
            .NodeSize   = queue_size,
    };
    return queue;
}



void queue_delete( Queue_struct *queue )
{
    heap_free(queue);
}


#define  GetTopTCBIndex    FindHighestPriority
extern uint8_t schedule_PendSV;

void WriteToQueue( Queue_struct *queue , uint32_t *buf, uint8_t CurrentTcbPriority)
{
    memcpy((void *) queue->writePoint, buf, (size_t) queue->NodeSize);
    queue->writePoint += queue->NodeSize;

    if (queue->writePoint >= queue->endPoint) {
        queue->writePoint = queue->startPoint;
    }

    if (queue->ReceiveTable!= 0) {
        //Wake up the highest priority task in the receiving list
        uint8_t uxPriority =  GetTopTCBIndex(queue->ReceiveTable);
        TaskHandle_t taskHandle = GetTaskHandle(uxPriority);
        queue->ReceiveTable &= ~(1 << uxPriority );//it belongs to the IPC layer,can't use State port!
        StateRemove(taskHandle,Block);// Also synchronize with the total blocking state
        StateRemove(taskHandle,Delay);
        StateAdd(taskHandle, Ready);
        if(uxPriority > CurrentTcbPriority){
            schedule();
        }
    }

    (queue->MessageNumber)++;
}

void ExtractFromQueue( Queue_struct *queue, uint32_t *buf, uint8_t CurrentTcbPriority)
{
    queue->readPoint += queue->NodeSize;

    if( queue->readPoint >= queue->endPoint ){
        queue->readPoint = queue->startPoint;
    }
    memcpy( ( void * ) buf, ( void * ) queue->readPoint, ( size_t ) queue->NodeSize );

    if (queue->SendTable != 0) {
        //Wake up the highest priority task in the sending list
        uint8_t uxPriority =  GetTopTCBIndex(queue->SendTable);
        TaskHandle_t taskHandle = GetTaskHandle(uxPriority);
        queue->SendTable &= ~(1 << uxPriority );//it belongs to the IPC layer,can't use State port!
        StateRemove(taskHandle,Block);// Also synchronize with the total blocking state
        StateRemove(taskHandle,Delay);
        StateAdd(taskHandle, Ready);
        if(uxPriority > CurrentTcbPriority ){
            schedule();
        }
    }

    (queue->MessageNumber)--;
}



uint8_t queue_send(Queue_struct *queue, uint32_t *buf, uint32_t Ticks)
{
    uint32_t xre = xEnterCritical();
    TaskHandle_t CurrentTCB = GetCurrentTCB();
    uint8_t CurrentTcbPriority = GetTaskPriority(CurrentTCB);

    if (queue->MessageNumber < queue->NodeNumber) {
        //normally write
        WriteToQueue(queue, buf, CurrentTcbPriority);

        xExitCritical(xre);
        return true;
    } //Block!

    if (queue->MessageNumber == queue->NodeNumber) {
        if (Ticks == 0) {
            xExitCritical(xre);
            return false;
        }
    }else{
        return false;
    }

    uint8_t volatile temp = schedule_PendSV;

    if(Ticks > 0){
        StateAdd(CurrentTCB,Block);
        queue->SendTable |= (1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        TaskDelay(Ticks);
    }
    xExitCritical(xre);

    while(temp == schedule_PendSV){ }//It loops until the schedule is start.

    uint32_t xReturn  = xEnterCritical();
    //Check whether the wake is due to delay or due to semaphore availability
    if( CheckState(CurrentTCB,Block) ){//if true ,the task is Block!
        queue->SendTable &= ~(1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        StateRemove(CurrentTCB,Block);
        xExitCritical(xReturn);
        return false;
    }else{
        WriteToQueue(queue, buf, CurrentTcbPriority);
        xExitCritical(xReturn);
        return true;
    }
}



uint8_t queue_receive( Queue_struct *queue, uint32_t *buf, uint32_t Ticks )
{
    uint32_t xre = xEnterCritical();
    TaskHandle_t CurrentTCB = GetCurrentTCB();
    uint8_t CurrentTcbPriority = GetTaskPriority(CurrentTCB);
    if( queue->MessageNumber > 0){
        ExtractFromQueue(queue, buf, CurrentTcbPriority);

        xExitCritical(xre);
        return true;
    }
    if (queue->MessageNumber == 0) {
        if (Ticks == 0) {
            xExitCritical(xre);
            return false;
        }
    }

    uint8_t volatile temp = schedule_PendSV;
    if(Ticks > 0){
        TaskHandle_t taskHandle = GetTaskHandle(CurrentTcbPriority);
        StateAdd(taskHandle,Block);
        queue->ReceiveTable |= (1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        TaskDelay(Ticks);
    }
    xExitCritical(xre);

    while(temp == schedule_PendSV){ }//It loops until the schedule is start.

    uint32_t xReturn  = xEnterCritical();
    //Check whether the wake is due to delay or due to semaphore availability
    uint8_t uxPriority = GetTaskPriority(CurrentTCB);
    TaskHandle_t taskHandle = GetTaskHandle(uxPriority);
    if( CheckState(taskHandle,Block) ){//if true ,the task is Block!
        queue->ReceiveTable &= ~(1 << uxPriority);//it belongs to the IPC layer,can't use State port!
        StateRemove(taskHandle,Block);
        xExitCritical(xReturn);
        return false;
    }else{
        ExtractFromQueue(queue, buf, CurrentTcbPriority);
        xExitCritical(xReturn);
        return true;
    }
}



