

#include "port.h"
#include "schedule.h"


extern TaskHandle_t volatile schedule_currentTCB;
__attribute__( ( naked ) ) void  SVC_Handler( void )
{
    __asm volatile (
            "	ldr	r3, CurrentTCBConst2		\n"
            "	ldr r1, [r3]					\n"
            "	ldr r0, [r1]					\n"
            "	ldmia r0!, {r4-r11}				\n"
            "	msr psp, r0						\n"
            "	isb								\n"
            "	mov r0, #0 						\n"
            "	msr	basepri, r0					\n"
            "	orr r14, #0xd					\n"
            "	bx r14							\n"
            "									\n"
            "	.align 4						\n"
            "CurrentTCBConst2: .word schedule_currentTCB				\n"
            );
}

extern void TaskSwitchContext(void);
__attribute__( ( naked ) ) void PendSV_Handler( void )
{
    __asm volatile
            (
            "	mrs r0, psp							\n"
            "	isb									\n"
            "										\n"
            "	ldr	r3, CurrentTCBConst			    \n"
            "	ldr	r2, [r3]						\n"
            "										\n"
            "	stmdb r0!, {r4-r11}					\n"
            "	str r0, [r2]						\n"
            "										\n"
            "	stmdb sp!, {r3, r14}				\n"
            "	mov r0, %0							\n"
            "	msr basepri, r0						\n"
            "   dsb                                 \n"
            "   isb                                 \n"
            "	bl TaskSwitchContext				\n"
            "	mov r0, #0							\n"
            "	msr basepri, r0						\n"
            "	ldmia sp!, {r3, r14}				\n"
            "										\n"
            "	ldr r1, [r3]						\n"
            "	ldr r0, [r1]						\n"
            "	ldmia r0!, {r4-r11}					\n"
            "	msr psp, r0							\n"
            "	isb									\n"
            "	bx r14								\n"
            "	nop									\n"
            "	.align 4							\n"
            "CurrentTCBConst: .word schedule_currentTCB	\n"
            ::"i" ( configShieldInterPriority )
            );
}



__attribute__((always_inline)) inline uint32_t  xEnterCritical( void )
{
    uint32_t xReturn;
    uint32_t temp;

    __asm volatile(
            " cpsid i               \n"
            " mrs %0, basepri       \n"
            " mov %1, %2			\n"
            " msr basepri, %1       \n"
            " dsb                   \n"
            " isb                   \n"
            " cpsie i               \n"
            : "=r" (xReturn), "=r"(temp)
            : "r" (configShieldInterPriority)
            : "memory"
            );

    return xReturn;
}

__attribute__((always_inline)) inline void xExitCritical( uint32_t xReturn )
{
    __asm volatile(
            " cpsid i               \n"
            " msr basepri, %0       \n"
            " dsb                   \n"
            " isb                   \n"
            " cpsie i               \n"
            :: "r" (xReturn)
            : "memory"
            );
}


__attribute__( ( naked ) )  void StartFirstTask(void)
{
    /* Start the first task. */
    __asm volatile (
            " ldr r0, =0xE000ED08 	\n"/* Use the NVIC offset register to locate the stack. */
            " ldr r0, [r0] 			\n"
            " ldr r0, [r0] 			\n"
            " msr msp, r0			\n"/* Set the msp back to the start of the stack. */
            " cpsie i				\n"/* Globally enable interrupts. */
            " cpsie f				\n"
            " dsb					\n"
            " isb					\n"
            " svc 0					\n"/* System call to start first task. */
            " nop					\n"//wait
            " .ltorg				\n"
            );
}
