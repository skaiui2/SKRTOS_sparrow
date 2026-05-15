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
#ifndef _FSL_SAI_SDMA_H_
#define _FSL_SAI_SDMA_H_

#include "fsl_sai.h"
#include "fsl_sdma.h"

/*!
 * @addtogroup sai_sdma
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

typedef struct _sai_sdma_handle sai_sdma_handle_t;

/*! @brief SAI SDMA transfer callback function for finish and error */
typedef void (*sai_sdma_callback_t)(I2S_Type *base, sai_sdma_handle_t *handle, status_t status, void *userData);

/*! @brief SAI DMA transfer handle, users should not touch the content of the handle. */
struct _sai_sdma_handle
{
    sdma_handle_t *dmaHandle;     /*!< DMA handler for SAI send */
    uint8_t bytesPerFrame;        /*!< Bytes in a frame */
    uint8_t channel;              /*!< Which data channel */
    uint8_t count;                /*!< The transfer data count in a DMA request */
    uint32_t state;               /*!< Internal state for SAI SDMA transfer */
    uint32_t eventSource;         /*!< SAI event source number */
    sai_sdma_callback_t callback; /*!< Callback for users while transfer finish or error occurs */
    void *userData;               /*!< User callback parameter */
    sdma_buffer_descriptor_t bdPool[SAI_XFER_QUEUE_SIZE]; /*!< BD pool for SDMA transfer. */
    sai_transfer_t saiQueue[SAI_XFER_QUEUE_SIZE];         /*!< Transfer queue storing queued transfer. */
    size_t transferSize[SAI_XFER_QUEUE_SIZE];             /*!< Data bytes need to transfer */
    volatile uint8_t queueUser;                           /*!< Index for user to queue transfer. */
    volatile uint8_t queueDriver;                         /*!< Index for driver to get the transfer data and size */
};

/*******************************************************************************
 * APIs
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name SDMA Transactional
 * @{
 */

/*!
 * @brief Initializes the SAI SDMA handle.
 *
 * This function initializes the SAI master DMA handle, which can be used for other SAI master transactional APIs.
 * Usually, for a specified SAI instance, call this API once to get the initialized handle.
 *
 * @param base SAI base pointer.
 * @param handle SAI SDMA handle pointer.
 * @param base SAI peripheral base address.
 * @param callback Pointer to user callback function.
 * @param userData User parameter passed to the callback function.
 * @param dmaHandle SDMA handle pointer, this handle shall be static allocated by users.
 */
void SAI_TransferTxCreateHandleSDMA(I2S_Type *base,
                                    sai_sdma_handle_t *handle,
                                    sai_sdma_callback_t callback,
                                    void *userData,
                                    sdma_handle_t *dmaHandle,
                                    uint32_t eventSource);

/*!
 * @brief Initializes the SAI Rx SDMA handle.
 *
 * This function initializes the SAI slave DMA handle, which can be used for other SAI master transactional APIs.
 * Usually, for a specified SAI instance, call this API once to get the initialized handle.
 *
 * @param base SAI base pointer.
 * @param handle SAI SDMA handle pointer.
 * @param base SAI peripheral base address.
 * @param callback Pointer to user callback function.
 * @param userData User parameter passed to the callback function.
 * @param dmaHandle SDMA handle pointer, this handle shall be static allocated by users.
 */
void SAI_TransferRxCreateHandleSDMA(I2S_Type *base,
                                    sai_sdma_handle_t *handle,
                                    sai_sdma_callback_t callback,
                                    void *userData,
                                    sdma_handle_t *dmaHandle,
                                    uint32_t eventSource);

/*!
 * @brief Configures the SAI Tx audio format.
 *
 * The audio format can be changed at run-time. This function configures the sample rate and audio data
 * format to be transferred. This function also sets the SDMA parameter according to formatting requirements.
 *
 * @param base SAI base pointer.
 * @param handle SAI SDMA handle pointer.
 * @param format Pointer to SAI audio data format structure.
 * @param mclkSourceClockHz SAI master clock source frequency in Hz.
 * @param bclkSourceClockHz SAI bit clock source frequency in Hz. If bit clock source is master
 * clock, this value should equals to masterClockHz in format.
 * @retval kStatus_Success Audio format set successfully.
 * @retval kStatus_InvalidArgument The input argument is invalid.
*/
void SAI_TransferTxSetFormatSDMA(I2S_Type *base,
                                 sai_sdma_handle_t *handle,
                                 sai_transfer_format_t *format,
                                 uint32_t mclkSourceClockHz,
                                 uint32_t bclkSourceClockHz);

/*!
 * @brief Configures the SAI Rx audio format.
 *
 * The audio format can be changed at run-time. This function configures the sample rate and audio data
 * format to be transferred. This function also sets the SDMA parameter according to formatting requirements.
 *
 * @param base SAI base pointer.
 * @param handle SAI SDMA handle pointer.
 * @param format Pointer to SAI audio data format structure.
 * @param mclkSourceClockHz SAI master clock source frequency in Hz.
 * @param bclkSourceClockHz SAI bit clock source frequency in Hz. If a bit clock source is the master
 * clock, this value should equal to masterClockHz in format.
 * @retval kStatus_Success Audio format set successfully.
 * @retval kStatus_InvalidArgument The input argument is invalid.
*/
void SAI_TransferRxSetFormatSDMA(I2S_Type *base,
                                 sai_sdma_handle_t *handle,
                                 sai_transfer_format_t *format,
                                 uint32_t mclkSourceClockHz,
                                 uint32_t bclkSourceClockHz);

/*!
 * @brief Performs a non-blocking SAI transfer using DMA.
 *
 * @note This interface returns immediately after the transfer initiates. Call
 * SAI_GetTransferStatus to poll the transfer status and check whether the SAI transfer is finished.
 *
 * @param base SAI base pointer.
 * @param handle SAI SDMA handle pointer.
 * @param xfer Pointer to the DMA transfer structure.
 * @retval kStatus_Success Start a SAI SDMA send successfully.
 * @retval kStatus_InvalidArgument The input argument is invalid.
 * @retval kStatus_TxBusy SAI is busy sending data.
 */
status_t SAI_TransferSendSDMA(I2S_Type *base, sai_sdma_handle_t *handle, sai_transfer_t *xfer);

/*!
 * @brief Performs a non-blocking SAI receive using SDMA.
 *
 * @note This interface returns immediately after the transfer initiates. Call
 * the SAI_GetReceiveRemainingBytes to poll the transfer status and check whether the SAI transfer is finished.
 *
 * @param base SAI base pointer
 * @param handle SAI SDMA handle pointer.
 * @param xfer Pointer to DMA transfer structure.
 * @retval kStatus_Success Start a SAI SDMA receive successfully.
 * @retval kStatus_InvalidArgument The input argument is invalid.
 * @retval kStatus_RxBusy SAI is busy receiving data.
 */
status_t SAI_TransferReceiveSDMA(I2S_Type *base, sai_sdma_handle_t *handle, sai_transfer_t *xfer);

/*!
 * @brief Aborts a SAI transfer using SDMA.
 *
 * @param base SAI base pointer.
 * @param handle SAI SDMA handle pointer.
 */
void SAI_TransferAbortSendSDMA(I2S_Type *base, sai_sdma_handle_t *handle);

/*!
 * @brief Aborts a SAI receive using SDMA.
 *
 * @param base SAI base pointer
 * @param handle SAI SDMA handle pointer.
 */
void SAI_TransferAbortReceiveSDMA(I2S_Type *base, sai_sdma_handle_t *handle);

/*! @} */

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */
#endif
