/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

#include "core_cm3.h"
#include<stdint.h>
#include<stdlib.h>


#define aligment_byte               0x07
#define configSysTickClockHz			( ( unsigned long ) 72000000 )
#define configTickRateHz			( ( uint32_t ) 1000 )
#define configShieldInterPriority 	191
#define config_heap   8*1024
#define configMaxPriori  4


#define Class(class)    \
typedef struct  class  class;\
struct class

#define MIN_size     ((size_t) (heapstructSize << 1))

Class(heap_node){
    heap_node *next;
    size_t blocksize;
};

Class(xheap){
    heap_node head;
    heap_node *tail;
    size_t allsize;
};

xheap theheap = {
        .tail = NULL,
        .allsize = config_heap,
};

static  uint8_t allheap[config_heap];
static const size_t heapstructSize = (sizeof(heap_node) + (size_t)(aligment_byte)) &~(aligment_byte);

void heap_init()
{
    heap_node *firstnode;
    uint32_t start_heap ,end_heap;
    //get start address
    start_heap =(uint32_t) allheap;
    if( (start_heap & aligment_byte) != 0){
        start_heap += aligment_byte ;
        start_heap &= ~aligment_byte;
        theheap.allsize -=  (size_t)(start_heap - (uint32_t)allheap);//byte aligment means move to high address,so sub it!
    }
    theheap.head.next = (heap_node *)start_heap;
    theheap.head.blocksize = (size_t)0;
    end_heap  = ( start_heap + (uint32_t)config_heap);
    if( (end_heap & aligment_byte) != 0){
        end_heap += aligment_byte ;
        end_heap &= ~aligment_byte;
        theheap.allsize =  (size_t)(end_heap - start_heap );
    }
    theheap.tail = (heap_node*)end_heap;
    theheap.tail->blocksize  = 0;
    theheap.tail->next =NULL;
    firstnode = (heap_node*)start_heap;
    firstnode->next = theheap.tail;
    firstnode->blocksize = theheap.allsize;
}

void *heap_malloc(size_t wantsize)
{
    heap_node *prevnode;
    heap_node *usenode;
    heap_node *newnode;
    size_t aligmentrequisize;
    void *xReturn = NULL;
    wantsize += heapstructSize;
    if((wantsize & aligment_byte) != 0x00)
    {
        aligmentrequisize = aligment_byte - (wantsize & aligment_byte);
        wantsize += aligmentrequisize;
    }//You can add the TaskSuspend function ,that make here be a atomic operation
    if(theheap.tail== NULL )
    {
        heap_init();
    }//Resume
    prevnode = &theheap.head;
    usenode = theheap.head.next;
    while((usenode->blocksize) < wantsize )
    {
        prevnode = usenode;
        usenode = usenode ->next;
    }
    xReturn = (void*)( ( (uint8_t*)usenode ) + heapstructSize );
    prevnode->next = usenode->next ;
    if( (usenode->blocksize - wantsize) > MIN_size )
    {
        newnode = (void*)( ( (uint8_t*)usenode ) + wantsize );
        newnode->blocksize = usenode->blocksize - wantsize;
        usenode->blocksize = wantsize;
        newnode->next = prevnode->next ;
        prevnode->next = newnode;
    }
    theheap.allsize-= usenode->blocksize;
    usenode->next = NULL;
    return xReturn;
}

static void InsertFreeBlock(heap_node* xInsertBlock);
void heap_free(void *xret)
{
    heap_node *xlink;
    uint8_t *xFree = (uint8_t*)xret;

    xFree -= heapstructSize;
    xlink = (void*)xFree;
    theheap.allsize += xlink->blocksize;
    InsertFreeBlock((heap_node*)xlink);
}

static void InsertFreeBlock(heap_node* xInsertBlock)
{
    heap_node *first_fitnode;
    uint8_t* getaddr;

    for(first_fitnode = &theheap.head;first_fitnode->next < xInsertBlock;first_fitnode = first_fitnode->next)
    { /*finding the fit node*/ }

    xInsertBlock->next = first_fitnode->next;
    first_fitnode->next = xInsertBlock;

    getaddr = (uint8_t*)xInsertBlock;
    if((getaddr + xInsertBlock->blocksize) == (uint8_t*)(xInsertBlock->next))
    {
        if(xInsertBlock->next != theheap.tail )
        {
            xInsertBlock->blocksize += xInsertBlock->next->blocksize;
            xInsertBlock->next = xInsertBlock->next->next;
        }
        else
        {
            xInsertBlock->next = theheap.tail;
        }
    }
    getaddr = (uint8_t*)first_fitnode;
    if((getaddr + first_fitnode->blocksize) == (uint8_t*) xInsertBlock)
    {
        first_fitnode->blocksize += xInsertBlock->blocksize;
        first_fitnode->next = xInsertBlock->next;
    }
}


