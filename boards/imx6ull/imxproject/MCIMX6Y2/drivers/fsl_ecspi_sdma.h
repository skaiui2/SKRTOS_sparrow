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
#ifndef _FSL_SPI_DMA_H_
#define _FSL_SPI_DMA_H_

#include "fsl_ecspi.h"
#include "fsl_sdma.h"

/*!
 * @addtogroup ecspi_sdma_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

typedef struct _ecspi_sdma_handle ecspi_sdma_handle_t;

/*! @brief ECSPI SDMA callback called at the end of transfer. */
typedef void (*ecspi_sdma_callback_t)(ECSPI_Type *base, ecspi_sdma_handle_t *handle, status_t status, void *userData);

/*! @brief ECSPI SDMA transfer handle, users should not touch the content of the handle.*/
struct _ecspi_sdma_handle
{
    bool txInProgress;              /*!< Send transfer finished */
    bool rxInProgress;              /*!< Receive transfer finished */
    sdma_handle_t *txSdmaHandle;    /*!< DMA handler for ECSPI send */
    sdma_handle_t *rxSdmaHandle;    /*!< DMA handler for ECSPI receive */
    ecspi_sdma_callback_t callback; /*!< Callback for ECSPI SDMA transfer */
    void *userData;                 /*!< User Data for ECSPI SDMA callback */
    uint32_t state;                 /*!< Internal state of ECSPI SDMA transfer */
    uint32_t ChannelTx;             /*!< Channel for send handle */
    uint32_t ChannelRx;             /*!< Channel for receive handler */
};

/*******************************************************************************
 * APIs
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name DMA Transactional
 * @{
 */

/*!
 * @brief Initialize the ECSPI master SDMA handle.
 *
 * This function initializes the ECSPI master SDMA handle which can be used for other SPI master transactional APIs.
 * Usually, for a specified ECSPI instance, user need only call this API once to get the initialized handle.
 *
 * @param base ECSPI peripheral base address.
 * @param handle ECSPI handle pointer.
 * @param callback User callback function called at the end of a transfer.
 * @param userData User data for callback.
 * @param txHandle SDMA handle pointer for ECSPI Tx, the handle shall be static allocated by users.
 * @param rxHandle SDMA handle pointer for ECSPI Rx, the handle shall be static allocated by users.
 * @param eventSourceTx event source for ECSPI send, which can be found in SDMA mapping.
 * @param eventSourceRx event source for ECSPI receive, which can be found in SDMA mapping.
 * @param TxChannel SDMA channel for ECSPI send.
 * @param RxChannel SDMA channel for ECSPI receive.
 */
void ECSPI_MasterTransferCreateHandleSDMA(ECSPI_Type *base,
                                          ecspi_sdma_handle_t *handle,
                                          ecspi_sdma_callback_t callback,
                                          void *userData,
                                          sdma_handle_t *txHandle,
                                          sdma_handle_t *rxHandle,
                                          uint32_t eventSourceTx,
                                          uint32_t eventSourceRx,
                                          uint32_t TxChannel,
                                          uint32_t RxChannel);

/*!
 * @brief Initialize the ECSPI Slave SDMA handle.
 *
 * This function initializes the ECSPI Slave SDMA handle which can be used for other SPI Slave transactional APIs.
 * Usually, for a specified ECSPI instance, user need only call this API once to get the initialized handle.
 *
 * @param base ECSPI peripheral base address.
 * @param handle ECSPI handle pointer.
 * @param callback User callback function called at the end of a transfer.
 * @param userData User data for callback.
 * @param txHandle SDMA handle pointer for ECSPI Tx, the handle shall be static allocated by users.
 * @param rxHandle SDMA handle pointer for ECSPI Rx, the handle shall be static allocated by users.
 * @param eventSourceTx event source for ECSPI send, which can be found in SDMA mapping.
 * @param eventSourceRx event source for ECSPI receive, which can be found in SDMA mapping.
 * @param TxChannel SDMA channel for ECSPI send.
 * @param RxChannel SDMA channel for ECSPI receive.
 */
void ECSPI_SlaveTransferCreateHandleSDMA(ECSPI_Type *base,
                                         ecspi_sdma_handle_t *handle,
                                         ecspi_sdma_callback_t callback,
                                         void *userData,
                                         sdma_handle_t *txHandle,
                                         sdma_handle_t *rxHandle,
                                         uint32_t eventSourceTx,
                                         uint32_t eventSourceRx,
                                         uint32_t TxChannel,
                                         uint32_t RxChannel);

/*!
 * @brief Perform a non-blocking ECSPI master transfer using SDMA.
 *
 * @note This interface returned immediately after transfer initiates.
 *
 * @param base ECSPI peripheral base address.
 * @param handle ECSPI SDMA handle pointer.
 * @param xfer Pointer to sdma transfer structure.
 * @retval kStatus_Success Successfully start a transfer.
 * @retval kStatus_InvalidArgument Input argument is invalid.
 * @retval kStatus_ECSPI_Busy EECSPI is not idle, is running another transfer.
 */
status_t ECSPI_MasterTransferSDMA(ECSPI_Type *base, ecspi_sdma_handle_t *handle, ecspi_transfer_t *xfer);

/*!
 * @brief Perform a non-blocking ECSPI slave transfer using SDMA.
 *
 * @note This interface returned immediately after transfer initiates.
 *
 * @param base ECSPI peripheral base address.
 * @param handle ECSPI SDMA handle pointer.
 * @param xfer Pointer to sdma transfer structure.
 * @retval kStatus_Success Successfully start a transfer.
 * @retval kStatus_InvalidArgument Input argument is invalid.
 * @retval kStatus_ECSPI_Busy EECSPI is not idle, is running another transfer.
 */
status_t ECSPI_SlaveTransferSDMA(ECSPI_Type *base, ecspi_sdma_handle_t *handle, ecspi_transfer_t *xfer);
/*!
 * @brief Abort a ECSPI master transfer using SDMA.
 *
 * @param base ECSPI peripheral base address.
 * @param handle ECSPI SDMA handle pointer.
 */
void ECSPI_MasterTransferAbortSDMA(ECSPI_Type *base, ecspi_sdma_handle_t *handle);
/*!
 * @brief Abort a ECSPI slave transfer using SDMA.
 *
 * @param base ECSPI peripheral base address.
 * @param handle ECSPI SDMA handle pointer.
 */
void ECSPI_SlaveTransferAbortSDMA(ECSPI_Type *base, ecspi_sdma_handle_t *handle);

/*! @} */

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */
#endif
