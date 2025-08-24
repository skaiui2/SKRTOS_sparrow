#include "quicksort.h"

void swap(int *a, int *b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

int part(int *arr, int low, int high) 
{
    int value = arr[high];
    int i = low - 1;
    int j = high;
    while(1) {
        while(arr[++i] < value);
        while(arr[--j] > value && j > low);
        if (i < j) {
            swap(&arr[i], &arr[j]);
        } else {
            break;
        }
    }
    swap(&arr[i], &arr[high]);
    return i;
}

void quickSort(int *arr, int low, int high) 
{
    if (low < high) {
        int i = part(arr, low, high);
        quickSort(arr, low, i -1);
        quickSort(arr, i+1, high);
    }
}

