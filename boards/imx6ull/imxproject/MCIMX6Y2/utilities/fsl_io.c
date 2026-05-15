/*
 * Copyright 2017 NXP
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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "fsl_io.h"
#include "fsl_debug_console_conf.h"
#ifdef FSL_RTOS_FREE_RTOS

#include "sem.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* check device when compile */
#if ((!defined(FSL_FEATURE_SOC_UART_COUNT) || (FSL_FEATURE_SOC_UART_COUNT == 0)) && \
     (DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_UART))
#error "Device not support, Please check the fsl_debug_console_conf.h to select a correct device."
#endif

#if ((!defined(FSL_FEATURE_SOC_IUART_COUNT) || (FSL_FEATURE_SOC_IUART_COUNT == 0)) && \
     (DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_IUART))
#error "Device not support, Please check the fsl_debug_console_conf.h to select a correct device."
#endif

#if ((!defined(FSL_FEATURE_SOC_LPUART_COUNT) || (FSL_FEATURE_SOC_LPUART_COUNT == 0)) && \
     (DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_LPUART))
#error "Device not support, Please check the fsl_debug_console_conf.h to select a correct device."
#endif

#if ((!defined(FSL_FEATURE_SOC_LPSCI_COUNT) || (FSL_FEATURE_SOC_LPSCI_COUNT == 0)) && \
     (DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_LPSCI))
#error "Device not support, Please check the fsl_debug_console_conf.h to select a correct device."
#endif

#if ((!defined(FSL_FEATURE_SOC_USB_COUNT) || (FSL_FEATURE_SOC_USB_COUNT == 0)) && defined(BOARD_USE_VIRTUALCOM) && \
     (DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_USBCDC))
#error "Device not support, Please check the fsl_debug_console_conf.h to select a correct device."
#endif

#if ((!defined(FSL_FEATURE_SOC_FLEXCOMM_COUNT) || (FSL_FEATURE_SOC_FLEXCOMM_COUNT == 0)) && \
     (DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_FLEXCOMM))
#error "Device not support, Please check the fsl_debug_console_conf.h to select a correct device."
#endif

#if ((!defined(FSL_FEATURE_SOC_VFIFO_COUNT) || (FSL_FEATURE_SOC_VFIFO_COUNT == 0)) && \
     (DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_VUSART))
#error "Device not support, Please check the fsl_debug_console_conf.h to select a correct device."
#endif

/* configuration for debug console device */
/* If new device is required as the low level device for debug console,
 * Add the #elif branch and add the preprocessor macro to judge whether
 * this kind of device exist in this SOC. */
#if ((DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_UART) || \
     (DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_IUART))

#include "fsl_uart.h"

#define DEVICE_TYPE UART_Type
#define DEVICE_CONFIG uart_config_t
#define DEVICE_INIT(base, config, clk) UART_Init(base, config, clk)
#define DEVICE_Deinit(base) UART_Deinit((DEVICE_TYPE *)base)
#define DEVICE_GET_DEFAULT_CONFIG(config) UART_GetDefaultConfig(config)
#define DEVICE_ENABLE_RX(base, flag) UART_EnableRx(base, flag)
#define DEVICE_ENABLE_TX(base, flag) UART_EnableTx(base, flag)

#if DEBUG_CONSOLE_TRANSFER_POLLING

#define DEVICE_TX(base, buffer, len) UART_WriteBlocking(base, buffer, len)
#define DEVICE_RX(base, buffer, len) UART_ReadBlocking(base, buffer, len)

#else

#define DEVICE_TX(base, handle, xfer) UART_TransferSendNonBlocking(base, handle, xfer)
#define DEVICE_RX(base, handle, xfer, receivedBytes) UART_TransferReceiveNonBlocking(base, handle, xfer, receivedBytes)

#define DEVICE_HANDLE uart_handle_t
#define DEVICE_CALLBACK uart_transfer_callback_t
#define DEVICE_CREATE_HANDLER UART_TransferCreateHandle
#define DEVICE_TRANSFER uart_transfer_t

#define DEVICE_TX_IDLE kStatus_UART_TxIdle
#define DEVICE_RX_IDLE kStatus_UART_RxIdle

#endif

#define DEVICE_TX_COMPLETE(base) UART_GetStatusFlag(base, kUART_TxCompleteFlag)

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_LPUART)

