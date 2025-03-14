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

#include "fsl_sai_sdma.h"

/*******************************************************************************
 * Definitations
 ******************************************************************************/
/*<! Structure definition for uart_sdma_private_handle_t. The structure is private. */
typedef struct _sai_sdma_private_handle
{
    I2S_Type *base;
    sai_sdma_handle_t *handle;
} sai_sdma_private_handle_t;

enum _sai_sdma_transfer_state
{
    kSAI_Busy = 0x0U, /*!< SAI is busy */
    kSAI_Idle,        /*!< Transfer is done. */
};

/*<! Private handle only used for internally. */
static sai_sdma_private_handle_t s_sdmaPrivateHandle[FSL_FEATURE_SOC_I2S_COUNT][2];

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get the instance number for SAI.
 *
 * @param base SAI base pointer.
 */
extern uint32_t SAI_GetInstance(I2S_Type *base);

/*!
 * @brief SAI SDMA callback for send.
 *
 * @param handle pointer to sai_sdma_handle_t structure which stores the transfer state.
 * @param userData Parameter for user callback.
 * @param done If the DMA transfer finished.
 * @param tcds The TCD index.
 */
static void SAI_TxSDMACallback(sdma_handle_t *handle, void *userData, bool transferDone, uint32_t bdIndex);

/*!
 * @brief SAI SDMA callback for receive.
 *
 * @param handle pointer to sai_sdma_handle_t structure which stores the transfer state.
 * @param userData Parameter for user callback.
 * @param done If the DMA transfer finished.
 * @param tcds The TCD index.
 */
static void SAI_RxSDMACallback(sdma_handle_t *handle, void *userData, bool transferDone, uint32_t bdIndex);

/*******************************************************************************
* Code
******************************************************************************/
static void SAI_TxSDMACallback(sdma_handle_t *handle, void *userData, bool transferDone, uint32_t bdIndex)
{
    sai_sdma_private_handle_t *privHandle = (sai_sdma_private_handle_t *)userData;
    sai_sdma_handle_t *saiHandle = privHandle->handle;

    /* If finished a blcok, call the callback function */
    memset(&saiHandle->saiQueue[saiHandle->queueDriver], 0, sizeof(sai_transfer_t));
    saiHandle->queueDriver = (saiHandle->queueDriver + 1) % SAI_XFER_QUEUE_SIZE;
    /* Stop SDMA transfer */
    SDMA_StopChannel(handle->base, handle->channel);
    if (saiHandle->callback)
    {
        (saiHandle->callback)(privHandle->base, saiHandle, kStatus_SAI_TxIdle, saiHandle->userData);
    }

    /* If all data finished, just stop the transfer */
    if (saiHandle->saiQueue[saiHandle->queueDriver].data == NULL)
    {
        SAI_TransferAbortSendSDMA(privHandle->base, saiHandle);
    }
}

static void SAI_RxSDMACallback(sdma_handle_t *handle, void *userData, bool transferDone, uint32_t bdIndex)
{
    sai_sdma_private_handle_t *privHandle = (sai_sdma_private_handle_t *)userData;
    sai_sdma_handle_t *saiHandle = privHandle->handle;

    /* If finished a blcok, call the callback function */
    memset(&saiHandle->saiQueue[saiHandle->queueDriver], 0, sizeof(sai_transfer_t));
    saiHandle->queueDriver = (saiHandle->queueDriver + 1) % SAI_XFER_QUEUE_SIZE;
    if (saiHandle->callback)
    {
        (saiHandle->callback)(privHandle->base, saiHandle, kStatus_SAI_RxIdle, saiHandle->userData);
    }

    /* If all data finished, just stop the transfer */
    if (saiHandle->saiQueue[saiHandle->queueDriver].data == NULL)
    {
        SAI_TransferAbortReceiveSDMA(privHandle->base, saiHandle);
    }
}