Class(Stack_register)
{
    //automatic stacking
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    //manual stacking
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t LR;
    uint32_t PC;
    uint32_t xPSR;
};

Class(TCB_t)
{
    volatile uint32_t * pxTopOfStack;
    unsigned long uxPriority;
    uint32_t * pxStack;
    Stack_register *self_stack;//Save the status of the stack in the task !You can use gdb to debug it!
};
typedef  TCB_t         *TaskHandle_t;

__attribute__( ( used ) )  TCB_t * volatile pxCurrentTCB = NULL;
typedef void (* TaskFunction_t)( void * );


#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler

void __attribute__( ( naked ) )  vPortSVCHandler( void )
{
    __asm volatile (
            "	ldr	r3, pxCurrentTCBConst2		\n"
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
            "pxCurrentTCBConst2: .word pxCurrentTCB				\n"
            );
}

void __attribute__( ( naked ) )  xPortPendSVHandler( void )
{
    __asm volatile
            (
            "	mrs r0, psp							\n"
            "	isb									\n"
            "										\n"
            "	ldr	r3, pxCurrentTCBConst			\n"
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
            "	bl vTaskSwitchContext				\n"
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
            "pxCurrentTCBConst: .word pxCurrentTCB	\n"
            ::"i" ( configShieldInterPriority )
            );
}

#define switchTask()\
*( ( volatile uint32_t * ) 0xe000ed04 ) = 1UL << 28UL;

uint32_t  NextTicks = ~(uint32_t)0;
uint32_t  TicksBase = 0;

uint32_t ReadyBitTable = 0;
TaskHandle_t TcbTaskTable[configMaxPriori];

uint32_t DelayBitTable = 0;
uint32_t TicksTable[configMaxPriori];
uint32_t TicksTableAssist[configMaxPriori];
uint32_t* WakeTicksTable;
uint32_t* OverWakeTicksTable;
#define TicksTableInit( ) {  \
    WakeTicksTable = TicksTable;    \
    OverWakeTicksTable = TicksTableAssist;  \
}
#define TicksTableSwitch( ){    \
    uint32_t *temp;             \
    temp = WakeTicksTable;      \
    WakeTicksTable = OverWakeTicksTable; \
    OverWakeTicksTable = temp;  \
    }

uint32_t SuspendBitTable = 0;

//the table is defined for signal mechanism
uint32_t BlockedBitTable = 0;

void SetTaskPriority( TCB_t *self )
{
    TcbTaskTable[self->uxPriority] = self;
}

/*The RTOS delay will switch the task.It is used to liberate low-priority task*/
void TaskDelay( uint16_t ticks )
{
    uint32_t WakeTime = TicksBase + ticks;
    TCB_t *self = pxCurrentTCB;
    if( WakeTime < TicksBase)
    {
        OverWakeTicksTable[self->uxPriority] = WakeTime;
    }
    else
    {
        WakeTicksTable[self->uxPriority] = WakeTime;
    }
    /* This is a useless operation(for DelayBitTable), it can be discarded.
     * But it is retained for the sake of normativity.For example, view the status of all current tasks.*/
    DelayBitTable |= (1 << (self->uxPriority) );
    ReadyBitTable &= ~(1 << (self->uxPriority) );
    switchTask();
}

void CheckTicks( void )
{
    TicksBase += 1;
    if( TicksBase == 0){
        TicksTableSwitch( );
    }
    for(int i=0 ; i < configMaxPriori;i++)
    {
        if(  WakeTicksTable[i]  > 0)
        {
            if ( TicksBase >= WakeTicksTable[i] )
            {
                WakeTicksTable[i] = 0;
                DelayBitTable &= ~(1 << i );//it is retained for the sake of normativity.
                ReadyBitTable |= (1 << i);
            }
        }
    }
    switchTask();
}


__attribute__((always_inline)) uint32_t xEnterCritical( void )
{
    uint32_t xReturn;

    __asm volatile(
            " cpsid i               \n"
            " mrs %0, basepri       \n"
            " msr basepri, %1       \n"
            " dsb                   \n"
            " isb                   \n"
            " cpsie i               \n"
            : "=r" (xReturn)
            : "r" (configShieldInterPriority)
            : "memory"
            );

    return xReturn;
}

__attribute__((always_inline)) void xEixtCritical( uint32_t xReturn )
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


