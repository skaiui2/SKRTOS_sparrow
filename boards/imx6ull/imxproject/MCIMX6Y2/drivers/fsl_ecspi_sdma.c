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

#include "fsl_ecspi_sdma.h"

/*******************************************************************************
 * Definitons
 ******************************************************************************/

/*!
* @brief Structure definition for ecspi_master_sdma_private_handle_t. The structure is private.
*/
typedef struct _ecspi_master_sdma_private_handle
{
    ECSPI_Type *base;            /*!< ECSPI peripheral base address. */
    ecspi_sdma_handle_t *handle; /*!< ecspi_sdma_handle_t handle */
} ecspi_master_sdma_private_handle_t;

/*!
* @brief Structure definition for ecspi_slave_sdma_private_handle_t. The structure is private.
*/
typedef struct _ecspi_slave_sdma_private_handle
{
    ECSPI_Type *base;            /*!< ECSPI peripheral base address. */
    ecspi_sdma_handle_t *handle; /*!< ecspi_sdma_handle_t handle */
} ecspi_slave_sdma_private_handle_t;

/*! @brief ECSPI transfer state, which is used for ECSPI transactiaonl APIs' internal state. */
enum _ecspi_sdma_states_t
{
    kECSPI_Idle = 0x0, /*!< ECSPI is idle state */
    kECSPI_Busy        /*!< ECSPI is busy tranferring data. */
};
/*! @brief Base pointer array */
static ECSPI_Type *const s_ecspiBases[] = ECSPI_BASE_PTRS;
/*<! Private handle only used for internally. */
static ecspi_master_sdma_private_handle_t s_ecspiMasterSdmaPrivateHandle[ARRAY_SIZE(s_ecspiBases)];
static ecspi_slave_sdma_private_handle_t s_ecspiSlaveSdmaPrivateHandle[ARRAY_SIZE(s_ecspiBases)];
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get the instance for ECSPI module.
 *
 * @param base ECSPI base address
 */
extern uint32_t ECSPI_GetInstance(ECSPI_Type *base);

/*!
* @brief SDMA_EcspiMasterCallback after the ECSPI master transfer completed by using SDMA.
* This is not a public API.
*/
static void SDMA_EcspiMasterCallback(sdma_handle_t *sdmaHandle,
                                     void *g_ecspiSdmaPrivateHandle,
                                     bool transferDone,
                                     uint32_t tcds);

/*!
* @brief SDMA_EcspiSlaveCallback after the ECSPI slave transfer completed by using SDMA.
* This is not a public API.
*/
static void SDMA_EcspiSlaveCallback(sdma_handle_t *sdmaHandle,
                                    void *g_ecspiSdmaPrivateHandle,
                                    bool transferDone,
                                    uint32_t tcds);
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
* Code
******************************************************************************/
static void SDMA_EcspiMasterCallback(sdma_handle_t *sdmaHandle,
                                     void *g_ecspiSdmaPrivateHandle,
                                     bool transferDone,
                                     uint32_t tcds)
{
    assert(sdmaHandle);
    assert(g_ecspiSdmaPrivateHandle);

    ecspi_master_sdma_private_handle_t *ecspiSdmaPrivateHandle;

    ecspiSdmaPrivateHandle = (ecspi_master_sdma_private_handle_t *)g_ecspiSdmaPrivateHandle;
    /* if channel is Tx channel, disable Tx channel SDMA enable*/
    if (sdmaHandle->channel == ecspiSdmaPrivateHandle->handle->ChannelTx)
    {
        ECSPI_EnableDMA((ecspiSdmaPrivateHandle->base), kECSPI_TxDmaEnable, false);
        ecspiSdmaPrivateHandle->handle->txInProgress = false;
        SDMA_SetChannelPriority(sdmaHandle->base, sdmaHandle->channel, 0);
        SDMA_AbortTransfer(sdmaHandle);
    }
    /* if channel is Rx channel, disable Rx channel SDMA enable*/
    else if (sdmaHandle->channel == ecspiSdmaPrivateHandle->handle->ChannelRx)
    {
        ECSPI_EnableDMA((ecspiSdmaPrivateHandle->base), kECSPI_RxDmaEnable, false);
        ecspiSdmaPrivateHandle->handle->rxInProgress = false;
        SDMA_SetChannelPriority(sdmaHandle->base, sdmaHandle->channel, 0);
        SDMA_AbortTransfer(sdmaHandle);
    }
    /* if both channel is finished, then abort SDMA transfer*/
    if ((ecspiSdmaPrivateHandle->handle->txInProgress == false) &&
        (ecspiSdmaPrivateHandle->handle->rxInProgress == false))
    {
        ECSPI_MasterTransferAbortSDMA(ecspiSdmaPrivateHandle->base, ecspiSdmaPrivateHandle->handle);
        if (ecspiSdmaPrivateHandle->handle->callback)
        {
            ecspiSdmaPrivateHandle->handle->callback(ecspiSdmaPrivateHandle->base, ecspiSdmaPrivateHandle->handle,
                                                     kStatus_Success, ecspiSdmaPrivateHandle->handle->userData);
        }
        ecspiSdmaPrivateHandle->handle->state = kECSPI_Idle;
    }
}

