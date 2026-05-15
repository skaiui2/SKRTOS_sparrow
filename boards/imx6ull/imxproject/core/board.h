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

#ifndef _BOARD_H_
#define _BOARD_H_

#include "clock_config.h"
#include "fsl_common.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief The board name */
#define BOARD_NAME "MCIMX6ULL-EVK"

/* The UART to use for debug messages. */
#define BOARD_DEBUG_UART_TYPE DEBUG_CONSOLE_DEVICE_TYPE_IUART
#define BOARD_DEBUG_UART_BAUDRATE 115200U
#define BOARD_DEBUG_UART_BASEADDR UART1_BASE
#define BOARD_DEBUG_UART_CLK_FREQ BOARD_DebugConsoleSrcFreq()
#define BOARD_UART_IRQ UART1_IRQn

#define BOARD_SAI_DEMO_I2C_BASEADDR I2C2

/*! @brief The Enet instance used for board. */
#define BOARD_ENET_BASEADDR ENET1

/*! @brief The Enet instance IRQ used for board. */
#define EXAMPLE_ENET_IRQ ENET1_IRQn
/*! @brief Get MMDC clock. */
#define BOARD_MMDC_FREQ (CLOCK_GetFreq(kCLOCK_MmdcClk))

/*! @brief The ENET PHY address. */
#define BOARD_ENET0_PHY_ADDRESS (0x02U) /* Phy address of enet port 0. */
#define BOARD_ENET1_PHY_ADDRESS (0x01U) /* Phy address of enet port 1. */

#define NXP74LV595_INPUT_STCP_GPIO GPIO5
#define NXP74LV595_INPUT_STCP_PIN 7U
#define NXP74LV595_INPUT_OE_GPIO GPIO5
#define NXP74LV595_INPUT_OE_PIN 8U
#define NXP74LV595_INPUT_SDI_GPIO GPIO5
#define NXP74LV595_INPUT_SDI_PIN 10U
#define NXP74LV595_INPUT_SHCP_GPIO GPIO5
#define NXP74LV595_INPUT_SHCP_PIN 11U

#define BOARD_USDHC1_BASEADDR USDHC1
#define BOARD_USDHC2_BASEADDR USDHC2
#define BOARD_USDHC_CD_GPIO_BASE GPIO1
#define BOARD_USDHC_CD_GPIO_PIN 19
#define BOARD_USDHC_CD_PORT_IRQ GPIO1_Combined_16_31_IRQn
#define BOARD_USDHC_CD_PORT_IRQ_HANDLER GPIO_IRQ_Handler

#define BOARD_USDHC_CD_STATUS() (GPIO_ReadPinInput(BOARD_USDHC_CD_GPIO_BASE, BOARD_USDHC_CD_GPIO_PIN))

#define BOARD_USDHC_CD_INTERRUPT_STATUS() (GPIO_GetPinsInterruptFlags(BOARD_USDHC_CD_GPIO_BASE))
#define BOARD_USDHC_CD_CLEAR_INTERRUPT(flag) (GPIO_ClearPinsInterruptFlags(BOARD_USDHC_CD_GPIO_BASE, flag))

#define BOARD_USDHC_CD_GPIO_INIT()                                                      \
    {                                                                                   \
        gpio_pin_config_t sw_config = {                                                 \
            kGPIO_DigitalInput, 0, kGPIO_IntFallingEdge,                                \
        };                                                                              \
        GPIO_PinInit(BOARD_USDHC_CD_GPIO_BASE, BOARD_USDHC_CD_GPIO_PIN, &sw_config);    \
        GPIO_EnableInterrupts(BOARD_USDHC_CD_GPIO_BASE, 1U << BOARD_USDHC_CD_GPIO_PIN); \
        GPIO_ClearPinsInterruptFlags(BOARD_USDHC_CD_GPIO_BASE, ~0);                     \
    }

#define BOARD_SD_POWER_RESET_GPIO (GPIO1)
#define BOARD_SD_POWER_RESET_GPIO_PIN (9U)

#define BOARD_USDHC_MMCCARD_POWER_CONTROL(state)

#define BOARD_USDHC_MMCCARD_POWER_CONTROL_INIT()                                             \
    {                                                                                        \
        gpio_pin_config_t sw_config = {                                                      \
            kGPIO_DigitalOutput, 0, kGPIO_NoIntmode,                                         \
        };                                                                                   \
        GPIO_PinInit(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PIN, &sw_config);  \
        GPIO_WritePinOutput(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PIN, true); \
    }

#define BOARD_USDHC_SDCARD_POWER_CONTROL_INIT()                                             \
    {                                                                                       \
        gpio_pin_config_t sw_config = {                                                     \
            kGPIO_DigitalOutput, 0, kGPIO_NoIntmode,                                        \
        };                                                                                  \
        GPIO_PinInit(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PIN, &sw_config); \
    }

#define BOARD_USDHC_SDCARD_POWER_CONTROL(state) \
    (GPIO_WritePinOutput(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PIN, state))

#define BOARD_USDHC1_CLK_FREQ (CLOCK_GetSysPfdFreq(kCLOCK_Pfd2) / (CLOCK_GetDiv(kCLOCK_Usdhc1Div) + 1U))
#define BOARD_USDHC2_CLK_FREQ (CLOCK_GetSysPfdFreq(kCLOCK_Pfd2) / (CLOCK_GetDiv(kCLOCK_Usdhc2Div) + 1U))