void SAI_TransferTxCreateHandleSDMA(I2S_Type *base,
                                    sai_sdma_handle_t *handle,
                                    sai_sdma_callback_t callback,
                                    void *userData,
                                    sdma_handle_t *dmaHandle,
                                    uint32_t eventSource)
{
    assert(handle && dmaHandle);

    uint32_t instance = SAI_GetInstance(base);

    /* Zero the handle */
    memset(handle, 0, sizeof(*handle));

    /* Set sai base to handle */
    handle->dmaHandle = dmaHandle;
    handle->callback = callback;
    handle->userData = userData;
    handle->eventSource = eventSource;

    /* Set SAI state to idle */
    handle->state = kSAI_Idle;

    s_sdmaPrivateHandle[instance][0].base = base;
    s_sdmaPrivateHandle[instance][0].handle = handle;

    SDMA_InstallBDMemory(dmaHandle, handle->bdPool, SAI_XFER_QUEUE_SIZE);

    /* Install callback for Tx dma channel */
    SDMA_SetCallback(dmaHandle, SAI_TxSDMACallback, &s_sdmaPrivateHandle[instance][0]);
}

void SAI_TransferRxCreateHandleSDMA(I2S_Type *base,
                                    sai_sdma_handle_t *handle,
                                    sai_sdma_callback_t callback,
                                    void *userData,
                                    sdma_handle_t *dmaHandle,
                                    uint32_t eventSource)
{
    assert(handle && dmaHandle);

    uint32_t instance = SAI_GetInstance(base);

    /* Zero the handle */
    memset(handle, 0, sizeof(*handle));

    /* Set sai base to handle */
    handle->dmaHandle = dmaHandle;
    handle->callback = callback;
    handle->userData = userData;
    handle->eventSource = eventSource;

    /* Set SAI state to idle */
    handle->state = kSAI_Idle;

    s_sdmaPrivateHandle[instance][1].base = base;
    s_sdmaPrivateHandle[instance][1].handle = handle;

    SDMA_InstallBDMemory(dmaHandle, handle->bdPool, SAI_XFER_QUEUE_SIZE);

    /* Install callback for Tx dma channel */
    SDMA_SetCallback(dmaHandle, SAI_RxSDMACallback, &s_sdmaPrivateHandle[instance][1]);
}

void SAI_TransferTxSetFormatSDMA(I2S_Type *base,
                                 sai_sdma_handle_t *handle,
                                 sai_transfer_format_t *format,
                                 uint32_t mclkSourceClockHz,
                                 uint32_t bclkSourceClockHz)
{
    assert(handle && format);

    /* Configure the audio format to SAI registers */
    SAI_TxSetFormat(base, format, mclkSourceClockHz, bclkSourceClockHz);

    /* Get the tranfer size from format, this should be used in SDMA configuration */
    if (format->bitWidth == 24U)
    {
        handle->bytesPerFrame = 4U;
    }
    else
    {
        handle->bytesPerFrame = format->bitWidth / 8U;
    }

    /* Update the data channel SAI used */
    handle->channel = format->channel;
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    handle->count = FSL_FEATURE_SAI_FIFO_COUNT - format->watermark;
#else
    handle->count = 1U;
#endif /* FSL_FEATURE_SAI_FIFO_COUNT */

    /* Clear the channel enable bits unitl do a send/receive */
    base->TCR3 &= ~I2S_TCR3_TCE_MASK;
}

void SAI_TransferRxSetFormatSDMA(I2S_Type *base,
                                 sai_sdma_handle_t *handle,
                                 sai_transfer_format_t *format,
                                 uint32_t mclkSourceClockHz,
                                 uint32_t bclkSourceClockHz)
{
    assert(handle && format);

    /* Configure the audio format to SAI registers */
    SAI_RxSetFormat(base, format, mclkSourceClockHz, bclkSourceClockHz);

    /* Get the tranfer size from format, this should be used in SDMA configuration */
    if (format->bitWidth == 24U)
    {
        handle->bytesPerFrame = 4U;
    }
    else
    {
        handle->bytesPerFrame = format->bitWidth / 8U;
    }

    /* Update the data channel SAI used */
    handle->channel = format->channel;

#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    handle->count = format->watermark;
#else
    handle->count = 1U;
#endif /* FSL_FEATURE_SAI_FIFO_COUNT */

    /* Clear the channel enable bits unitl do a send/receive */
    base->RCR3 &= ~I2S_RCR3_RCE_MASK;
}

