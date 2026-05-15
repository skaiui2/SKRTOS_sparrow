/*
 * Copyright (c) 2017, Freescale Semiconductor, Inc.
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
#ifndef _FSL_UART_SDMA_H_
#define _FSL_UART_SDMA_H_

#include "fsl_uart.h"
#include "fsl_sdma.h"

/*!
 * @addtogroup uart_sdma_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Forward declaration of the handle typedef. */
typedef struct _uart_sdma_handle uart_sdma_handle_t;

/*! @brief UART transfer callback function. */
typedef void (*uart_sdma_transfer_callback_t)(UART_Type *base,
                                              uart_sdma_handle_t *handle,
                                              status_t status,
                                              void *userData);

/*!
* @brief UART sDMA handle
*/
struct _uart_sdma_handle
{
    uart_sdma_transfer_callback_t callback; /*!< Callback function. */
    void *userData;                         /*!< UART callback function parameter.*/
    size_t rxDataSizeAll;                   /*!< Size of the data to receive. */
    size_t txDataSizeAll;                   /*!< Size of the data to send out. */
    sdma_handle_t *txSdmaHandle;            /*!< The sDMA TX channel used. */
    sdma_handle_t *rxSdmaHandle;            /*!< The sDMA RX channel used. */
    volatile uint8_t txState;               /*!< TX transfer state. */
    volatile uint8_t rxState;               /*!< RX transfer state */
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name sDMA transactional
 * @{
 */

/*!
 * @brief Initializes the UART handle which is used in transactional functions.
 * @param base UART peripheral base address.
 * @param handle Pointer to the uart_sdma_handle_t structure.
 * @param callback UART callback, NULL means no callback.
 * @param userData User callback function data.
 * @param rxSdmaHandle User-requested DMA handle for RX DMA transfer.
 * @param txSdmaHandle User-requested DMA handle for TX DMA transfer.
 * @param eventSourceTx Eventsource for TX DMA transfer.
 * @param eventSourceRx Eventsource for RX DMA transfer.
 */
void UART_TransferCreateHandleSDMA(UART_Type *base,
                                   uart_sdma_handle_t *handle,
                                   uart_sdma_transfer_callback_t callback,
                                   void *userData,
                                   sdma_handle_t *txSdmaHandle,
                                   sdma_handle_t *rxSdmaHandle,
                                   uint32_t eventSourceTx,
                                   uint32_t eventSourceRx);

/*!
 * @brief Sends data using sDMA.
 *
 * This function sends data using sDMA. This is a non-blocking function, which returns
 * right away. When all data is sent, the send callback function is called.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param xfer UART sDMA transfer structure. See #uart_transfer_t.
 * @retval kStatus_Success if succeeded; otherwise failed.
 * @retval kStatus_UART_TxBusy Previous transfer ongoing.
 * @retval kStatus_InvalidArgument Invalid argument.
 */
status_t UART_SendSDMA(UART_Type *base, uart_sdma_handle_t *handle, uart_transfer_t *xfer);

/*!
 * @brief Receives data using sDMA.
 *
 * This function receives data using sDMA. This is a non-blocking function, which returns
 * right away. When all data is received, the receive callback function is called.
 *
 * @param base UART peripheral base address.
 * @param handle Pointer to the uart_sdma_handle_t structure.
 * @param xfer UART sDMA transfer structure. See #uart_transfer_t.
 * @retval kStatus_Success if succeeded; otherwise failed.
 * @retval kStatus_UART_RxBusy Previous transfer ongoing.
 * @retval kStatus_InvalidArgument Invalid argument.
 */
status_t UART_ReceiveSDMA(UART_Type *base, uart_sdma_handle_t *handle, uart_transfer_t *xfer);

/*!
 * @brief Aborts the sent data using sDMA.
 *
 * This function aborts sent data using sDMA.
 *
 * @param base UART peripheral base address.
 * @param handle Pointer to the uart_sdma_handle_t structure.
 */
void UART_TransferAbortSendSDMA(UART_Type *base, uart_sdma_handle_t *handle);

/*!
 * @brief Aborts the receive data using sDMA.
 *
 * This function aborts receive data using sDMA.
 *
 * @param base UART peripheral base address.
 * @param handle Pointer to the uart_sdma_handle_t structure.
 */
void UART_TransferAbortReceiveSDMA(UART_Type *base, uart_sdma_handle_t *handle);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_UART_SDMA_H_ */
