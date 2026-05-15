/* ---------------------------------------------------------------------------------------*/
/*  @file:    startup_MCIMX6Y2.S                                                          */
/*  @purpose: CMSIS Cortex-A7 Core Device Startup File                                    */
/*            MCIMX6Y2                                                                    */
/*  @version:  2.0                                                                        */
/*  @date:     2016-8-24                                                                  */
/*  @build:   b160722                                                                     */
/* ---------------------------------------------------------------------------------------*/
/*                                                                                        */
/* Copyright (c) 1997 - 2016 , Freescale Semiconductor, Inc.                              */
/* All rights reserved.                                                                   */
/*                                                                                        */
/* Redistribution and use in source and binary forms, with or without modification,       */
/* are permitted provided that the following conditions are met:                          */
/*                                                                                        */
/* o Redistributions of source code must retain the above copyright notice, this list     */
/*   of conditions and the following disclaimer.                                          */
/*                                                                                        */
/* o Redistributions in binary form must reproduce the above copyright notice, this       */
/*   list of conditions and the following disclaimer in the documentation and/or          */
/*   other materials provided with the distribution.                                      */
/*                                                                                        */
/* o Neither the name of copyright holder nor the names of its               */
/*   contributors may be used to endorse or promote products derived from this            */
/*   software without specific prior written permission.                                  */
/*                                                                                        */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND        */
/* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED          */
/* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                 */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR       */
/* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES         */
/* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;           */
/* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON         */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT                */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS          */
/* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/*****************************************************************************/
/* Version: GCC for ARM Embedded Processors                                  */
/*****************************************************************************/
    .syntax unified
    .arch armv7-a

    .section .isr_vector, "a"
    .align 2
    .globl __isr_vector
__isr_vector:
    ldr     pc, =Reset_Handler           /* Reset                  */
    ldr     pc, =Undefined_Handler       /* Undefined instructions */
    ldr     pc, =SVC_Handler             /* Supervisor Call        */
    ldr     pc, =PrefAbort_Handler       /* Prefetch abort         */
    ldr     pc, =DataAbort_Handler       /* Data abort             */
    .word   0                            /* RESERVED               */
    ldr     pc, =IRQ_Handler             /* IRQ interrupt          */
    ldr     pc, =FIQ_Handler             /* FIQ interrupt          */

    .size    __isr_vector, . - __isr_vector

    .text
    .arm

/* Reset Handler */

    .arm
    .align 2
    .globl   Reset_Handler
    .weak    Reset_Handler
    .type    Reset_Handler, %function
Reset_Handler:
    cpsid   i               /* Mask interrupts */

    /* Reset SCTlr Settings */
    mrc     p15, 0, r0, c1, c0, 0     /* Read CP15 System Control register                  */
    bic     r0,  r0, #(0x1 << 12)     /* Clear I bit 12 to disable I Cache                  */
    bic     r0,  r0, #(0x1 <<  2)     /* Clear C bit  2 to disable D Cache                  */
    bic     r0,  r0, #0x2             /* Clear A bit  1 to disable strict alignment         */
    bic     r0,  r0, #(0x1 << 11)     /* Clear Z bit 11 to disable branch prediction        */
    bic     r0,  r0, #0x1             /* Clear M bit  0 to disable MMU                      */
    mcr     p15, 0, r0, c1, c0, 0     /* Write value back to CP15 System Control register   */

    /* Set up stack for IRQ, System/User and Supervisor Modes */
    cps     #0x12                /* Enter IRQ mode                */
    ldr     sp, =__IStackTop     /* Set up IRQ handler stack      */

    cps     #0x1F                /* Enter System mode             */
    ldr     sp, =__CStackTop     /* Set up System/User Mode stack */

    cps     #0x13                /* Enter Supervisor mode         */
    ldr     sp, =__CStackTop     /* Set up Supervisor Mode stack  */

#ifndef __NO_SYSTEM_INIT
    ldr     r0,=SystemInit
    blx     r0
#endif
    cpsie   i                    /* Unmask interrupts             */

/*     Loop to copy data from read only memory to RAM. The ranges
 *      of copy from/to are specified by following symbols evaluated in
 *      linker script.
 *      __etext: End of code section, i.e., begin of data sections to copy from.
 *      __data_start__/__data_end__: RAM address range that data should be
 *      copied to. Both must be aligned to 4 bytes boundary.  */

    ldr    r1, =__etext

/* Here are two copies of loop implemenations. First one favors code size
 * and the second one favors performance. Default uses the first one.
 * Change to "#if 0" to use the second one */
#ifdef __STARTUP_INITIALIZE_NONCACHEDATA
    ldr    r2, =__noncachedata_start__
    ldr    r3, =__noncachedata_end__
#if 1
.LC0:
    cmp     r2, r3
    ittt    lt
    ldrlt   r0, [r1], #4
    strlt   r0, [r2], #4
    blt    .LC0
#else
    subs    r3, r2
    ble    .LC1
.LC0:
    subs    r3, #4
    ldr     r0, [r1, r3]
    str     r0, [r2, r3]
    bgt    .LC0
