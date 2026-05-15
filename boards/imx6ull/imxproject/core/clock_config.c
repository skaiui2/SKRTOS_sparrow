/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 * All rights reserved.
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

#include "fsl_common.h"
#include "clock_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

const clock_arm_pll_config_t g_ccmConfigArmPll_1056M = {.loopDivider = 88U};
const clock_arm_pll_config_t g_ccmConfigArmPll_792M = {.loopDivider = 66U};
const clock_arm_pll_config_t g_ccmConfigArmPll_900M = {.loopDivider = 75U};

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* System clock frequency. */
extern uint32_t SystemCoreClock;

/*******************************************************************************
 * Code
 ******************************************************************************/
static void BOARD_BootClockGate(void)
{
    /* Disable all unused peripheral clock */
    CCM->CCGR0 = 0x00C0000FU;
    CCM->CCGR1 = 0xF00C0000U;
    CCM->CCGR2 = 0x003F0030U;
    CCM->CCGR3 = 0xFF3CC300U;
    CCM->CCGR4 = 0x0000F3FCU;
    CCM->CCGR5 = 0x00330033U;
    CCM->CCGR6 = 0x00CC0300U;
}

void BOARD_BootClockRUN(void)
{
    /* Boot ROM did initialize the XTAL, here we only sets external XTAL OSC freq */
    CLOCK_SetXtalFreq(24000000U);
    CLOCK_SetRtcXtalFreq(32768U);

    /* Switch CPU off ARM PLL */
    if (CLOCK_GetMux(kCLOCK_Pll1SwMux) == 0) /* CPU runs on ARM PLL */
    {
        CLOCK_SetMux(kCLOCK_StepMux, 0);   /* Set Step MUX to OSC */
        CLOCK_SetMux(kCLOCK_Pll1SwMux, 1); /* Let CPU run on Step MUX */
    }
#if (defined(CPU_MCIMX6Y2CVM05) || defined(CPU_MCIMX6Y2DVM05) || defined(CPU_MCIMX6Y0CVM05) || \
     defined(CPU_MCIMX6Y0DVM05) || defined(CPU_MCIMX6Y1CVM05) || defined(CPU_MCIMX6Y1CVK05) || \
     defined(CPU_MCIMX6Y1DVM05) || defined(CPU_MCIMX6Y1DVK05) || defined(CPU_MCIMX6Y7DVK05))
    CLOCK_InitArmPll(&g_ccmConfigArmPll_1056M); /* Configure ARM PLL to 1056M */
    CLOCK_SetMux(kCLOCK_Pll1SwMux, 0);          /* Now CPU runs again on ARM PLL*/
    CLOCK_SetDiv(kCLOCK_ArmDiv, 1);             /* Configure ARM clock root with divide 2 */
#elif defined(CPU_MCIMX6Y2CVM08)
    CLOCK_InitArmPll(&g_ccmConfigArmPll_792M); /* Configure ARM PLL to 792M */
    CLOCK_SetMux(kCLOCK_Pll1SwMux, 0);         /* Now CPU runs again on ARM PLL*/
    CLOCK_SetDiv(kCLOCK_ArmDiv, 0);            /* Configure ARM clock root with divide 1 */
#elif(defined(CPU_MCIMX6Y2DVM09) || defined(CPU_MCIMX6Y7DVM09))
    CLOCK_InitArmPll(&g_ccmConfigArmPll_900M); /* Configure ARM PLL to 900M */
    CLOCK_SetMux(kCLOCK_Pll1SwMux, 0);         /* Now CPU runs again on ARM PLL*/
    CLOCK_SetDiv(kCLOCK_ArmDiv, 0);            /* Configure ARM clock root with divide 1 */
#endif

/*jump to ocram to run configure clock api */
#if defined(__ICCARM__)
#pragma section = "RSTACK"
    asm volatile(
        "mov     r0,sp             \n"
        "mov     sp,%0             \n"
        "push    {r0-r3}           \n"
        "bl      BOARD_SetRunClock \n"
        "pop     {r0-r3}           \n"
        "mov     sp,r0             \n"
        :
        : "r"((uint32_t)__section_end("RSTACK")), "r"(BOARD_SetRunClock)
        : "r0");
#elif defined(__GNUC__)
    extern uint32_t __RStackTop[];
    asm volatile(
        "mov     r0,sp             \n"
        "mov     sp,%0             \n"
        "push    {r0-r3}           \n"
        "bl      BOARD_SetRunClock \n"
        "pop     {r0-r3}           \n"
        "mov     sp,r0             \n"
        :
        : "r"((uint32_t)__RStackTop), "r"(BOARD_SetRunClock)
        : "r0");
#else
#error Not supported compiler type
#endif

    /* Disable unused clock */
    BOARD_BootClockGate();

    /* Power down all unused PLL */
    CLOCK_DeinitUsb2Pll();
    CLOCK_DeinitAudioPll();
    CLOCK_DeinitVideoPll();
    CLOCK_DeinitEnetPll();

    /* Configure UART divider to default */
    CLOCK_SetMux(kCLOCK_UartMux, 0); /* Set UART source to PLL3 80M */
    CLOCK_SetDiv(kCLOCK_UartDiv, 0); /* Set UART divider to 1 */

    CLOCK_SetMux(kCLOCK_PerclkMux, 0); /* Set I2C/EPIT source to IPG 66M */
    CLOCK_SetDiv(kCLOCK_PerclkDiv, 0); /* Set I2C/EPIT divider to 1 */

    /* Update core clock */
    SystemCoreClockUpdate();
}