void ECSPI_MasterTransferAbortSDMA(ECSPI_Type *base, ecspi_sdma_handle_t *handle)
{
    assert(handle);

    ECSPI_Enable(base, false);

    ECSPI_EnableDMA(base, kECSPI_DmaAllEnable, false);

    SDMA_AbortTransfer(handle->rxSdmaHandle);
    SDMA_AbortTransfer(handle->txSdmaHandle);

    handle->state = kECSPI_Idle;
}

static void SDMA_EcspiSlaveCallback(sdma_handle_t *sdmaHandle,
                                    void *g_ecspiSdmaPrivateHandle,
                                    bool transferDone,
                                    uint32_t tcds)
{
    assert(sdmaHandle);
    assert(g_ecspiSdmaPrivateHandle);

    ecspi_slave_sdma_private_handle_t *ecspiSdmaPrivateHandle;

    ecspiSdmaPrivateHandle = (ecspi_slave_sdma_private_handle_t *)g_ecspiSdmaPrivateHandle;
    /* if channel is Tx channel, disable Tx channel SDMA enable*/
    if (sdmaHandle->channel == ecspiSdmaPrivateHandle->handle->ChannelTx)
    {
        ECSPI_EnableDMA((ecspiSdmaPrivateHandle->base), kECSPI_TxDmaEnable, false);
        ecspiSdmaPrivateHandle->handle->txInProgress = false;
        SDMA_SetChannelPriority(sdmaHandle->base, sdmaHandle->channel, 0);
        SDMA_AbortTransfer(sdmaHandle);
    }
    /* if channel is Rx channel, disable Rx channel SDMA enable*/
    else if (sdmaHandle->channel == ecspiSdmaPrivateHandle->handle->ChannelRx)
    {
        ECSPI_EnableDMA((ecspiSdmaPrivateHandle->base), kECSPI_RxDmaEnable, false);
        ecspiSdmaPrivateHandle->handle->rxInProgress = false;
        SDMA_SetChannelPriority(sdmaHandle->base, sdmaHandle->channel, 0);
        SDMA_AbortTransfer(sdmaHandle);
    }
    /* if both channel is finished, then abort SDMA transfer*/
    if ((ecspiSdmaPrivateHandle->handle->txInProgress == false) &&
        (ecspiSdmaPrivateHandle->handle->rxInProgress == false))
    {
        ECSPI_MasterTransferAbortSDMA(ecspiSdmaPrivateHandle->base, ecspiSdmaPrivateHandle->handle);
        if (ecspiSdmaPrivateHandle->handle->callback)
        {
            ecspiSdmaPrivateHandle->handle->callback(ecspiSdmaPrivateHandle->base, ecspiSdmaPrivateHandle->handle,
                                                     kStatus_Success, ecspiSdmaPrivateHandle->handle->userData);
        }
        ecspiSdmaPrivateHandle->handle->state = kECSPI_Idle;
    }
}

void ECSPI_SlaveTransferAbortSDMA(ECSPI_Type *base, ecspi_sdma_handle_t *handle)
{
    assert(handle);

    ECSPI_Enable(base, false);

    ECSPI_EnableDMA(base, kECSPI_RxDmaEnable, false);
    ECSPI_EnableDMA(base, kECSPI_TxDmaEnable, false);

    SDMA_AbortTransfer(handle->rxSdmaHandle);
    SDMA_AbortTransfer(handle->txSdmaHandle);

    handle->state = kECSPI_Idle;
}

