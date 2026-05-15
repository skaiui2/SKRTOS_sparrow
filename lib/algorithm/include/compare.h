#ifndef COMPARE_H
#define COMPARE_H

#include <stdint.h>
/*
 * If we know a,b both less than the half of number axis.
 * Spill or not,if a is ahead of b, return false.Or else, return true.
 */
static inline uint8_t compare_before(uint32_t a, uint32_t b) {
    return (int)(a - b) < 0;
}

static inline uint8_t compare_before_eq(uint32_t a, uint32_t b) {
    return (int)(a - b) <= 0;
}

/*
 * If we know a,b both less than the half of number axis.
 * Spill or not,if a is ahead of b, return true.Or else, return false.
 */
static inline uint8_t compare_after(uint32_t a, uint32_t b) {
    return (int)(a - b) > 0;
}

static inline uint8_t compare_after_eq(uint32_t a, uint32_t b) {
    return (int)(a - b) >= 0;
}


#endif