    RSEG    CODE:CODE(2)
    thumb

    EXTERN pxCurrentTCB
    EXTERN vTaskSwitchContext
    EXTERN ReadyBitTable


    PUBLIC vPortStartFirstTask
    PUBLIC PendSV_Handler
    PUBLIC SVC_Handler
    PUBLIC xEnterCritical
    PUBLIC xExitCritical
    PUBLIC FindHighestPriority

/*-----------------------------------------------------------*/

PendSV_Handler:
    mrs r0, psp
    isb
    ldr r3, =pxCurrentTCB           /* Get the location of the current TCB. */
    ldr r2, [r3]

    stmdb r0!, {r4-r11}             /* Save the remaining registers. */
    str r0, [r2]                    /* Save the new top of stack into the first member of the TCB. */

    stmdb sp!, {r3, r14}
    mov r0, #191
    msr basepri, r0
    dsb
    isb
    bl vTaskSwitchContext
    mov r0, #0
    msr basepri, r0
    ldmia sp!, {r3, r14}

    ldr r1, [r3]
    ldr r0, [r1]                    /* The first item in pxCurrentTCB is the task top of stack. */
    ldmia r0!, {r4-r11}             /* Pop the registers. */
    msr psp, r0
    isb
    bx r14

/*-----------------------------------------------------------*/

SVC_Handler:
    /* Get the location of the current TCB. */
    ldr r3, =pxCurrentTCB
    ldr r1, [r3]
    ldr r0, [r1]
    /* Pop the core registers. */
    ldmia r0!, {r4-r11}
    msr psp, r0
    isb
    mov r0, #0
    msr basepri, r0
    orr r14, r14, #13
    bx r14

/*-----------------------------------------------------------*/

vPortStartFirstTask:
    /* Use the NVIC offset register to locate the stack. */
    ldr r0, =0xE000ED08
    ldr r0, [r0]
    ldr r0, [r0]
    /* Set the msp back to the start of the stack. */
    msr msp, r0
    /* Call SVC to start the first task, ensuring interrupts are enabled. */
    cpsie i
    cpsie f
    dsb
    isb
    svc 0
    
    

/*-----------------------------------------------------------*/

xEnterCritical:
    cpsid i
    mrs r0, basepri
    mov r1, #191
    msr basepri, r1
    dsb
    isb
    cpsie i
    bx lr

/*-----------------------------------------------------------*/

xExitCritical:
    cpsid i
    msr basepri, r0
    dsb
    isb
    cpsie i
    bx lr

/*-----------------------------------------------------------*/

FindHighestPriority:
    ldr r1, =ReadyBitTable 
    ldr r2, [r1]
    clz r0, r2
    mov r1, #31
    sub r0, r1, r0
    bx lr

    END