void SysTick_Handler(void)
{
    uint32_t xre = xEnterCritical();
    CheckTicks();
    xEixtCritical(xre);
}

uint32_t * pxPortInitialiseStack( uint32_t * pxTopOfStack,
                                  TaskFunction_t pxCode,
                                  void * pvParameters ,
                                  TaskHandle_t * const self)
{
    pxTopOfStack -= 16;
    Stack_register *Stack = (Stack_register *)pxTopOfStack;

    Stack->xPSR = 0x01000000UL;
    Stack->PC = ( ( uint32_t ) pxCode ) & ( ( uint32_t ) 0xfffffffeUL );
    Stack->LR = ( uint32_t ) pvParameters;
    Stack->r0 = ( uint32_t ) self;
    (*self)->self_stack = Stack;

    return pxTopOfStack;
}

void xTaskCreate( TaskFunction_t pxTaskCode,
                  const uint16_t usStackDepth,
                  void * const pvParameters,//it can be used to debug
                  uint32_t uxPriority,
                  TaskHandle_t * const self )
{
    uint32_t *topStack =NULL;
    TCB_t *NewTcb = (TCB_t *)heap_malloc(sizeof(TCB_t *));
    *self = ( TCB_t *) NewTcb;
    TcbTaskTable[uxPriority] = NewTcb;
    NewTcb->uxPriority = uxPriority;
    NewTcb->pxStack = ( uint32_t *) heap_malloc( ( ( ( size_t ) usStackDepth ) * sizeof( uint32_t * ) ) );
    topStack =  NewTcb->pxStack + (usStackDepth - (uint32_t)1) ;
    topStack = ( uint32_t *) (((uint32_t)topStack) & (~((uint32_t) aligment_byte)));
    NewTcb->pxTopOfStack = pxPortInitialiseStack(topStack,pxTaskCode,pvParameters,self);

    pxCurrentTCB = NewTcb;
    ReadyBitTable |= (1 << uxPriority);
}

__attribute__( ( always_inline ) ) static inline uint8_t FindHighestPriority( void )
{
    uint8_t TopZeroNumber;
    __asm volatile
            (
            "clz %0, %1\n"
            "mov r3, #31\n"
            "sub %0, r3, %0\n"
            :"=r" (TopZeroNumber)
            :"r" (ReadyBitTable)
            :"r3"
            );
    return TopZeroNumber;
}

void vTaskSwitchContext( void )
{
    pxCurrentTCB = TcbTaskTable[ FindHighestPriority()];
}

void EnterSleepMode(void)
{
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    __WFI();
}


//Task handle can be hide, but in order to debug, it must be created manually by the user
TaskHandle_t leisureTcb = NULL;

void leisureTask( )
{//leisureTask content can be manually modified as needed
    while (1) {
        EnterSleepMode();
    }
}

void SchedulerInit( void )
{
    TicksTableInit();
    xTaskCreate(    leisureTask,
                    128,
                    NULL,
                    0,
                    &leisureTcb
    );
}


void __attribute__( ( always_inline ) ) SchedulerStart( void )
{
    /* Start the timer that generates the tick ISR.  Interrupts are disabled
     * here already. */
    ( *( ( volatile uint32_t * ) 0xe000ed20 ) ) |= ( ( ( uint32_t ) 255UL ) << 16UL );
    ( *( ( volatile uint32_t * ) 0xe000ed20 ) ) |= ( ( ( uint32_t ) 255UL ) << 24UL );

    /* Stop and clear the SysTick. */
    SysTick->CTRL = 0UL;
    SysTick->VAL = 0UL;
    /* Configure SysTick to interrupt at the requested rate. */
    SysTick->LOAD = ( configSysTickClockHz / configTickRateHz ) - 1UL;
    SysTick->CTRL = ( ( 1UL << 2UL ) | ( 1UL << 1UL ) | ( 1UL << 0UL ) );

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
            " nop					\n"
            " .ltorg				\n"
            );
}


//Task Area!The user must create task handle manually because of debugging and specification
TaskHandle_t tcbTask1 = NULL;
TaskHandle_t tcbTask2 = NULL;

void led_bright( )
{
    while (1) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        TaskDelay(1000);
    }
}

void led_extinguish( )
{
    while (1) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        TaskDelay(500);
    }
}

void APP( )
{

    xTaskCreate(    led_bright,
                    128,
                    NULL,
                    2,
                    &tcbTask1
    );

    xTaskCreate(    led_extinguish,
                    128,
                    NULL,
                    3,
                    &tcbTask2
    );
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  SchedulerInit();
  APP();
  SchedulerStart();

  while (1)
  {

  }

}


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
