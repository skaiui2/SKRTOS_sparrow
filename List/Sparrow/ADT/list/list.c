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

#include <stddef.h>
#include "list.h"


void ListInit(TheList *xList)
{
    xList->count = 0;
    xList->head = NULL;
    xList->tail = NULL;
    xList->SaveNode = NULL;
}

void ListNodeInit(ListNode *node)
{
    node->prev = NULL;
    node->next = NULL;
    node->value = 0;
    node->TheList = NULL;
}


void InsertFirst(TheList *xList, ListNode *new_node)
{
    xList->head = new_node;
    xList->tail = new_node;
    new_node->prev = (ListNode *)xList;
    new_node->next = new_node;

}


void InsertHead(TheList *xList, ListNode *new_node)
{
    new_node->prev = (ListNode*)xList;
    new_node->next = xList->head;

    xList->head->prev = new_node;

    xList->head = new_node;
    xList->tail->next = new_node;

}

void InsertTail(TheList *xList, ListNode *new_node)
{
    new_node->prev = xList->tail;
    new_node->next = xList->tail->next;

    xList->tail->next = new_node;

    xList->tail = new_node;

}


void InsertMiddle(TheList *xList, ListNode *new_node)
{
    ListNode *find_help;

    for(find_help = xList->head;find_help->next->value <= new_node->value; find_help = find_help->next)
    {
        //finding...
    }

    new_node->prev = find_help;
    new_node->next = find_help->next;

    find_help->next->prev = new_node;

    find_help->next = new_node;


}

/* use the table to record all condition,
 * separate the judgment conditions (like if else)  and excution
*/

uint8_t control(TheList *xList , ListNode *new_node)
{

    uint8_t rt = 0;

    if(xList->count== 0)       return rt;
    rt += 1;

    if( new_node->value <= xList->head->value)   return rt;
    rt += 1;

    if( new_node->value >= xList->tail->value)    return rt;
    rt += 1;

    if( ( ( new_node->value) > (xList->head->value)  ) && (new_node->value < xList->tail->value) )  return rt;


}


void dataflow(uint8_t rt,TheList *xList, ListNode *new_node)
{
    void (*ListInsert[])(TheList *xList, ListNode *new_node) = {
            InsertFirst,
            InsertHead,
            InsertTail,
            InsertMiddle,
    };
    ListInsert[rt](xList, new_node);

}

/*
 * you can use xlist->tail replace xlist->head,then the task will swap
 * the order of their execution in same priority.
 *  example:
 *  now the order of their execution in same priorty task is ab.ab.ab...,
 *  after replacing tail, it is ab.ba.ab.ba
 * */
void ListAdd(TheList *xList, ListNode *new_node)
{
    uint8_t op = control(xList,new_node);
    dataflow(op,xList, new_node);
    xList->SaveNode = xList->head;
    new_node->TheList = (void *)xList;

    xList->count += 1;
}

void RemoveLast( TheList *xList, ListNode *rm_node )
{
    xList->head = NULL;
    xList->tail = NULL;
}

void RemoveHead( TheList *xList, ListNode *rm_node )
{
    xList->head = rm_node->next;
    xList->tail->next = rm_node->next;
    rm_node->next->prev = rm_node->prev;

}

void RemoveMiddle( TheList *xList, ListNode *rm_node )
{
    rm_node->prev->next = rm_node->next;
    rm_node->next->prev = rm_node->prev;
}

void RemoveTail( TheList *xList, ListNode *rm_node )
{
    rm_node->prev->next =  rm_node->next;
    xList->tail = rm_node->prev;
}


uint8_t MoveControl(TheList *xList , ListNode *rm_node)
{
    uint8_t rt = 0;

    if( xList->count == 1)          return  rt;
    rt += 1;

    if(xList ->head == rm_node)       return rt;
    rt +=1;

    if( rm_node == xList->tail)   return rt;
    rt += 1;

    if( ( ( rm_node->value) >= (xList->head->value)  ) && (rm_node->value <= xList->tail->value) )   return rt;
}


void MoveFlow(uint8_t rt,TheList *xList, ListNode *rm_node)
{
    void (*ListRemove[])(TheList *xList, ListNode *rm_node) = {
            RemoveLast,
            RemoveHead,
            RemoveTail,
            RemoveMiddle
    };
    ListRemove[rt](xList, rm_node);
}

void ListRemove(TheList *xList, ListNode *rm_node)
{
    xList->SaveNode = xList->SaveNode->next;
    uint8_t op = MoveControl(xList,rm_node);
    MoveFlow(op,xList, rm_node);
    ListNodeInit(rm_node);
    xList->count -= 1;
}


