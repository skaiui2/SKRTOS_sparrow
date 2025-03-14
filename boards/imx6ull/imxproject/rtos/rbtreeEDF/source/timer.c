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

Class(timer_struct)
{
    rb_node             TimerNode;
    uint32_t            TimerPeriod;
    TimerFunction_t     CallBackFun;
    uint8_t             TimerStopFlag;
};

rb_root ClockTree;
static uint16_t TimerCheckPeriod = 0;

extern uint64_t AbsoluteClock;

void ClockTreeAdd(timer_struct *timer)
{
    uint32_t cpu_lock = xEnterCritical();
    timer->TimerNode.value += AbsoluteClock;
    rb_Insert_node(&ClockTree, &timer->TimerNode);
    xExitCritical(cpu_lock);
}


void ClockListRemove(timer_struct *timer)
{
    uint32_t cpu_lock = xEnterCritical();
    rb_remove_node(&ClockTree, &timer->TimerNode);
    xExitCritical(cpu_lock);
}


void timer_check(void)
{
    while (1) {
        TaskEnter();

        rb_node *next_node = NULL;
        rb_node *node = ClockTree.first_node;
        while ( (node) && (node->value <= AbsoluteClock) ) {
            next_node = rb_next(node);
            timer_struct *timer = container_of(node, timer_struct, TimerNode);
            timer->CallBackFun(timer);
            if (timer->TimerStopFlag == run) {
                node->value += timer->TimerPeriod;
            } else {
                ClockListRemove(timer);//remove node will be initialized.
            }
            node = next_node;
        }

        TaskExit();
    }
}


TaskHandle_t TimerInit(uint16_t stack, uint16_t period, uint8_t RespondLine, uint32_t deadline, uint8_t check_period)
{
    TaskHandle_t self = NULL;
    rb_root_init(&ClockTree);
    TaskCreate((TaskFunction_t)timer_check,
                stack,
                NULL,
               period,
               RespondLine,
               deadline,
                &self);
    TimerCheckPeriod = AbsoluteClock + check_period;
    return self;
}


timer_struct *TimerCreat(TimerFunction_t CallBackFun, uint32_t period, uint8_t timer_flag)
{
    timer_struct *timer = heap_malloc(sizeof(timer_struct));
    *timer = (timer_struct){
            .TimerPeriod = period,
            .CallBackFun = CallBackFun,
            .TimerStopFlag = timer_flag
    };
    rb_node_init(&(timer->TimerNode));
    timer->TimerNode.value = AbsoluteClock + period;
    ClockTreeAdd(timer);
    return timer;
}


void TimerDelete(TimerHandle timer)
{
    heap_free(timer);
}


uint8_t TimerRerun(timer_struct *timer, uint8_t timer_flag)
{
    ClockTreeAdd(timer);
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
    ClockListRemove(timer);
    return atomic_set_return(stop, (uint32_t *)&(timer->TimerStopFlag));
}








