#include "rbtree.h"
#include "class.h"
#include "stdint.h"
#include "stddef.h"

typedef void (*TimerFunction_t)(void *);

Class(timer_struct) {
    rb_node TimerNode;
    uint32_t TimerPeriod;
    TimerFunction_t CallBackFun;
    uint8_t TimerStopFlag;
};

#define stop 0
#define run 1

int fired1 = 0, fired2 = 0;

void cb1(void *t) { fired1 = 1; }
void cb2(void *t) { fired2 = 1; }

int main() {
    rb_root ClockTree;
    rb_root_init(&ClockTree);
    
    timer_struct timers[3];
    
    /* TEST 1: Empty tree - rb_first returns NULL */
    rb_node *node = rb_first(&ClockTree);
    if (node != NULL) return 1; /* FAIL */
    
    /* TEST 2: insert one timer, expired, remove it, tree becomes empty again */
    timers[0].TimerNode.value = 10;
    timers[0].TimerStopFlag = stop;
    timers[0].CallBackFun = cb1;
    rb_Insert_node(&ClockTree, &timers[0].TimerNode);
    
    /* Pretend AbsoluteClock = 20 */
    node = rb_first(&ClockTree);
    if (node == NULL) return 2;
    if (node->value > 20) return 3; /* should be expired */
    
    /* Remove it (stop flag) */
    rb_node *next_node = rb_next(node);
    rb_remove_node(&ClockTree, &timers[0].TimerNode);
    node = next_node;
    
    /* Tree should be empty now */
    if (ClockTree.count != 0) return 4;
    
    /* TEST 3: Two timers, one expires, capture next before removal */
    timers[1].TimerNode.value = 5;
    timers[1].TimerStopFlag = stop;
    timers[1].CallBackFun = cb2;
    timers[2].TimerNode.value = 15;
    timers[2].TimerStopFlag = run; /* re-trigger */
    timers[2].CallBackFun = cb1;
    rb_Insert_node(&ClockTree, &timers[1].TimerNode);
    rb_Insert_node(&ClockTree, &timers[2].TimerNode);
    
    /* Iterate - first node (value=5) expired */
    node = rb_first(&ClockTree);
    if (node == NULL) return 5;
    if (node->value != 5) return 6;
    
    /* Capture next BEFORE removal (UAF fix) */
    next_node = rb_next(node);
    rb_remove_node(&ClockTree, node); /* remove timers[1] */
    node = next_node;
    
    /* Now node should be 15 */
    if (node == NULL) return 7;
    if (node->value != 15) return 8;
    
    /* TEST 4: rb_next returns NULL (end of tree) */
    next_node = rb_next(node);
    if (next_node != NULL) return 9; /* rb_next of last node should return NULL */
    
    return 0; /* ALL PASS */
}