void ECSPI_MasterTransferCreateHandleSDMA(ECSPI_Type *base,
                                          ecspi_sdma_handle_t *handle,
                                          ecspi_sdma_callback_t callback,
                                          void *userData,
                                          sdma_handle_t *txHandle,
                                          sdma_handle_t *rxHandle,
                                          uint32_t eventSourceTx,
                                          uint32_t eventSourceRx,
                                          uint32_t TxChannel,
                                          uint32_t RxChannel)
{
    assert(handle);
    assert(txHandle);
    assert(rxHandle);
    uint32_t instance = ECSPI_GetInstance(base);

    /* Zero the handle */
    memset(handle, 0, sizeof(*handle));

    /* Set ECSPI base to handle */
    rxHandle->eventSource = eventSourceRx;
    txHandle->eventSource = eventSourceTx;
    handle->txSdmaHandle = txHandle;
    handle->rxSdmaHandle = rxHandle;
    handle->ChannelTx = TxChannel;
    handle->ChannelRx = RxChannel;
    handle->callback = callback;
    handle->userData = userData;

    /* Set ECSPI state to idle */
    handle->state = kECSPI_Idle;

    /* Set handle to global state */
    s_ecspiMasterSdmaPrivateHandle[instance].base = base;
    s_ecspiMasterSdmaPrivateHandle[instance].handle = handle;

    /* Install callback for Tx and Rx sdma channel */
    SDMA_SetCallback(handle->rxSdmaHandle, SDMA_EcspiMasterCallback, &s_ecspiMasterSdmaPrivateHandle[instance]);
    SDMA_SetCallback(handle->txSdmaHandle, SDMA_EcspiMasterCallback, &s_ecspiMasterSdmaPrivateHandle[instance]);
}

void ECSPI_SlaveTransferCreateHandleSDMA(ECSPI_Type *base,
                                         ecspi_sdma_handle_t *handle,
                                         ecspi_sdma_callback_t callback,
                                         void *userData,
                                         sdma_handle_t *txHandle,
                                         sdma_handle_t *rxHandle,
                                         uint32_t eventSourceTx,
                                         uint32_t eventSourceRx,
                                         uint32_t TxChannel,
                                         uint32_t RxChannel)
{
    assert(handle);
    assert(txHandle);
    assert(rxHandle);
    uint32_t instance = ECSPI_GetInstance(base);

    /* Zero the handle */
    memset(handle, 0, sizeof(*handle));

    /* Set ECSPI base to handle */
    rxHandle->eventSource = eventSourceRx;
    txHandle->eventSource = eventSourceTx;
    handle->txSdmaHandle = txHandle;
    handle->rxSdmaHandle = rxHandle;
    handle->ChannelTx = TxChannel;
    handle->ChannelRx = RxChannel;
    handle->callback = callback;
    handle->userData = userData;

    /* Set ECSPI state to idle */
    handle->state = kECSPI_Idle;

    /* Set handle to global state */
    s_ecspiSlaveSdmaPrivateHandle[instance].base = base;
    s_ecspiSlaveSdmaPrivateHandle[instance].handle = handle;

    /* Install callback for Tx and Rx sdma channel */
    SDMA_SetCallback(handle->txSdmaHandle, SDMA_EcspiSlaveCallback, &s_ecspiSlaveSdmaPrivateHandle[instance]);
    SDMA_SetCallback(handle->rxSdmaHandle, SDMA_EcspiSlaveCallback, &s_ecspiSlaveSdmaPrivateHandle[instance]);
}