status_t SAI_TransferSendSDMA(I2S_Type *base, sai_sdma_handle_t *handle, sai_transfer_t *xfer)
{
    assert(handle && xfer);

    sdma_transfer_config_t config = {0};
    uint32_t destAddr = SAI_TxGetDataRegisterAddress(base, handle->channel);
    sdma_handle_t *dmaHandle = handle->dmaHandle;
    sdma_peripheral_t perType = kSDMA_PeripheralNormal;

    /* Check if input parameter invalid */
    if ((xfer->data == NULL) || (xfer->dataSize == 0U))
    {
        return kStatus_InvalidArgument;
    }

    if (handle->saiQueue[handle->queueUser].data)
    {
        return kStatus_SAI_QueueFull;
    }

    /* Change the state of handle */
    handle->transferSize[handle->queueUser] = xfer->dataSize;
    handle->saiQueue[handle->queueUser].data = xfer->data;
    handle->saiQueue[handle->queueUser].dataSize = xfer->dataSize;

#if defined(FSL_FEATURE_SOC_SPBA_COUNT) && (FSL_FEATURE_SOC_SPBA_COUNT > 0)
    uint32_t address = (uint32_t)base;
    /* Judge if the instance is located in SPBA */
    if ((address >= FSL_FEATURE_SPBA_START) && (address < FSL_FEATURE_SPBA_END))
    {
        perType = kSDMA_PeripheralNormal_SP;
    }
#endif /* FSL_FEATURE_SOC_SPBA_COUNT */

    /* Prepare sdma configure */
    SDMA_PrepareTransfer(&config, (uint32_t)xfer->data, destAddr, handle->bytesPerFrame, handle->bytesPerFrame,
                         handle->count * handle->bytesPerFrame, xfer->dataSize, handle->eventSource, perType,
                         kSDMA_MemoryToPeripheral);

    if (handle->queueUser == SAI_XFER_QUEUE_SIZE - 1U)
    {
        SDMA_ConfigBufferDescriptor(&dmaHandle->BDPool[handle->queueUser], (uint32_t)(xfer->data), destAddr,
                                    config.destTransferSize, xfer->dataSize, true, true, true,
                                    kSDMA_MemoryToPeripheral);
    }
    else
    {
        SDMA_ConfigBufferDescriptor(&dmaHandle->BDPool[handle->queueUser], (uint32_t)(xfer->data), destAddr,
                                    config.destTransferSize, xfer->dataSize, true, true, false,
                                    kSDMA_MemoryToPeripheral);
    }

    handle->queueUser = (handle->queueUser + 1) % SAI_XFER_QUEUE_SIZE;

    if (handle->state != kSAI_Busy)
    {
        SDMA_SubmitTransfer(handle->dmaHandle, &config);

        /* Start DMA transfer */
        SDMA_StartTransfer(handle->dmaHandle);

        /* Enable DMA enable bit */
        SAI_TxEnableDMA(base, kSAI_FIFORequestDMAEnable, true);

        /* Enable SAI Tx clock */
        SAI_TxEnable(base, true);

        /* Enable the channel FIFO */
        base->TCR3 |= I2S_TCR3_TCE(1U << handle->channel);
    }
    handle->state = kSAI_Busy;

    return kStatus_Success;
}

