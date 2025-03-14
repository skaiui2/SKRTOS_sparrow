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
#include "fsl_debug_console.h"
#include "board.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
static const mmu_attribute_t s_mmuDevAttr = {.type = MMU_MemoryDevice,
                                             .domain = 0U,
                                             .accessPerm = MMU_AccessRWNA,
                                             .shareable = 0U,
                                             .notSecure = 0U,
                                             .notGlob = 0U,
                                             .notExec = 1U};

static const mmu_attribute_t s_mmuRomAttr = {.type = MMU_MemoryWriteBackWriteAllocate,
                                             .domain = 0U,
                                             .accessPerm = MMU_AccessRORO,
                                             .shareable = 0U,
                                             .notSecure = 0U,
                                             .notGlob = 0U,
                                             .notExec = 0U};

static const mmu_attribute_t s_mmuRamAttr = {.type = MMU_MemoryWriteBackWriteAllocate,
                                             .domain = 0U,
                                             .accessPerm = MMU_AccessRWRW,
                                             .shareable = 0U,
                                             .notSecure = 0U,
                                             .notGlob = 0U,
                                             .notExec = 0U};

static const mmu_attribute_t s_mmuBufferAttr = {.type = MMU_MemoryNonCacheable,
                                                .domain = 0U,
                                                .accessPerm = MMU_AccessRWRW,
                                                .shareable = 0U,
                                                .notSecure = 0U,
                                                .notGlob = 0U,
                                                .notExec = 0U};

#if defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment = 16384
static uint32_t MMU_L1Table[4096] @"OcramData";
#elif defined(__GNUC__)
static uint32_t MMU_L1Table[4096] __attribute__((section("OcramData"), aligned(16384)));
#else
#error Not supported compiler type
#endif

/*Initial output value of NXP74LV595*/
static uint8_t s_NXP74LV595Output = 0U;

/*******************************************************************************
 * Code
 ******************************************************************************/
/* Initialize memory system (MMU). */
void BOARD_InitMemory(void)
{
    uint32_t i;

    MMU_Init(MMU_L1Table);

    MMU_ConfigSection(MMU_L1Table, (const void *)0x00000000U, 0x00000000U, &s_mmuRomAttr); /* ROM */
    MMU_ConfigSection(MMU_L1Table, (const void *)0x00900000U, 0x00900000U, &s_mmuRamAttr); /* OCRAM */
    MMU_ConfigSection(MMU_L1Table, (const void *)0x00A00000U, 0x00A00000U, &s_mmuDevAttr); /* GIC */
    MMU_ConfigSection(MMU_L1Table, (const void *)0x00B00000U, 0x00B00000U, &s_mmuDevAttr); /* GPV_0 PL301 */
    MMU_ConfigSection(MMU_L1Table, (const void *)0x00C00000U, 0x00C00000U, &s_mmuDevAttr); /* GPV_1 PL301 */
    MMU_ConfigSection(MMU_L1Table, (const void *)0x00D00000U, 0x00D00000U, &s_mmuDevAttr); /* cpu */
    MMU_ConfigSection(MMU_L1Table, (const void *)0x00E00000U, 0x00E00000U, &s_mmuDevAttr); /* per_m */
    MMU_ConfigSection(MMU_L1Table, (const void *)0x01800000U, 0x01800000U, &s_mmuDevAttr); /* APBH DMA */
    MMU_ConfigSection(MMU_L1Table, (const void *)0x02000000U, 0x02000000U, &s_mmuDevAttr); /* AIPS-1 */
    MMU_ConfigSection(MMU_L1Table, (const void *)0x02100000U, 0x02100000U, &s_mmuDevAttr); /* AIPS-2 */
    MMU_ConfigSection(MMU_L1Table, (const void *)0x02200000U, 0x02200000U, &s_mmuDevAttr); /* AIPS-3 */

    for (i = 0; i < 32; i++)
    {
        MMU_ConfigSection(MMU_L1Table, (const void *)(0x0C000000U + (i << 20)), (0x0C000000U + (i << 20)),
                          &s_mmuDevAttr); /* QSPI Rx Buf */
    }

    for (i = 0; i < 256; i++)
    {
        MMU_ConfigSection(MMU_L1Table, (const void *)(0x50000000U + (i << 20)), (0x50000000U + (i << 20)),
                          &s_mmuRamAttr); /* EIM */
    }

    for (i = 0; i < 256; i++)
    {
        MMU_ConfigSection(MMU_L1Table, (const void *)(0x60000000U + (i << 20)), (0x60000000U + (i << 20)),
                          &s_mmuRomAttr); /* QSPI */
    }

    for (i = 0; i < 2048; i++)
    {
        MMU_ConfigSection(MMU_L1Table, (const void *)(0x80000000U + (i << 20)), (0x80000000U + (i << 20)),
                          &s_mmuRamAttr); /* DDR */
    }

/* You can place global or static variables in NonCacheable section to make it uncacheable.*/
#if defined(__ICCARM__)
#pragma section = "NonCacheable"
    uint32_t ncahceStart = (uint32_t)__section_begin("NonCacheable");
    uint32_t size = (uint32_t)__section_size("NonCacheable");
#elif defined(__GNUC__)
    extern uint32_t __noncachedata_start__[];
    extern uint32_t __noncachedata_end__[];
    uint32_t ncahceStart = (uint32_t)__noncachedata_start__;
    uint32_t size = (uint32_t)__noncachedata_end__ - (uint32_t)__noncachedata_start__;
#else
#error Not supported compiler type
#endif
    size = (size + 0xFFFFFU) & (~0xFFFFFU);

    for (i = 0; i < ((size) >> 20); i++)
    {
        MMU_ConfigSection(MMU_L1Table, (const void *)(ncahceStart + (i << 20)), (ncahceStart + (i << 20)),
                          &s_mmuBufferAttr); /* Frame buffer */
    }

    MMU_Enable();
}

