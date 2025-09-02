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
#include "port.h"
#include "compare.h"



Class(timer_struct)
{
    uint16_t            TimerPeriod;
    TimerFunction_t     CallBackFun;
    uint8_t             TimerStopFlag;
    uint8_t             Index;
};

uint32_t TimerTable = 0;
TimerHandle TimerStructArray[configTimerNumber];
uint32_t    TimerRecordArray[configTimerNumber];
static uint16_t TimerCheckPeriod = 0;
extern uint32_t TicksBase;

void TimerTableAdd(timer_struct *timer)
{
    uint32_t cpu_lock = EnterCritical();
    TimerStructArray[timer->Index] = timer;
    TimerRecordArray[timer->Index] = TicksBase + timer->TimerPeriod;
    TimerTable |= (1 << timer->Index);
    ExitCritical(cpu_lock);
}

void TimerTableRemove(timer_struct *timer)
{
    uint32_t cpu_lock = EnterCritical();
    TimerStructArray[timer->Index] = NULL;
    TimerRecordArray[timer->Index] = 0;
    TimerTable &= ~(1 << timer->Index);
    ExitCritical(cpu_lock);
}


void timer_check(void) {
    while (1) {
        uint32_t lookup = TimerTable;
        while (lookup) {
            uint8_t i = FindHighestPriority(lookup);
            if (compare_before_eq(TimerRecordArray[i], TicksBase)) {
                timer_struct *timer = TimerStructArray[i];
                timer->CallBackFun(timer);
                if (timer->TimerStopFlag == run) {
                    TimerRecordArray[timer->Index] += timer->TimerPeriod;
                } else {
                    TimerTableRemove(timer);
                }
            }
            lookup &= ~(1 << i);
        }

        TaskDelay(TimerCheckPeriod);
    }
}


TaskHandle_t TimerInit(uint8_t timer_priority, uint16_t stack, uint8_t check_period)
{
    TaskHandle_t self = NULL;
    for (uint8_t i = 0; i < configTimerNumber; i++) {
        TimerStructArray[i] = NULL;
        TimerRecordArray[i] = 0;
    }
    TaskCreate((TaskFunction_t)timer_check,
               stack,
               NULL,
               timer_priority,
               &self
               );
    TimerCheckPeriod = check_period;

    return self;
}


timer_struct *TimerCreat(TimerFunction_t CallBackFun, uint32_t period, uint8_t index, uint8_t timer_flag)
{
    timer_struct *timer = heap_malloc(sizeof(timer_struct));
    *timer = (timer_struct){
            .TimerPeriod = period,
            .CallBackFun = CallBackFun,
            .TimerStopFlag = timer_flag,
            .Index = index
    };
    TimerRecordArray[timer->Index] = TicksBase + period;
    TimerTableAdd(timer);
    return timer;
}

void TimerDelete(TimerHandle timer)
{
    heap_free(timer);
}


uint8_t TimerRerun(timer_struct *timer, uint8_t timer_flag)
{
    TimerTableAdd(timer);
    return atomic_set_return(timer_flag, (uint32_t *)&(timer->TimerStopFlag));
}


/*
 * timer callback function will Execute once, then removed.
 */
uint8_t TimerStop(timer_struct *timer)
{
    return atomic_set_return(stop, (uint32_t *)&(timer->TimerStopFlag));
}

/*
 * timer callback function removed Immediately.
 */
uint8_t TimerStopImmediate(timer_struct *timer)
{
    TimerTableRemove(timer);
    return atomic_set_return(stop, (uint32_t *)&(timer->TimerStopFlag));
}