status_t ECSPI_MasterTransferSDMA(ECSPI_Type *base, ecspi_sdma_handle_t *handle, ecspi_transfer_t *xfer)
{
    assert(base && handle && xfer);

    sdma_transfer_config_t xferConfig;
    sdma_peripheral_t perType = kSDMA_PeripheralNormal;

    /* Check if ECSPI is busy */
    if (handle->state == kECSPI_Busy)
    {
        return kStatus_ECSPI_Busy;
    }

    /* Check if the input arguments valid */
    if (((xfer->txData == NULL) && (xfer->rxData == NULL)) || (xfer->dataSize == 0U))
    {
        return kStatus_InvalidArgument;
    }
    ECSPI_Enable(base, true);
    handle->state = kECSPI_Busy;

    ECSPI_SetChannelSelect(base, xfer->channel);

#if defined(FSL_FEATURE_SOC_SPBA_COUNT) && (FSL_FEATURE_SOC_SPBA_COUNT > 0)
    uint32_t address = (uint32_t)base;
    /* Judge if the instance is located in SPBA */
    if ((address >= FSL_FEATURE_SPBA_START) && (address < FSL_FEATURE_SPBA_END))
    {
        perType = kSDMA_PeripheralNormal_SP;
    }
#endif /* FSL_FEATURE_SOC_SPBA_COUNT */

    /* Prepare transfer. */
    SDMA_PrepareTransfer(&xferConfig, (uint32_t)xfer->txData, (uint32_t) & (base->TXDATA), sizeof(uint8_t),
                         sizeof(uint8_t), sizeof(uint8_t), xfer->dataSize, handle->txSdmaHandle->eventSource, perType,
                         kSDMA_MemoryToPeripheral);

    /* Submit transfer. */
    SDMA_SubmitTransfer(handle->txSdmaHandle, &xferConfig);

    /* Prepare transfer. */
    SDMA_PrepareTransfer(&xferConfig, (uint32_t) & (base->RXDATA), (uint32_t)xfer->rxData, sizeof(uint8_t),
                         sizeof(uint8_t), sizeof(uint8_t), xfer->dataSize, handle->rxSdmaHandle->eventSource, perType,
                         kSDMA_PeripheralToMemory);
    /* Submit transfer. */
    SDMA_SubmitTransfer(handle->rxSdmaHandle, &xferConfig);
    /* Start Rx transfer */
    handle->rxInProgress = true;
    SDMA_StartTransfer(handle->rxSdmaHandle);
    ECSPI_EnableDMA(base, kECSPI_RxDmaEnable, true);

    /* Start Tx transfer */
    handle->txInProgress = true;
    SDMA_StartTransfer(handle->txSdmaHandle);
    ECSPI_EnableDMA(base, kECSPI_TxDmaEnable, true);

    return kStatus_Success;
}

status_t ECSPI_SlaveTransferSDMA(ECSPI_Type *base, ecspi_sdma_handle_t *handle, ecspi_transfer_t *xfer)
{
    assert(base && handle && xfer);

    sdma_transfer_config_t xferConfig;
    sdma_peripheral_t perType = kSDMA_PeripheralNormal;

    /* Check if ECSPI is busy */
    if (handle->state == kECSPI_Busy)
    {
        return kStatus_ECSPI_Busy;
    }

    /* Check if the input arguments valid */
    if (((xfer->txData == NULL) && (xfer->rxData == NULL)) || (xfer->dataSize == 0U))
    {
        return kStatus_InvalidArgument;
    }
    ECSPI_Enable(base, true);
    handle->state = kECSPI_Busy;

    ECSPI_SetChannelSelect(base, xfer->channel);

#if defined(FSL_FEATURE_SOC_SPBA_COUNT) && (FSL_FEATURE_SOC_SPBA_COUNT > 0)
    uint32_t address = (uint32_t)base;
    /* Judge if the instance is located in SPBA */
    if ((address >= FSL_FEATURE_SPBA_START) && (address < FSL_FEATURE_SPBA_END))
    {
        perType = kSDMA_PeripheralNormal_SP;
    }
#endif /* FSL_FEATURE_SOC_SPBA_COUNT */

    /* Prepare transfer. */
    SDMA_PrepareTransfer(&xferConfig, (uint32_t) & (base->RXDATA), (uint32_t)xfer->rxData, sizeof(uint8_t),
                         sizeof(uint8_t), sizeof(uint8_t), xfer->dataSize, handle->rxSdmaHandle->eventSource, perType,
                         kSDMA_PeripheralToMemory);
    /* Submit transfer. */
    SDMA_SubmitTransfer(handle->rxSdmaHandle, &xferConfig);

    /* Prepare transfer. */
    SDMA_PrepareTransfer(&xferConfig, (uint32_t)xfer->txData, (uint32_t) & (base->TXDATA), sizeof(uint8_t),
                         sizeof(uint8_t), sizeof(uint8_t), xfer->dataSize, handle->txSdmaHandle->eventSource, perType,
                         kSDMA_MemoryToPeripheral);

    /* Submit transfer. */
    SDMA_SubmitTransfer(handle->txSdmaHandle, &xferConfig);
    /* start Tx transfer */
    handle->txInProgress = true;
    ECSPI_EnableDMA(base, kECSPI_TxDmaEnable, true);
    SDMA_StartTransfer(handle->txSdmaHandle);

    /* start Rx transfer */
    handle->rxInProgress = true;
    ECSPI_EnableDMA(base, kECSPI_RxDmaEnable, true);
    SDMA_StartTransfer(handle->rxSdmaHandle);

    return kStatus_Success;
}