#include "fsl_lpuart.h"

#define DEVICE_TYPE LPUART_Type
#define DEVICE_CONFIG lpuart_config_t
#define DEVICE_INIT(base, config, clk) LPUART_Init(base, config, clk)
#define DEVICE_Deinit(base) LPUART_Deinit((DEVICE_TYPE *)base)
#define DEVICE_GET_DEFAULT_CONFIG(config) LPUART_GetDefaultConfig(config)
#define DEVICE_ENABLE_RX(base, flag) LPUART_EnableRx(base, flag)
#define DEVICE_ENABLE_TX(base, flag) LPUART_EnableTx(base, flag)

#if DEBUG_CONSOLE_TRANSFER_POLLING

#define DEVICE_TX(base, buffer, len) LPUART_WriteBlocking(base, buffer, len)
#define DEVICE_RX(base, buffer, len, receivedBytes) LPUART_ReadBlocking(base, buffer, len, receivedBytes)

#else

#define DEVICE_TX(base, handle, xfer) LPUART_TransferSendNonBlocking(base, handle, xfer)
#define DEVICE_RX(base, handle, xfer) LPUART_TransferReceiveNonBlocking(base, handle, xfer)

#define DEVICE_HANDLE lpuart_handle_t
#define DEVICE_CALLBACK lpuart_transfer_callback_t
#define DEVICE_CREATE_HANDLER LPUART_TransferCreateHandle
#define DEVICE_TRANSFER lpuart_transfer_t

#define DEVICE_TX_IDLE kStatus_LPUART_TxIdle
#define DEVICE_RX_IDLE kStatus_LPUART_RxIdle

#endif

#define DEVICE_TX_COMPLETE(base) (LPUART_GetStatusFlags(base) & kLPUART_TransmissionCompleteFlag)

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_LPSCI)
#include "fsl_lpsci.h"

#define DEVICE_TYPE UART0_Type
#define DEVICE_CONFIG lpsci_config_t
#define DEVICE_INIT(base, config, clk) LPSCI_Init(base, config, clk)
#define DEVICE_Deinit(base) LPSCI_Deinit((DEVICE_TYPE *)base)
#define DEVICE_GET_DEFAULT_CONFIG(config) LPSCI_GetDefaultConfig(config)
#define DEVICE_ENABLE_RX(base, flag) LPSCI_EnableRx(base, flag)
#define DEVICE_ENABLE_TX(base, flag) LPSCI_EnableTx(base, flag)

#if DEBUG_CONSOLE_TRANSFER_POLLING
#define DEVICE_TX(base, buffer, len) LPSCI_WriteBlocking(base, buffer, len)
#define DEVICE_RX(base, buffer, len) LPSCI_ReadBlocking(base, buffer, len)

#else

#define DEVICE_TX(base, handle, xfer) LPSCI_TransferSendNonBlocking(base, handle, xfer)
#define DEVICE_RX(base, handle, xfer, receivedBytes) LPSCI_TransferReceiveNonBlocking(base, handle, xfer, receivedBytes)

#define DEVICE_HANDLE lpsci_handle_t
#define DEVICE_CALLBACK lpsci_transfer_callback_t
#define DEVICE_CREATE_HANDLER LPSCI_TransferCreateHandle
#define DEVICE_TRANSFER lpsci_transfer_t

#define DEVICE_TX_IDLE kStatus_LPSCI_TxIdle
#define DEVICE_RX_IDLE kStatus_LPSCI_RxIdle

#endif

#define DEVICE_TX_COMPLETE(base) (LPSCI_GetStatusFlags(base) & kLPSCI_TransmissionCompleteFlag)

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_USBCDC)
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device_cdc_acm.h"
#include "usb_device_ch9.h"
#include "virtual_com.h"

#define DEVICE_TYPE void
#define DEVICE_CONFIG
#define DEVICE_INIT(base, config, clk) USB_VcomInit()
#define DEVICE_Deinit(base) USB_VcomDeinit((DEVICE_TYPE *)base)
#define DEVICE_GET_DEFAULT_CONFIG(config)
#define DEVICE_ENABLE_RX(base, flag)
#define DEVICE_ENABLE_TX(base, flag)

#if DEBUG_CONSOLE_TRANSFER_POLLING
#define DEVICE_TX(base, buffer, len) USB_VcomWriteBlocking(base, buffer, len)
#define DEVICE_RX(base, buffer, len) USB_VcomReadBlocking(base, buffer, len)