.LC1:
#endif
    ldr    r1,=__DATA_ROM
#endif /* __STARTUP_INITIALIZE_NONCACHEDATA */

    ldr    r2, =__data_start__
    ldr    r3, =__data_end__
#if 1
.LC2:
    cmp     r2, r3
    ittt    lt
    ldrlt   r0, [r1], #4
    strlt   r0, [r2], #4
    blt    .LC2
#else
    subs    r3, r2
    ble    .LC3
.LC2:
    subs    r3, #4
    ldr     r0, [r1, r3]
    str     r0, [r2, r3]
    bgt    .LC2
.LC3:
#endif

/*     Loop to copy ocram data from read only memory to ocram. The ranges
 *      of copy from/to are specified by following symbols evaluated in
 *      linker script.
 *      __ocram_data_start__/__ocram_data_end__: OCRAM address range that data should be
 *      copied to. Both must be aligned to 4 bytes boundary.  */

    ldr    r2, =__ocram_data_start__
    ldr    r3, =__ocram_data_end__

#if 1
.LC4:
    cmp     r2, r3
    ittt    lt
    ldrlt   r0, [r1], #4
    strlt   r0, [r2], #4
    blt    .LC4
#else
    subs    r3, r2
    ble    .LC1
.LC4:
    subs    r3, #4
    ldr     r0, [r1, r3]
    str     r0, [r2, r3]
    bgt    .LC4
.LC5:
#endif

#ifdef __STARTUP_CLEAR_BSS
/*     This part of work usually is done in C library startup code. Otherwise,
 *     define this macro to enable it in this startup.
 *
 *     Loop to zero out BSS section, which uses following symbols
 *     in linker script:
 *      __bss_start__: start of BSS section. Must align to 4
 *      __bss_end__: end of BSS section. Must align to 4
 */
    ldr r1, =__bss_start__
    ldr r2, =__bss_end__

    movs    r0, 0
.LC6:
    cmp     r1, r2
    itt     lt
    strlt   r0, [r1], #4
    blt    .LC6
#endif /* __STARTUP_CLEAR_BSS */

#ifndef __START
#define __START _start
#endif
#ifndef __ATOLLIC__
    ldr   r0,=__START
    blx   r0
#else
    ldr   r0,=__libc_init_array
    blx   r0
    ldr   r0,=main
    bx    r0
#endif
    .pool
    .size Reset_Handler, . - Reset_Handler

    .align  2
    .arm
    .weak Undefined_Handler
    .type Undefined_Handler, %function
Undefined_Handler:
    b Undefined_Handler
    .size Undefined_Handler, . - Undefined_Handler

    .align 2
    .arm
    .weak SVC_Handler
    .type SVC_Handler, %function
SVC_Handler:
    ldr   r0,=SVC_Handler
    bx    r0
    .size SVC_Handler, . - SVC_Handler

    .align 2
    .arm
    .weak PrefAbort_Handler
    .type PrefAbort_Handler, %function
PrefAbort_Handler:
    ldr   r0,=PrefAbort_Handler
    bx    r0
    .size PrefAbort_Handler, . - PrefAbort_Handler

    .align 2
    .arm
    .weak DataAbort_Handler
    .type DataAbort_Handler, %function
DataAbort_Handler:
    ldr   r0,=DataAbort_Handler
    bx    r0
    .size DataAbort_Handler, . - DataAbort_Handler

    .align 2
    .arm
    .weak IRQ_Handler
    .type IRQ_Handler, %function
IRQ_Handler:
    push    {lr}                         /* Save return address+4                                */
    push    {r0-r3, r12}                 /* Push caller save registers                           */

    MRS     r0, spsr                     /* Save SPRS to allow interrupt reentry                 */
    push    {r0}

    MRC     P15, 4, r1, C15, C0, 0       /* Get GIC base address                                 */
    ADD     r1, r1, #0x2000              /* r1: GICC base address                                */
    LDR     r0, [r1, #0xC]               /* r0: IAR                                              */

    push    {r0, r1}

    CPS     #0x13                        /* Change to Supervisor mode to allow interrupt reentry */

    push    {lr}                         /* Save Supervisor lr                                   */
    LDR     r2, =SystemIrqHandler
    BLX     r2                           /* Call SystemIrqHandler with param GCC                 */
    POP     {lr}

    CPS     #0x12                        /* Back to IRQ mode                                     */

    POP     {r0, r1}

    STR     r0, [r1, #0x10]              /* Now IRQ handler finished: write to EOIR              */

    POP     {r0}
    MSR     spsr_cxsf, r0

    POP     {r0-r3, r12}
    POP     {lr}
    SUBS    pc, lr, #4
    .size IRQ_Handler, . - IRQ_Handler

    .align 2
    .arm
    .weak FIQ_Handler
    .type FIQ_Handler, %function
FIQ_Handler:
    ldr   r0,=FIQ_Handler
    bx    r0
    .size FIQ_Handler, . - FIQ_Handler

    .end
