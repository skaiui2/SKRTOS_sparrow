//
// Created by el on 2024/11/9.
//

#ifndef STM32F1_HEAP_H
#define STM32F1_HEAP_H
#include "schedule.h"

void *heap_malloc(size_t WantSize);
void heap_free(void *xReturn);


#endif //STM32F1_HEAP_H
