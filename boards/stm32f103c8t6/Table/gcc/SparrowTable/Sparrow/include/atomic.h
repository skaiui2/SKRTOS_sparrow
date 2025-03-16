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

#ifndef ATOMIC_H
#define ATOMIC_H
#include<stdint.h>


/* You must use the atomic for global variable!!Not a local variable or a malloc address.
 * Operand must be byte aligned!!
 *
 */

/*
 * ldrex is used to read data from memory address i to register %0 (output).
 * To perform the specified operation (such as add or sub), the operands are registers %0 and v, and the result is stored in %0.
 * Using strex attempts to store the result back to memory address v,store the flag to tmp, returning 0 if successful and non-zero if unsuccessful.
 * If the storage fails, jump back to TAB 1 and try again.
 */
#define ATOMIC_OP_RETURN(op)                                \
static inline int atomic_##op##_return( uint32_t i,uint32_t *v)        \
{                                                           \
    uint32_t output;                                             \
    uint32_t tmp;                                                \
    __asm volatile (                                        \
    "1: ldrex     %0,[%2]		\n"                         \
    "   "#op"   %0,%0,%3   		\n"                         \
    "	strex     %1,%0,[%2]		\n"                     \
    "   teq     %1,#0           \n"                         \
    "   bne     1b              \n"                         \
    :   "=&r" (output),"=&r"(tmp)                           \
    :   "r" (v),"r"(i)                                      \
    :   "cc"                                                \
    );                                                      \
    return  output;                                         \
}

//like upwards,but no return replace by changing the memory.

#define ATOMIC_OP(op)                                \
static inline void atomic_##op( uint32_t i,uint32_t *v)        \
{                                                           \
    uint32_t tmp;                                                \
    __asm volatile (                                        \
    "1: ldrex     %0,[%2]		\n"                         \
    "   "#op"   %0,%0,%1   		\n"                         \
    "	strex     %1,%0,[%2]		\n"                         \
    "   teq     %1,#0           \n"                         \
    "   bne     1b              \n"                         \
    :   "=&r"(tmp)                           \
    :   "r" (i),"r"(v)                                      \
    :   "cc"                                                \
    );                                                      \
}

#define ATOMIC_OPS(op)  ATOMIC_OP(op) ATOMIC_OP_RETURN(op)

ATOMIC_OPS(add)

ATOMIC_OPS(sub)

#define atomic_inc(v) (atomic_add(1,v))
#define atomic_dec(v) (atomic_sub(1,v))


#define atomic_inc_return(v) (atomic_add_return(1,v))
#define atomic_dec_return(v) (atomic_sub_return(1,v))



static inline uint32_t atomic_set_return(uint32_t i, uint32_t *v) {
    uint32_t tmp, res;
    __asm volatile (
            "1: ldrex %0, [%2]     \n"
            "   strex %1, %3, [%2] \n"
            "   teq %1, #0         \n"
            "   bne 1b             \n"
            : "=&r" (tmp), "=&r" (res)
            : "r" (v), "r" (i)
            : "cc"
            );
    return res;
}



static inline void atomic_set(uint32_t i, uint32_t *v) {
    uint32_t tmp;
    __asm volatile (
            "1: ldrex %0, [%1]     \n"
            "   strex %0, %2, [%1] \n"
            "   teq %0, #0         \n"
            "   bne 1b             \n"
            : "=&r" (tmp)
            : "r" (v), "r" (i)
            : "cc"
            );
}










#endif