#define BOARD_SD_HOST_BASEADDR BOARD_USDHC1_BASEADDR
#define BOARD_SD_HOST_CLK_FREQ BOARD_USDHC1_CLK_FREQ
#define BOARD_SD_HOST_IRQ USDHC1_IRQn
#define BOARD_MMC_HOST_BASEADDR BOARD_USDHC1_BASEADDR
#define BOARD_MMC_HOST_CLK_FREQ BOARD_USDHC1_CLK_FREQ
#define BOARD_MMC_HOST_IRQ USDHC1_IRQn
#define BOARD_MMC_VCCQ_SUPPLY kMMC_VoltageWindow170to195
#define BOARD_MMC_VCC_SUPPLY kMMC_VoltageWindows270to360
/* we are using the BB SD socket to DEMO the MMC example,but the
* SD socket provide 4bit bus only, so we define this macro to avoid
* 8bit data bus test
*/
#define BOARD_MMC_SUPPORT_8BIT_BUS (0U)

#define BOARD_SD_HOST_SUPPORT_SDR104_FREQ (130000000U)
#define BOARD_SD_HOST_SUPPORT_HS200_FREQ (150000000U)
/* define for SD/MMC config IO driver strength dynamic */
#define BOARD_SD_PIN_CONFIG(speed, strength)                                                      \
    {                                                                                             \
        IOMUXC_SetPinConfig(IOMUXC_SD1_CMD_USDHC1_CMD,                                            \
                            IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |   \
                                IOMUXC_SW_PAD_CTL_PAD_DSE(strength));                             \
        IOMUXC_SetPinConfig(IOMUXC_SD1_CLK_USDHC1_CLK,                                            \
                            IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(0) |   \
                                IOMUXC_SW_PAD_CTL_PAD_DSE(strength));                             \
        IOMUXC_SetPinConfig(IOMUXC_SD1_DATA0_USDHC1_DATA0,                                        \
                            IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |   \
                                IOMUXC_SW_PAD_CTL_PAD_DSE(strength));                             \
        IOMUXC_SetPinConfig(IOMUXC_SD1_DATA1_USDHC1_DATA1,                                        \
                            IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |   \
                                IOMUXC_SW_PAD_CTL_PAD_DSE(strength));                             \
        IOMUXC_SetPinConfig(IOMUXC_SD1_DATA2_USDHC1_DATA2,                                        \
                            IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |   \
                                IOMUXC_SW_PAD_CTL_PAD_DSE(strength));                             \
        IOMUXC_SetPinConfig(IOMUXC_SD1_DATA3_USDHC1_DATA3,                                        \
                            IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |   \
                                IOMUXC_SW_PAD_CTL_PAD_DSE(strength));                             \
    }

#define BOARD_MMC_PIN_CONFIG(speed, strength)                                                     \
    {                                                                                             \
        IOMUXC_SetPinConfig(IOMUXC_SD1_CMD_USDHC1_CMD,                                            \
                            IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |   \
                                IOMUXC_SW_PAD_CTL_PAD_DSE(strength));                             \
        IOMUXC_SetPinConfig(IOMUXC_SD1_CLK_USDHC1_CLK,                                            \
                            IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(0) |   \
                                IOMUXC_SW_PAD_CTL_PAD_DSE(strength));                             \
        IOMUXC_SetPinConfig(IOMUXC_SD1_DATA0_USDHC1_DATA0,                                        \
                            IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |   \
                                IOMUXC_SW_PAD_CTL_PAD_DSE(strength));                             \
        IOMUXC_SetPinConfig(IOMUXC_SD1_DATA1_USDHC1_DATA1,                                        \
                            IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |   \
                                IOMUXC_SW_PAD_CTL_PAD_DSE(strength));                             \
        IOMUXC_SetPinConfig(IOMUXC_SD1_DATA2_USDHC1_DATA2,                                        \
                            IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |   \
                                IOMUXC_SW_PAD_CTL_PAD_DSE(strength));                             \
        IOMUXC_SetPinConfig(IOMUXC_SD1_DATA3_USDHC1_DATA3,                                        \
                            IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK | \
                                IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |   \
                                IOMUXC_SW_PAD_CTL_PAD_DSE(strength));                             \
    }

typedef enum _NXP74LV595_signal
{
    kSignal_NXP74LV595_Low = 0U,
    kSignal_NXP74LV595_High = 1U
} NXP74LV595_signal_t;

typedef enum _NXP74LV595_parOutputPins
{
    kNXP74LV595_HDMI_nRST = 0U,
    kNXP74LV595_ENET1_nRST = 1U,
    kNXP74LV595_ENET2_nRST = 2U,
    kNXP74LV595_CAN1_2_STBY = 3U,
    kNXP74LV595_BT_nPWD = 4U,
    kNXP74LV595_CSI_RST = 5U,
    kNXP74LV595_CSI_PWDN = 6U,
    kNXP74LV595_LCD_nPWREN = 7U
} NXP74LV595_parOutputPins_t;

/* USB PHY condfiguration */
#define BOARD_USB_PHY_D_CAL (0x0CU)
#define BOARD_USB_PHY_TXCAL45DP (0x06U)
#define BOARD_USB_PHY_TXCAL45DM (0x06U)

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * API
 ******************************************************************************/
void BOARD_InitMemory(void);

uint32_t BOARD_DebugConsoleSrcFreq(void);

void BOARD_InitDebugConsole(void);

void BOARD_NXP74LV595_SetValue(NXP74LV595_parOutputPins_t pin, NXP74LV595_signal_t value);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _BOARD_H_ */