#else

#endif

#define DEVICE_TX_COMPLETE(base) true

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_FLEXCOMM)
#include "fsl_usart.h"

#define DEVICE_TYPE USART_Type
#define DEVICE_CONFIG usart_config_t
#define DEVICE_INIT(base, config, clk) USART_Init(base, config, clk)
#define DEVICE_Deinit(base) USART_Deinit((DEVICE_TYPE *)base)
#define DEVICE_GET_DEFAULT_CONFIG(config) USART_GetDefaultConfig(config)
#define DEVICE_ENABLE_RX(base, flag)
#define DEVICE_ENABLE_TX(base, flag)

#if DEBUG_CONSOLE_TRANSFER_POLLING

#define DEVICE_TX(base, buffer, len) USART_WriteBlocking(base, buffer, len)
#define DEVICE_RX(base, buffer, len) USART_ReadBlocking(base, buffer, len)

#else

#define DEVICE_TX(base, handle, xfer) USART_TransferSendNonBlocking(base, handle, xfer)
#define DEVICE_RX(base, handle, xfer, receivedBytes) USART_TransferReceiveNonBlocking(base, handle, xfer, receivedBytes)

#define DEVICE_HANDLE usart_handle_t
#define DEVICE_CALLBACK usart_transfer_callback_t
#define DEVICE_CREATE_HANDLER USART_TransferCreateHandle
#define DEVICE_TRANSFER usart_transfer_t

#define DEVICE_TX_IDLE kStatus_USART_TxIdle
#define DEVICE_RX_IDLE kStatus_USART_RxIdle

#endif

#define DEVICE_TX_COMPLETE(base) (USART_GetStatusFlags(base) & kUSART_TxFifoEmptyFlag)

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_VUSART)
#include "fsl_usart.h"

#define DEVICE_TYPE USART_Type
#define DEVICE_CONFIG usart_config_t
#define DEVICE_INIT(base, config, clk) USART_Init(base, config, clk)
#define DEVICE_Deinit(base) USART_Deinit((DEVICE_TYPE *)base)
#define DEVICE_GET_DEFAULT_CONFIG(config) USART_GetDefaultConfig(config)
#define DEVICE_ENABLE_RX(base, flag)
#define DEVICE_ENABLE_TX(base, flag)

#if DEBUG_CONSOLE_TRANSFER_POLLING

#define DEVICE_TX(base, buffer, len) USART_WriteBlocking(base, buffer, len)
#define DEVICE_RX(base, buffer, len) USART_ReadBlocking(base, buffer, len)

#else

#define DEVICE_TX(base, handle, xfer) USART_TransferSendNonBlocking(base, handle, xfer)
#define DEVICE_RX(base, handle, xfer, receivedBytes) USART_TransferReceiveNonBlocking(base, handle, xfer, receivedBytes)

#define DEVICE_HANDLE usart_handle_t
#define DEVICE_CALLBACK usart_transfer_callback_t
#define DEVICE_CREATE_HANDLER USART_TransferCreateHandle
#define DEVICE_TRANSFER usart_transfer_t

#define DEVICE_TX_IDLE kStatus_USART_TxIdle
#define DEVICE_RX_IDLE kStatus_USART_RxIdle

#endif

#define DEVICE_TX_COMPLETE(base) (USART_GetStatusFlags(base) & kUSART_TxFifoEmptyFlag)

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_RTT)

#else
/* error no device select */
#error "no device"
#endif

#if DEBUG_CONSOLE_TRANSFER_INTERRUPT

#ifdef FSL_RTOS_FREE_RTOS
/*! @brief IO lock create */
#define IO_TRANSMIT_LOCK_CREATE()                   \
    {                                               \
        IO_TX_xSemaphore = semaphore_creat(1); \
    \
}

/*! @brief IO lock take with non blocking from ISR */
#define IO_TRANSMIT_LOCK_TAKE_FROM_ISR() semaphore_take(IO_TX_xSemaphore, 0)
/*! @brief IO lock take with non blocking */
#define IO_TRANSMIT_LOCK_TAKE() semaphore_take(IO_TX_xSemaphore, 0U)
/*! @brief transmit lock release from ISR */
#define IO_TRANSMIT_LOCK_RELEASE_FROM_ISR()            \
    {                                                  \
        semaphore_release(IO_TX_xSemaphore); \
    }
