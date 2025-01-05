
#ifndef LIST_H
#define LIST_H

#include<stdint.h>
#include "class.h"


/*this is public*/
Class(ListNode)
{
    uint32_t value;
    ListNode* prev;
    ListNode* next;
    void    *TheList;
};

Class(TheList)
{
    uint8_t count;
    ListNode *head;
    ListNode *tail;
    ListNode *SaveNode;
};



void ListInit(TheList *xList);
void ListAdd(TheList *xList, ListNode *new_node);
void ListRemove(TheList *xList, ListNode *rm_node);

#endif
