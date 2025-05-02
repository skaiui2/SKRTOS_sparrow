#ifndef RADIX_H
#define RADIX_H
#include "macro.h"


#define SIZE_LEVEL  2
#define BIT_LEVEL    1


typedef struct radix_tree_root *rad_root_handle;

void radix_tree_init(rad_root_handle root);
int radix_tree_insert(rad_root_handle root, unsigned long index, void *item);
void *radix_tree_lookup(rad_root_handle root, unsigned long index);
void *radix_tree_delete(rad_root_handle root, unsigned long index);


#endif 
