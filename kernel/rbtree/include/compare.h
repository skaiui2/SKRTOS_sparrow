#ifndef COMPARE_H
#define COMPARE_H

#include <stdint.h>

static inline uint8_t compare_before_eq(uint32_t a, uint32_t b) {
	return (int32_t)(b - a) >= 0;
}

#endif
