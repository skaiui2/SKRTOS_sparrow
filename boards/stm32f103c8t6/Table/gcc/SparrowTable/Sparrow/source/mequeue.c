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

#include <string.h>
#include "mequeue.h"
#include "heap.h"
#include "port.h"


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
extern uint8_t schedule_count;

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
        TableRemove(taskHandle,Block);// Also synchronize with the total blocking state
        TableRemove(taskHandle,Delay);
        TableAdd(taskHandle, Ready);
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
        TableRemove(taskHandle,Block);// Also synchronize with the total blocking state
        TableRemove(taskHandle,Delay);
        TableAdd(taskHandle, Ready);
        if(uxPriority > CurrentTcbPriority ){
            schedule();
        }
    }

    (queue->MessageNumber)--;
}



uint8_t queue_send(Queue_struct *queue, uint32_t *buf, uint32_t Ticks)
{
    uint32_t xre = EnterCritical();
    TaskHandle_t CurrentTCB = GetCurrentTCB();
    uint8_t CurrentTcbPriority = GetTaskPriority(CurrentTCB);

    if (queue->MessageNumber < queue->NodeNumber) {
        //normally write
        WriteToQueue(queue, buf, CurrentTcbPriority);

        ExitCritical(xre);
        return true;
    } //Block!

    if (queue->MessageNumber == queue->NodeNumber) {
        if (Ticks == 0) {
            ExitCritical(xre);
            return false;
        }
    }else{
        return false;
    }

    uint8_t volatile temp = schedule_count;

    if(Ticks > 0){
        TableAdd(CurrentTCB,Block);
        queue->SendTable |= (1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        TaskDelay(Ticks);
    }
    ExitCritical(xre);

    while(temp == schedule_count){ }//It loops until the schedule is start.

    uint32_t xReturn  = EnterCritical();
    //Check whether the wake is due to delay or due to semaphore availability
    if( CheckState(CurrentTCB,Block) ){//if true ,the task is Block!
        queue->SendTable &= ~(1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        TableRemove(CurrentTCB,Block);
        ExitCritical(xReturn);
        return false;
    }else{
        WriteToQueue(queue, buf, CurrentTcbPriority);
        ExitCritical(xReturn);
        return true;
    }
}



uint8_t queue_receive( Queue_struct *queue, uint32_t *buf, uint32_t Ticks )
{
    uint32_t xre = EnterCritical();
    TaskHandle_t CurrentTCB = GetCurrentTCB();
    uint8_t CurrentTcbPriority = GetTaskPriority(CurrentTCB);
    if( queue->MessageNumber > 0){
        ExtractFromQueue(queue, buf, CurrentTcbPriority);

        ExitCritical(xre);
        return true;
    }
    if (queue->MessageNumber == 0) {
        if (Ticks == 0) {
            ExitCritical(xre);
            return false;
        }
    }

    uint8_t volatile temp = schedule_count;
    if(Ticks > 0){
        TaskHandle_t taskHandle = GetTaskHandle(CurrentTcbPriority);
        TableAdd(taskHandle,Block);
        queue->ReceiveTable |= (1 << CurrentTcbPriority);//it belongs to the IPC layer,can't use State port!
        TaskDelay(Ticks);
    }
    ExitCritical(xre);

    while(temp == schedule_count){ }//It loops until the schedule is start.

    uint32_t xReturn  = EnterCritical();
    //Check whether the wake is due to delay or due to semaphore availability
    uint8_t uxPriority = GetTaskPriority(CurrentTCB);
    TaskHandle_t taskHandle = GetTaskHandle(uxPriority);
    if( CheckState(taskHandle,Block) ){//if true ,the task is Block!
        queue->ReceiveTable &= ~(1 << uxPriority);//it belongs to the IPC layer,can't use State port!
        TableRemove(taskHandle,Block);
        ExitCritical(xReturn);
        return false;
    }else{
        ExtractFromQueue(queue, buf, CurrentTcbPriority);
        ExitCritical(xReturn);
        return true;
    }
}