/* Get debug console frequency. */
uint32_t BOARD_DebugConsoleSrcFreq(void)
{
    uint32_t freq;

    /* To make it simple, we assume default PLL and divider settings, and the only variable
       from application is use PLL3 source or OSC source */
    if (CLOCK_GetMux(kCLOCK_UartMux) == 0) /* PLL3 div6 80M */
    {
        freq = (CLOCK_GetPllFreq(kCLOCK_PllUsb1) / 6U) / (CLOCK_GetDiv(kCLOCK_UartDiv) + 1U);
    }
    else
    {
        freq = CLOCK_GetOscFreq() / (CLOCK_GetDiv(kCLOCK_UartDiv) + 1U);
    }

    return freq;
}

/* Initialize debug console. */
void BOARD_InitDebugConsole(void)
{
    uint32_t uartClkSrcFreq;

    uartClkSrcFreq = BOARD_DebugConsoleSrcFreq();

#ifdef FSL_RTOS_FREE_RTOS
    GIC_SetPriority(BOARD_UART_IRQ, 25);
#endif

    DbgConsole_Init(BOARD_DEBUG_UART_BASEADDR, BOARD_DEBUG_UART_BAUDRATE, BOARD_DEBUG_UART_TYPE, uartClkSrcFreq);
}

void Delay(uint32_t ticks)
{
    while (ticks--)
    {
        __NOP();
    }
}

/* Set parallel pins output value of 74LV595 */
void BOARD_NXP74LV595_SetValue(NXP74LV595_parOutputPins_t pin, NXP74LV595_signal_t value)
{
    uint8_t shiftNumber;
    uint8_t mask;

    /*Init GPIO pins and open output gate of 74LV595*/
    NXP74LV595_INPUT_OE_GPIO->DR &= ~(1U << NXP74LV595_INPUT_OE_PIN);
    NXP74LV595_INPUT_OE_GPIO->GDIR |= (1U << NXP74LV595_INPUT_OE_PIN);
    NXP74LV595_INPUT_STCP_GPIO->GDIR |= (1U << NXP74LV595_INPUT_STCP_PIN);
    NXP74LV595_INPUT_SDI_GPIO->GDIR |= (1U << NXP74LV595_INPUT_SDI_PIN);
    NXP74LV595_INPUT_SHCP_GPIO->GDIR |= (1U << NXP74LV595_INPUT_SHCP_PIN);

    /*Calculate all parallel pins output value that will be set*/
    s_NXP74LV595Output = (s_NXP74LV595Output & (~(1U << pin))) | (value << pin);

    for (shiftNumber = 0; shiftNumber < 8; shiftNumber++)
    {
        /*High data bits transfer first*/
        mask = (s_NXP74LV595Output >> (7 - shiftNumber)) & 1U;
        if (0 == mask)
        {
            NXP74LV595_INPUT_SDI_GPIO->DR &= ~(1U << NXP74LV595_INPUT_SDI_PIN);
        }
        else
        {
            NXP74LV595_INPUT_SDI_GPIO->DR |= (1U << NXP74LV595_INPUT_SDI_PIN);
        }
        /*Contents of shift register shifted.*/
        NXP74LV595_INPUT_SHCP_GPIO->DR &= ~(1U << NXP74LV595_INPUT_SHCP_PIN);
        Delay(1000);
        NXP74LV595_INPUT_SHCP_GPIO->DR |= (1U << NXP74LV595_INPUT_SHCP_PIN);
        Delay(1000);
    }
    /*Contents of shift register stages are transferred to the storage register and parallel output stages*/
    NXP74LV595_INPUT_STCP_GPIO->DR &= ~(1U << NXP74LV595_INPUT_STCP_PIN);
    Delay(1000);
    NXP74LV595_INPUT_STCP_GPIO->DR |= (1U << NXP74LV595_INPUT_STCP_PIN);
    Delay(1000);
}
