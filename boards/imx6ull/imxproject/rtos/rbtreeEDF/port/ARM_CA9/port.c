


#include "class.h"
#include "schedule.h"
#include "config.h"
#include "port.h"

/* A variable is used to keep track of the critical section nesting.  This
variable has to be stored as part of the task context and must be initialised to
a non zero value to ensure interrupts don't inadvertently become unmasked before
the scheduler starts.  As it is stored as part of the task context it will
automatically be set to 0 when the first task is started. */
volatile uint32_t ulCriticalNesting = 9999UL;

/* Saved as part of the task context.  If ulPortTaskHasFPUContext is non-zero then
a floating point context must be saved and restored for the task. */
volatile uint32_t ulPortTaskHasFPUContext = 0;

/* Set to 1 to pend a context switch from an ISR. */
volatile uint32_t ulPortYieldRequired = 0;

/* Counts the interrupt nesting depth.  A context switch is only performed if
if the nesting depth is 0. */
volatile uint32_t ulPortInterruptNesting = 0UL;
/* Used in the asm file. */
__attribute__(( used )) const uint32_t ulICCIAR = portICCIAR_INTERRUPT_ACKNOWLEDGE_REGISTER_ADDRESS;
__attribute__(( used )) const uint32_t ulICCEOIR = portICCEOIR_END_OF_INTERRUPT_REGISTER_ADDRESS;
__attribute__(( used )) const uint32_t ulICCPMR	= portICCPMR_PRIORITY_MASK_REGISTER_ADDRESS;
__attribute__(( used )) const uint32_t ulMaxAPIPriorityMask = ( 20 << 3 );//only three bit for priority


extern void SystemClearSystickFlag(void);
void RTOS_Tick_Handler( void )
{
	__asm volatile ( "CPSID i" );
	portICCPMR_PRIORITY_MASK_REGISTER = ( uint32_t ) ( 20 << 3 );
	__asm volatile (	"dsb		\n"
						"isb		\n" );
	__asm volatile ( "CPSIE i" );

	CheckTicks();

	__asm volatile ( "CPSID i" );											
	portICCPMR_PRIORITY_MASK_REGISTER = 0XFFUL;			
	__asm volatile (	"DSB		\n"								
						"ISB		\n" );							
	__asm volatile ( "CPSIE i" );		

	SystemClearSystickFlag();
}


/*
 * If the program runs here, there is a problem with the use of the RTOS,
 * such as the stack allocation space is too small, and the use of undefined operations.
 */
volatile uint32_t error = 0;
void ErrorHandle(void)
{
    while (1){
        error++;
    }
}

uint32_t *StackInit( uint32_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{	
	pxTopOfStack -= 18;
	struct Stack_register *stack = (struct Stack_register *)pxTopOfStack;
	
	*stack = (struct Stack_register) {
		.xPSR = 0x1f | 0x20 ,
		.PC = (uint32_t)pxCode ,
		.LR = (uint32_t)ErrorHandle,
		.r0 = (uint32_t)pvParameters,
		.critical = 0,
		.floatReg = 0
	};
	
	return pxTopOfStack;
}



#define portBINARY_POINT_BITS			( ( uint8_t ) 0x03 )
#define portAPSR_MODE_BITS_MASK			( 0x1F )
#define portAPSR_USER_MODE				( 0x10 )
void StartFirstTask(void)
{
	uint32_t ulAPSR;
	volatile uint8_t ucMaxPriorityValue;

	volatile uint32_t ulOriginalPriority = *( volatile uint8_t * const ) ( 0x00a01000UL + 0x400UL );

	*( volatile uint8_t * const ) ( 0x00a01000UL + 0x400UL ) = 0xff;

	ucMaxPriorityValue = *( volatile uint8_t * const ) ( 0x00a01000UL + 0x400UL );

	while( ( ucMaxPriorityValue & 0x01 ) != 0x01 ) {
		ucMaxPriorityValue >>= ( uint8_t ) 0x01;
	}

	*( volatile uint8_t * const ) ( 0x00a01000UL + 0x400UL ) = ulOriginalPriority;
	
	
	__asm volatile ( "MRS %0, APSR" : "=r" ( ulAPSR ) );
	ulAPSR &= portAPSR_MODE_BITS_MASK;
	
	if( ulAPSR != portAPSR_USER_MODE )
	{
		if( ( portICCBPR_BINARY_POINT_REGISTER & portBINARY_POINT_BITS ) <= portMAX_BINARY_POINT_VALUE )
		{
			//disable irq until next tick.
			__asm volatile ( "CPSID i" );									
		
			SystemSetupSystick(configTICK_RATE_HZ, (void *)RTOS_Tick_Handler, configUNIQUE_INTERRUPT_PRIORITIES - 2);
			
			vPortRestoreTaskContext();
		}
	}

}



uint32_t xEnterCritical()
{
	__asm volatile ( "CPSID i" );	
	return 0;
}

void xExitCritical(uint32_t xre)
{
	__asm volatile ( "CPSIE i" );	
} 



void vApplicationFPUSafeIRQHandler( uint32_t ulICCIAR )
{
	( void ) ulICCIAR;

}