/*! @brief transmit lock release  */
#define IO_TRANSMIT_LOCK_RELEASE()        \
    {                                     \
        semaphore_release(IO_TX_xSemaphore); \
    }

/*! @brief IO lock create */
#define IO_RECEIVE_LOCK_CREATE()                     \
    {                                                \
        IO_RX_xSemaphore = semaphore_creat(0); \
    \
}
/*! @brief IO lock take blocking */
#define IO_RECEIVE_LOCK_TAKE_BLOCKING() semaphore_take(IO_RX_xSemaphore, 100)
/*! @brief IO lock release form ISR */
#define IO_RECEIVE_LOCK_RELEASE_FROM_ISR()             \
    {                                                  \
        semaphore_release(IO_RX_xSemaphore); \
    \
}

#else
/* define for bare metal software */
#define IO_BARE_METAL
/*! @brief IO lock create */
#define IO_TRANSMIT_LOCK_CREATE()
/*! @brief IO lock take with non blocking */
#define IO_TRANSMIT_LOCK_TAKE_NON_BLOCKING() (true)
/*! @brief State structure storing io. */
#define IO_TRANSMIT_LOCK_RELEASE()

/*! @brief IO lock create */
#define IO_RECEIVE_LOCK_CREATE()
/*! @brief IO lock take blocking */
#define IO_RECEIVE_LOCK_TAKE_BLOCKING()      \
    {                                        \
        while (!(s_debugConsoleIO.rxStatus)) \
            ;                                \
        s_debugConsoleIO.rxStatus = false;   \
    \
}
/*! @brief IO lock release form ISR */
#define IO_RECEIVE_LOCK_RELEASE_FROM_ISR() (s_debugConsoleIO.rxStatus = true)
#endif

#else
/*! @brief IO lock create */
#define IO_TRANSMIT_LOCK_CREATE()
/*! @brief IO lock take with non blocking */
#define IO_TRANSMIT_LOCK_TAKE_NON_BLOCKING() (true)
/*! @brief State structure storing io. */
#define IO_TRANSMIT_LOCK_RELEASE()

/*! @brief IO lock create */
#define IO_RECEIVE_LOCK_CREATE()
/*! @brief IO lock take blocking */
#define IO_RECEIVE_LOCK_TAKE_BLOCKING()
/*! @brief IO lock release form ISR */
#define IO_RECEIVE_LOCK_RELEASE_FROM_ISR()
#endif

/*! @brief State structure storing io. */
typedef struct io_State
{
    DEVICE_TYPE *base; /*!< Base of the IP register. */
#if DEBUG_CONSOLE_TRANSFER_INTERRUPT
    void *callBack;               /*!< define the callback function for buffer */
    DEVICE_HANDLE transferHandle; /*!< interrupt transfer handler */
#ifdef IO_BARE_METAL
    volatile bool rxStatus; /*for bare metal software scanf event */
#endif
#endif

} io_state_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Debug console IO state information. */
static io_state_t s_debugConsoleIO = {
    .base = NULL,
#if DEBUG_CONSOLE_TRANSFER_INTERRUPT
    .callBack = NULL,
#ifdef IO_BARE_METAL
    .rxStatus = false, /*for bare metal software scanf event */
#endif
#endif
};

#ifdef FSL_RTOS_FREE_RTOS
Semaphore_Handle IO_TX_xSemaphore;
Semaphore_Handle IO_RX_xSemaphore;
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/

#if DEBUG_CONSOLE_TRANSFER_INTERRUPT
static void IO_Callback(DEVICE_TYPE *base, DEVICE_HANDLE *handle, status_t status, void *userData)
{
    uint8_t *logAddr = NULL;
    bool tx = false, rx = false;
    size_t size = 0U;
    DEVICE_TRANSFER transfer = {0U};

    if (status == DEVICE_RX_IDLE)
    {
        rx = true;
        size = handle->txDataSizeAll;
        /* notify task */
        IO_RECEIVE_LOCK_RELEASE_FROM_ISR();
    }

    if (status == DEVICE_TX_IDLE)
    {
        tx = true;
        size = handle->txDataSizeAll;
    }

    /* inform the buffer layer that transfer is complete */
    if (s_debugConsoleIO.callBack != NULL)
    {
        /* call buffer callback function */
        logAddr = ((notify)(s_debugConsoleIO.callBack))(&size, rx, tx);
        if ((logAddr != NULL) && (status == DEVICE_TX_IDLE))
        {
            transfer.data = logAddr;
            transfer.dataSize = size;
            /* continue flush the buffer if data is avaliable */
            DEVICE_TX(s_debugConsoleIO.base, &s_debugConsoleIO.transferHandle, &transfer);
        }
    }
}

