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

#include "timer.h"
#include "heap.h"
#include "atomic.h"



Class(timer_struct)
{
    ListNode            TimerNode;
    uint32_t            TimerPeriod;
    TimerFunction_t     CallBackFun;
    uint8_t             TimerStopFlag ;
};

TheList ClockList;
volatile uint64_t AbsoluteClock = 0;

void ClockListAdd(timer_struct *timer)
{
    const uint32_t constTicks = AbsoluteClock;
    uint32_t wakeTime = constTicks + timer->TimerNode.value;
    timer->TimerNode.value = wakeTime;

    ListAdd(&ClockList, &timer->TimerNode);
}


void timer_check(void)
{
    while (1) {
        ListNode *node = ClockList.head;
        while ( node->value <= AbsoluteClock ) {
            timer_struct *timer = container_of( node,timer_struct,TimerNode);
            timer->CallBackFun(timer);
            if(timer->TimerStopFlag == stop) {
                ListRemove(&ClockList, &timer->TimerNode);
            }
            if( node != ClockList.tail ) {
                node = node->next;
            }
        }
    }
}


TaskHandle_t xTimerInit(uint8_t timer_priority, uint16_t stack)
{
    TaskHandle_t self = NULL;
    ListInit(&ClockList);
    xTaskCreate((TaskFunction_t)timer_check,
                stack,
                NULL,
                timer_priority,
                &self,
                0);
    return self;
}


timer_struct *xTimerCreat(TimerFunction_t CallBackFun, uint32_t period, uint8_t timer_flag)
{
    timer_struct *timer = heap_malloc(sizeof(timer_struct));
    *timer = (timer_struct){
            .TimerPeriod = period,
            .CallBackFun = CallBackFun,
            .TimerStopFlag = timer_flag
    };
    timer->TimerNode.value = period;
    ListInit(&ClockList);
    ClockListAdd(timer);
    return timer;
}


uint8_t TimerRerun(timer_struct *timer)
{
    atomic_set(run, (uint32_t *)&(timer->TimerStopFlag));
    ClockListAdd(timer);
    return timer->TimerStopFlag;
}



uint8_t TimerStop(timer_struct *timer)
{
    atomic_set(stop, (uint32_t *)&(timer->TimerStopFlag));
    ListRemove(&ClockList,&timer->TimerNode);
    return timer->TimerStopFlag;
}





