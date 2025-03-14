
#ifndef CONFIG_H
#define CONFIG_H







#define configTICK_RATE_HZ (1000)




/* GIC information defintions. */
#define configINTERRUPT_CONTROLLER_BASE_ADDRESS 0x00a01000UL
#define configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET 0x1000UL
#define configUNIQUE_INTERRUPT_PRIORITIES 32


/* Definitions that map the FreeRTOS port exception handlers to startup handler names. */
#define RTOS_IRQ_Handler IRQ_Handler
#define RTOS_SWI_Handler SVC_Handler










#endif