static bool IO_TransmitLockTake(void)
{
#ifdef __CA7_REV
    if (SystemGetIRQNestingLevel())
#else
    if (__get_IPSR())
#endif
    {
        return IO_TRANSMIT_LOCK_TAKE_FROM_ISR();
    }
    else
    {
        return IO_TRANSMIT_LOCK_TAKE();
    }
}

static void IO_TransmitLockRelease(void)
{
#ifdef __CA7_REV
    if (SystemGetIRQNestingLevel())
#else
    if (__get_IPSR())
#endif
    {
        IO_TRANSMIT_LOCK_RELEASE_FROM_ISR();
    }
    else
    {
        IO_TRANSMIT_LOCK_RELEASE();
    }
}

#endif

void IO_Init(uint32_t baseAddr, uint32_t baudRate, uint32_t clkSrcFreq, void *callBack)
{
    DEVICE_CONFIG config;
    DEVICE_TYPE *base = (DEVICE_TYPE *)(baseAddr);

    DEVICE_GET_DEFAULT_CONFIG(&config);
    config.baudRate_Bps = baudRate;
    /* Enable clock and initial UART module follow user configure structure. */
    DEVICE_INIT(base, &config, clkSrcFreq);
    DEVICE_ENABLE_TX(base, true);
    DEVICE_ENABLE_RX(base, true);

    s_debugConsoleIO.base = base;

#if DEBUG_CONSOLE_TRANSFER_INTERRUPT
    s_debugConsoleIO.callBack = callBack;
    /* create IO lock */
    IO_TRANSMIT_LOCK_CREATE();
    /* create handler for interrupt transfer */
    DEVICE_CREATE_HANDLER(base, &s_debugConsoleIO.transferHandle, IO_Callback, NULL);
#endif
    /* create recieve lock */
    IO_RECEIVE_LOCK_CREATE();
}

void IO_Deinit(void)
{
    /* device deinit function */
    DEVICE_Deinit(s_debugConsoleIO.base);
}

status_t IO_WaitIdle(void)
{
    /* wait transfer complete flag */
    while (!(DEVICE_TX_COMPLETE(s_debugConsoleIO.base)))
        ;

    return kStatus_Success;
}

#if DEBUG_CONSOLE_TRANSFER_INTERRUPT

status_t IO_Transmit(uint8_t *ch, size_t size)
{
    DEVICE_TRANSFER transfer = {0U};
    status_t status = kStatus_Success;
    /* try take IO lock，if fail return directly, do not blocking,
    * IRQ will handle the log print.
    */
    if (IO_TransmitLockTake())
    {
        transfer.data = ch;
        transfer.dataSize = size;
        /* transfer data */
        status = DEVICE_TX(s_debugConsoleIO.base, &s_debugConsoleIO.transferHandle, &transfer);
        /* IO lock release */
        IO_TransmitLockRelease();
    }

    return status;
}

status_t IO_Receive(uint8_t *ch, size_t size)
{
    DEVICE_TRANSFER transfer = {0U};
    size_t receiveBytes = 0U;
    status_t status = kStatus_Success;

    transfer.data = ch;
    transfer.dataSize = size;

    status = DEVICE_RX(s_debugConsoleIO.base, &s_debugConsoleIO.transferHandle, &transfer, &receiveBytes);
    /* wait Semaphore */
    IO_RECEIVE_LOCK_TAKE_BLOCKING();

    return status;
}

#else

status_t IO_Transmit(uint8_t *ch, size_t size)
{
    DEVICE_TX(s_debugConsoleIO.base, ch, size);

    return kStatus_Success;
}

status_t IO_Receive(uint8_t *ch, size_t size)
{
    DEVICE_RX(s_debugConsoleIO.base, ch, size);

    return kStatus_Success;
}

#endif /* DEBUG_CONSOLE_TRANSFER_INTERRUPT */