status_t SAI_TransferReceiveSDMA(I2S_Type *base, sai_sdma_handle_t *handle, sai_transfer_t *xfer)
{
    assert(handle && xfer);

    sdma_transfer_config_t config = {0};
    sdma_handle_t *dmaHandle = handle->dmaHandle;
    uint32_t srcAddr = SAI_RxGetDataRegisterAddress(base, handle->channel);
    sdma_peripheral_t perType = kSDMA_PeripheralNormal;

    /* Check if input parameter invalid */
    if ((xfer->data == NULL) || (xfer->dataSize == 0U))
    {
        return kStatus_InvalidArgument;
    }

    if (handle->saiQueue[handle->queueUser].data)
    {
        return kStatus_SAI_QueueFull;
    }

    /* Update queue state  */
    handle->transferSize[handle->queueUser] = xfer->dataSize;
    handle->saiQueue[handle->queueUser].data = xfer->data;
    handle->saiQueue[handle->queueUser].dataSize = xfer->dataSize;

#if defined(FSL_FEATURE_SOC_SPBA_COUNT) && (FSL_FEATURE_SOC_SPBA_COUNT > 0)
    uint32_t address = (uint32_t)base;
    /* Judge if the instance is located in SPBA */
    if ((address >= FSL_FEATURE_SPBA_START) && (address < FSL_FEATURE_SPBA_END))
    {
        perType = kSDMA_PeripheralNormal_SP;
    }
#endif /* FSL_FEATURE_SOC_SPBA_COUNT */

    /* Prepare sdma configure */
    SDMA_PrepareTransfer(&config, srcAddr, (uint32_t)xfer->data, handle->bytesPerFrame, handle->bytesPerFrame,
                         handle->count * handle->bytesPerFrame, xfer->dataSize, handle->eventSource, perType,
                         kSDMA_PeripheralToMemory);

    if (handle->queueUser == SAI_XFER_QUEUE_SIZE - 1U)
    {
        SDMA_ConfigBufferDescriptor(&dmaHandle->BDPool[handle->queueUser], srcAddr, (uint32_t)xfer->data,
                                    config.destTransferSize, xfer->dataSize, true, true, true,
                                    kSDMA_PeripheralToMemory);
    }
    else
    {
        SDMA_ConfigBufferDescriptor(&dmaHandle->BDPool[handle->queueUser], srcAddr, (uint32_t)xfer->data,
                                    config.destTransferSize, xfer->dataSize, true, true, false,
                                    kSDMA_PeripheralToMemory);
    }

    handle->queueUser = (handle->queueUser + 1) % SAI_XFER_QUEUE_SIZE;

    if (handle->state != kSAI_Busy)
    {
        SDMA_SubmitTransfer(handle->dmaHandle, &config);

        /* Start DMA transfer */
        SDMA_StartTransfer(handle->dmaHandle);

        /* Enable DMA enable bit */
        SAI_RxEnableDMA(base, kSAI_FIFORequestDMAEnable, true);

        /* Enable SAI Rx clock */
        SAI_RxEnable(base, true);

        /* Enable the channel FIFO */
        base->RCR3 |= I2S_RCR3_RCE(1U << handle->channel);
    }

    handle->state = kSAI_Busy;

    return kStatus_Success;
}

void SAI_TransferAbortSendSDMA(I2S_Type *base, sai_sdma_handle_t *handle)
{
    assert(handle);

    /* Disable dma */
    SDMA_AbortTransfer(handle->dmaHandle);

    /* Disable the channel FIFO */
    base->TCR3 &= ~I2S_TCR3_TCE_MASK;

    /* Disable DMA enable bit */
    SAI_TxEnableDMA(base, kSAI_FIFORequestDMAEnable, false);

    /* Reset the FIFO pointer, at the same time clear all error flags if set */
    base->TCSR |= (I2S_TCSR_FR_MASK | I2S_TCSR_SR_MASK);
    base->TCSR &= ~I2S_TCSR_SR_MASK;

    /* Disable Tx */
    SAI_TxEnable(base, false);

    /* Set the handle state */
    handle->state = kSAI_Idle;
}

void SAI_TransferAbortReceiveSDMA(I2S_Type *base, sai_sdma_handle_t *handle)
{
    assert(handle);

    /* Disable dma */
    SDMA_AbortTransfer(handle->dmaHandle);

    /* Disable the channel FIFO */
    base->RCR3 &= ~I2S_RCR3_RCE_MASK;

    /* Disable DMA enable bit */
    SAI_RxEnableDMA(base, kSAI_FIFORequestDMAEnable, false);

    /* Disable Rx */
    SAI_RxEnable(base, false);

    /* Reset the FIFO pointer, at the same time clear all error flags if set */
    base->RCSR |= (I2S_RCSR_FR_MASK | I2S_RCSR_SR_MASK);
    base->RCSR &= ~I2S_RCSR_SR_MASK;

    /* Set the handle state */
    handle->state = kSAI_Idle;
}
