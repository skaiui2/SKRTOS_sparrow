/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*System includes.*/
#include <stdio.h>

#include "schedule.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "board.h"
#include "fsl_debug_console.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_common.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEMO_UART_BASEADDR UART1_BASE
#define DEMO_UART_CLK_FREQ BOARD_DebugConsoleSrcFreq()
#define DEMO_IRQn UART1_IRQn
#define DEMO_DEBUG_CONSOLE_DEVICE_TYPE DEBUG_CONSOLE_DEVICE_TYPE_IUART
#define DEMO_DEBUG_UART_BAUDRATE 115200U



/*******************************************************************************
* Variables
******************************************************************************/
TaskHandle_t printTask1Handle = NULL;
TaskHandle_t printTask2Handle = NULL;
TaskHandle_t printTask3Handle = NULL;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/* Application API */
static void printTask1(void *pvParameters);
static void printTask2(void *pvParameters);
static void printTask3(void *pvParameters);
/*******************************************************************************
 * Code
 ******************************************************************************/

/*Provides own implementation of vApplicationIRQHandler() to call SystemIrqHandler()
 *directly without saving the FPU registers on interrupt entry.
 */
void vApplicationIRQHandler(uint32_t ulICCIAR)
{
    SystemIrqHandler(ulICCIAR);
}





void APP()
{
    TaskCreate(printTask1, 128, NULL,500, 1,10, &printTask1Handle);
    TaskCreate(printTask2, 128, NULL,1000, 10,10, &printTask2Handle);
    //TaskCreate(printTask3, 256, 1500,NULL,3,20, &printTask3Handle);
}


int main(void)
{
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitMemory();
    BOARD_InitDebugConsole();


    /* Install IRQ Handler */
    SystemInitIrqTable();
    /* Set interrupt priority */
    GIC_SetPriority(DEMO_IRQn, 25);


    SchedulerInit();
    APP();
    SchedulerStart();

    for (;;)
        ;
}

/*******************************************************************************
 * Application functions
 ******************************************************************************/
/*!
 * @brief print task1 function
 */
static void printTask1(void *pvParameters)
{
    while (1)
    {
        TaskEnter(); 
        PRINTF("\r\n%s\r\n", "one console Demo-print task1.");
        
        TaskExit();
    }
}

/*!
 * @brief print task2 function
 */
static void printTask2(void *pvParameters)
{
    while (1)
    {
        TaskEnter();
        PRINTF("\r\nDebug console Demo-print task2.\r\n");
        TaskExit();
    }
}

/*!
 * @brief print task3 function
 */
static void printTask3(void *pvParameters)
{
    while (1)
    {
        TaskEnter();
        PRINTF("\r\nDebug console Demo-print task3.\r\n");
        TaskExit();
    }
}
