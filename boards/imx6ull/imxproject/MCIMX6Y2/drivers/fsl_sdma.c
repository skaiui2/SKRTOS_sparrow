/*
 * Copyright (c) 2017, NXP Semiconductors, Inc.
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

#include "fsl_sdma.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get instance number for SDMA.
 *
 * @param base SDMA peripheral base address.
 */
static uint32_t SDMA_GetInstance(SDMAARM_Type *base);

/*!
 * @brief Run scripts for channel0.
 *
 * Channel0 is by default used as the boot channel for SDMA, also the scripts for channel0 will download scripts
 * for other channels from ARM platform to SDMA RAM context.
 *
 * @param base SDMA peripheral base address.
 */
static void SDMA_RunChannel0(SDMAARM_Type *base);

/*!
 * @brief Load the SDMA contex from ARM memory into SDMA RAM region.
 *
 * @param base SDMA peripheral base address.
 * @param channel SDMA channel number.
 * @param tcd Point to TCD structure.
 */
static void SDMA_LoadContext(sdma_handle_t *handle, const sdma_transfer_config_t *config);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Array to map SDMA instance number to base pointer. */
static SDMAARM_Type *const s_sdmaBases[] = SDMAARM_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Array to map SDMA instance number to clock name. */
static const clock_ip_name_t s_sdmaClockName[] = SDMA_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*! @brief Array to map SDMA instance number to IRQ number. */
static const IRQn_Type s_sdmaIRQNumber[FSL_FEATURE_SOC_SDMA_COUNT] = SDMAARM_IRQS;

/*! @brief Pointers to transfer handle for each SDMA channel. */
static sdma_handle_t *s_SDMAHandle[FSL_FEATURE_SOC_SDMA_COUNT][FSL_FEATURE_SDMA_MODULE_CHANNEL];

/*! @brief channel 0 Channel control blcok */
AT_NONCACHEABLE_SECTION_ALIGN(
    static sdma_channel_control_t s_SDMACCB[FSL_FEATURE_SOC_SDMA_COUNT][FSL_FEATURE_SDMA_MODULE_CHANNEL], 4);

/*! @brief channel 0 buffer descriptor */
AT_NONCACHEABLE_SECTION_ALIGN(
    static sdma_buffer_descriptor_t s_SDMABD[FSL_FEATURE_SOC_SDMA_COUNT][FSL_FEATURE_SDMA_MODULE_CHANNEL], 4);
/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t SDMA_GetInstance(SDMAARM_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_sdmaBases); instance++)
    {
        if (s_sdmaBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_sdmaBases));

    return instance;
}

static void SDMA_RunChannel0(SDMAARM_Type *base)
{
    /* Start channel 0 */
    SDMA_StartChannelSoftware(base, 0U);

    /* Waiting for channel 0 finished */
    while ((base->STOP_STAT & 0x1) == 1U)
    {
    }

    /* Clear the channel interrupt status */
    SDMA_ClearChannelInterruptStatus(base, 0x1U);

    /* Set SDMA context switch to dynamic switching */
    SDMA_SetContextSwitchMode(base, kSDMA_ContextSwitchModeDynamic);
}

static uint32_t SDMA_GetScriptAddr(sdma_peripheral_t peripheral, sdma_transfer_type_t type)
{
    uint32_t val = 0;

    if (type == kSDMA_MemoryToMemory)
    {
        val = FSL_FEATURE_SDMA_M2M_ADDR;
    }
    else if (type == kSDMA_MemoryToPeripheral)
    {
        switch (peripheral)
        {
            case kSDMA_PeripheralTypeUART:
            case kSDMA_PeripheralNormal:
                val = FSL_FEATURE_SDMA_M2P_ADDR;
                break;
            case kSDMA_PeripheralTypeUART_SP:
            case kSDMA_PeripheralNormal_SP:
                val = FSL_FEATURE_SDMA_M2SHP_ADDR;
                break;
            case kSDMA_PeripheralTypeSPDIF:
                val = FSL_FEATURE_SDMA_M2SPDIF_ADDR;
                break;
            default:
                break;
        }
    }
    else
    {
        switch (peripheral)
        {
            case kSDMA_PeripheralTypeUART:
                val = FSL_FEATURE_SDMA_UART2M_ADDR;
                break;
            case kSDMA_PeripheralNormal:
                val = FSL_FEATURE_SDMA_P2M_ADDR;
                break;
            case kSDMA_PeripheralTypeUART_SP:
                val = FSL_FEATURE_SDMA_UARTSH2M_ADDR;
                break;
            case kSDMA_PeripheralNormal_SP:
                val = FSL_FEATURE_SDMA_SHP2M_ADDR;
                break;
            case kSDMA_PeripheralTypeSPDIF:
                val = FSL_FEATURE_SDMA_SPDIF2M_ADDR;
                break;
            default:
                break;
        }
    }

    return val;
}

static void SDMA_LoadContext(sdma_handle_t *handle, const sdma_transfer_config_t *config)
{
    uint32_t instance = SDMA_GetInstance(handle->base);
    sdma_context_data_t *context = handle->context;

    memset(context, 0, sizeof(sdma_context_data_t));

    /* Set SDMA core's PC to the channel script address */
    context->PC = config->scriptAddr;

    /* Set the request source into context */
    if (config->eventSource >= 32)
    {
        context->GeneralReg[0] = (1U << (config->eventSource - 32));
    }
    else
    {
        context->GeneralReg[1] = (1U << config->eventSource);
    }

    /* Set source address and dest address for p2p, m2p and p2m */
    if (config->type == kSDMA_MemoryToPeripheral)
    {
        context->GeneralReg[2] = (uint32_t)config->destAddr;
        context->GeneralReg[6] = (uint32_t)config->destAddr;
    }
    else
    {
        context->GeneralReg[2] = (uint32_t)config->srcAddr;
        context->GeneralReg[6] = (uint32_t)config->srcAddr;
    }

    /* Set watermark for p2p, m2p and p2m into context */
    context->GeneralReg[7] = config->bytesPerRequest;

    s_SDMABD[instance][0].command = kSDMA_BDCommandSETDM;
    s_SDMABD[instance][0].status = kSDMA_BDStatusDone | kSDMA_BDStatusWrap | kSDMA_BDStatusInterrupt;
    s_SDMABD[instance][0].count = sizeof(*context) / 4U;
    s_SDMABD[instance][0].bufferAddr = (uint32_t)context;
    s_SDMABD[instance][0].extendBufferAddr = 2048 + (sizeof(*context) / 4) * handle->channel;

    /* Run channel0 scripts after context prepared */
    SDMA_RunChannel0(handle->base);
}

void SDMA_Init(SDMAARM_Type *base, const sdma_config_t *config)
{
    assert(config != NULL);

    uint32_t tmpreg;
    uint32_t instance = SDMA_GetInstance(base);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate SDMA periphral clock */
    CLOCK_EnableClock(s_sdmaClockName[instance]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Clear the channel CCB */
    memset(&s_SDMACCB[instance][0], 0, sizeof(sdma_channel_control_t) * FSL_FEATURE_SDMA_MODULE_CHANNEL);

    /* Reset all SDMA registers */
    SDMA_ResetModule(base);

    /* Init the CCB for channel 0 */
    memset(&s_SDMABD[instance][0], 0, sizeof(sdma_buffer_descriptor_t));
    s_SDMACCB[instance][0].currentBDAddr = (uint32_t)(&s_SDMABD[instance][0]);
    s_SDMACCB[instance][0].baseBDAddr = (uint32_t)(&s_SDMABD[instance][0]);

    /* Set channel 0 priority */
    SDMA_SetChannelPriority(base, 0, 7U);

    /* Set channel 0 ownership */
    base->HOSTOVR = 0U;
    base->EVTOVR = 1U;

    /* Configure SDMA peripheral according to the configuration structure. */
    tmpreg = base->CONFIG;
    tmpreg &= ~(SDMAARM_CONFIG_ACR_MASK | SDMAARM_CONFIG_RTDOBS_MASK);
    /* Channel 0 shall use static context switch method */
    tmpreg |= (SDMAARM_CONFIG_ACR(config->ratio) | SDMAARM_CONFIG_RTDOBS(config->enableRealTimeDebugPin) |
               SDMAARM_CONFIG_CSM(0U));
    base->CONFIG = tmpreg;

    tmpreg = base->SDMA_LOCK;
    tmpreg &= ~SDMAARM_SDMA_LOCK_SRESET_LOCK_CLR_MASK;
    tmpreg |= SDMAARM_SDMA_LOCK_SRESET_LOCK_CLR(config->isSoftwareResetClearLock);
    base->SDMA_LOCK = tmpreg;

    /* Set the context size to 32 bytes */
    base->CHN0ADDR = 0x4050U;

    /* Set channle 0 CCB address */
    base->MC0PTR = (uint32_t)(&s_SDMACCB[instance][0]);
}

void SDMA_Deinit(SDMAARM_Type *base)
{
    /* Clear the MC0PTR register */
    base->MC0PTR = 0U;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate SDMA periphral clock */
    CLOCK_DisableClock(s_sdmaClockName[SDMA_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void SDMA_GetDefaultConfig(sdma_config_t *config)
{
    assert(config != NULL);

    config->enableRealTimeDebugPin = false;
    config->isSoftwareResetClearLock = true;
    config->ratio = kSDMA_HalfARMClockFreq;
}

void SDMA_ResetModule(SDMAARM_Type *base)
{
    uint32_t i = 0, status;

    base->MC0PTR = 0;
    status = base->INTR;
    SDMA_ClearChannelInterruptStatus(base, status);
    status = base->STOP_STAT;
    SDMA_ClearChannelStopStatus(base, status);
    base->EVTOVR = 0;
    base->DSPOVR = 0xFFFFFFFFU;
    base->HOSTOVR = 0;
    status = base->EVTPEND;
    SDMA_ClearChannelPendStatus(base, status);
    base->INTRMASK = 0;

    /* Disable all events */
    for (i = 0; i < FSL_FEATURE_SDMA_EVENT_NUM; i++)
    {
        SDMA_SetSourceChannel(base, i, 0);
    }

    /* Clear all channel priority */
    for (i = 0; i < FSL_FEATURE_SDMA_MODULE_CHANNEL; i++)
    {
        SDMA_SetChannelPriority(base, i, 0);
    }
}

void SDMA_ConfigBufferDescriptor(sdma_buffer_descriptor_t *bd,
                                 uint32_t srcAddr,
                                 uint32_t destAddr,
                                 sdma_transfer_size_t busWidth,
                                 size_t bufferSize,
                                 bool isLast,
                                 bool enableInterrupt,
                                 bool isWrap,
                                 sdma_transfer_type_t type)
{
    /* Set the descriptor to 0 */
    memset(bd, 0, sizeof(sdma_buffer_descriptor_t));
    if (type == kSDMA_PeripheralToMemory)
    {
        bd->bufferAddr = destAddr;
    }
    else
    {
        bd->bufferAddr = srcAddr;
    }
    bd->extendBufferAddr = destAddr;
    if (isLast)
    {
        bd->status |= kSDMA_BDStatusLast;
    }
    else
    {
        bd->status |= kSDMA_BDStatusContinuous;
    }

    /* Set interrupt and wrap feature */
    if (enableInterrupt)
    {
        bd->status |= kSDMA_BDStatusInterrupt;
    }
    if (isWrap)
    {
        bd->status |= kSDMA_BDStatusWrap;
    }

    bd->status |= kSDMA_BDStatusDone;

    /* Configure the command according to bus width */
    bd->command = busWidth;
    bd->count = bufferSize;
}

void SDMA_SetContextSwitchMode(SDMAARM_Type *base, sdma_context_switch_mode_t mode)
{
    uint32_t val = base->CONFIG & (~SDMAARM_CONFIG_CSM_MASK);
    val |= mode;
    base->CONFIG = val;
}

bool SDMA_GetRequestSourceStatus(SDMAARM_Type *base, uint32_t source)
{
    if (source < 32U)
    {
        return ((base->EVT_MIRROR & (1U << source)) >> source);
    }
    else
    {
        source -= 32U;
        return ((base->EVT_MIRROR2 & (1U << source)) >> source);
    }
}

void SDMA_CreateHandle(sdma_handle_t *handle, SDMAARM_Type *base, uint32_t channel, sdma_context_data_t *context)
{
    assert(handle != NULL);
    assert(channel < FSL_FEATURE_SDMA_MODULE_CHANNEL);

    uint32_t sdmaInstance;

    /* Zero the handle */
    memset(handle, 0, sizeof(*handle));

    handle->base = base;
    handle->channel = channel;
    handle->bdCount = 1U;
    handle->context = context;
    /* Get the DMA instance number */
    sdmaInstance = SDMA_GetInstance(base);
    s_SDMAHandle[sdmaInstance][channel] = handle;

    /* Set channel CCB, default is the static buffer descriptor if not use EDMA_InstallBDMemory */
    s_SDMACCB[sdmaInstance][channel].baseBDAddr = (uint32_t)(&s_SDMABD[sdmaInstance][channel]);
    s_SDMACCB[sdmaInstance][channel].currentBDAddr = (uint32_t)(&s_SDMABD[sdmaInstance][channel]);

    /* Enable interrupt */
    EnableIRQ(s_sdmaIRQNumber[sdmaInstance]);
}

void SDMA_InstallBDMemory(sdma_handle_t *handle, sdma_buffer_descriptor_t *BDPool, uint32_t BDCount)
{
    assert(handle && BDPool && BDCount);

    uint32_t sdmaInstance = SDMA_GetInstance(handle->base);

    /* Send user defined buffer descrptor pool to handle */
    handle->BDPool = BDPool;
    handle->bdCount = BDCount;

    /* Update the CCB contents */
    s_SDMACCB[sdmaInstance][handle->channel].baseBDAddr = (uint32_t)(handle->BDPool);
    s_SDMACCB[sdmaInstance][handle->channel].currentBDAddr = (uint32_t)(handle->BDPool);
}

void SDMA_SetCallback(sdma_handle_t *handle, sdma_callback callback, void *userData)
{
    assert(handle != NULL);

    handle->callback = callback;
    handle->userData = userData;
}

void SDMA_PrepareTransfer(sdma_transfer_config_t *config,
                          uint32_t srcAddr,
                          uint32_t destAddr,
                          uint32_t srcWidth,
                          uint32_t destWidth,
                          uint32_t bytesEachRequest,
                          uint32_t transferSize,
                          uint32_t eventSource,
                          sdma_peripheral_t peripheral,
                          sdma_transfer_type_t type)
{
    assert(config != NULL);
    assert((srcWidth == 1U) || (srcWidth == 2U) || (srcWidth == 4U));
    assert((destWidth == 1U) || (destWidth == 2U) || (destWidth == 4U));

    config->srcAddr = srcAddr;
    config->destAddr = destAddr;
    config->bytesPerRequest = bytesEachRequest;
    config->transferSzie = transferSize;
    config->type = type;
    switch (srcWidth)
    {
        case 1U:
            config->srcTransferSize = kSDMA_TransferSize1Bytes;
            break;
        case 2U:
            config->srcTransferSize = kSDMA_TransferSize2Bytes;
            break;
        case 4U:
            config->srcTransferSize = kSDMA_TransferSize4Bytes;
            break;
        default:
            break;
    }
    switch (destWidth)
    {
        case 1U:
            config->destTransferSize = kSDMA_TransferSize1Bytes;
            break;
        case 2U:
            config->destTransferSize = kSDMA_TransferSize2Bytes;
            break;
        case 4U:
            config->destTransferSize = kSDMA_TransferSize4Bytes;
            break;
        default:
            break;
    }
    switch (type)
    {
        case kSDMA_MemoryToMemory:
            config->scriptAddr = FSL_FEATURE_SDMA_M2M_ADDR;
            config->isEventIgnore = true;
            config->isSoftTriggerIgnore = false;
            config->eventSource = 0;
            break;
        case kSDMA_MemoryToPeripheral:
            config->scriptAddr = SDMA_GetScriptAddr(peripheral, kSDMA_MemoryToPeripheral);
            config->isEventIgnore = false;
            config->isSoftTriggerIgnore = true;
            config->eventSource = eventSource;
            break;
        case kSDMA_PeripheralToMemory:
            config->scriptAddr = SDMA_GetScriptAddr(peripheral, kSDMA_PeripheralToMemory);
            config->isEventIgnore = false;
            config->isSoftTriggerIgnore = true;
            config->eventSource = eventSource;
            break;
        default:
            break;
    }
}

void SDMA_SubmitTransfer(sdma_handle_t *handle, const sdma_transfer_config_t *config)
{
    assert(handle != NULL);
    assert(config != NULL);

    uint32_t val = 0U;
    uint32_t instance = SDMA_GetInstance(handle->base);

    handle->eventSource = config->eventSource;

    /* Set event source channel */
    if (config->type != kSDMA_MemoryToMemory)
    {
        val = handle->base->CHNENBL[config->eventSource];
        val |= (1U << (handle->channel));
        SDMA_SetSourceChannel(handle->base, config->eventSource, val);
    }

    /* DO register shall always set */
    handle->base->DSPOVR |= (1U << handle->channel);

    /* Configure EO bit */
    if (config->isEventIgnore)
    {
        handle->base->EVTOVR |= (1U << handle->channel);
    }
    else
    {
        handle->base->EVTOVR &= ~(1U << handle->channel);
    }

    /* Configure HO bits */
    if (config->isSoftTriggerIgnore)
    {
        handle->base->HOSTOVR |= (1U << handle->channel);
    }
    else
    {
        handle->base->HOSTOVR &= ~(1U << handle->channel);
    }

    /* If use default buffer descriptor, configure the buffer descriptor */
    if (handle->BDPool == NULL)
    {
        memset(&s_SDMABD[instance][handle->channel], 0, sizeof(sdma_buffer_descriptor_t));
        if (config->type == kSDMA_MemoryToPeripheral)
        {
            SDMA_ConfigBufferDescriptor(&s_SDMABD[instance][handle->channel], config->srcAddr, config->destAddr,
                                        config->destTransferSize, config->transferSzie, true, true, false,
                                        config->type);
        }
        else
        {
            SDMA_ConfigBufferDescriptor(&s_SDMABD[instance][handle->channel], config->srcAddr, config->destAddr,
                                        config->srcTransferSize, config->transferSzie, true, true, false, config->type);
        }
    }

    /*Load the context */
    SDMA_LoadContext(handle, config);
}

void SDMA_StartTransfer(sdma_handle_t *handle)
{
    assert(handle != NULL);

    /* Set the channel priority */
    if (handle->priority == 0)
    {
        handle->priority = handle->base->SDMA_CHNPRI[handle->channel];
    }

    /* Set priority if regsiter bit is 0*/
    if (handle->base->SDMA_CHNPRI[handle->channel] == 0)
    {
        SDMA_SetChannelPriority(handle->base, handle->channel, handle->priority);
    }

    if (handle->eventSource != 0)
    {
        SDMA_StartChannelEvents(handle->base, handle->channel);
    }
    else
    {
        SDMA_StartChannelSoftware(handle->base, handle->channel);
    }
}

void SDMA_StopTransfer(sdma_handle_t *handle)
{
    assert(handle != NULL);

    SDMA_StopChannel(handle->base, handle->channel);
}

void SDMA_AbortTransfer(sdma_handle_t *handle)
{
    assert(handle);

    uint32_t val = 0;

    SDMA_StopTransfer(handle);

    /* Clear the event map. */
    val = handle->base->CHNENBL[handle->eventSource];
    val &= ~(1U << (handle->channel));
    SDMA_SetSourceChannel(handle->base, handle->eventSource, val);

    /* Clear the channel priority */
    SDMA_SetChannelPriority(handle->base, handle->channel, 0);
}

void SDMA_HandleIRQ(sdma_handle_t *handle)
{
    assert(handle != NULL);
    uint32_t status = 0;

    /* Clear the status for all channels */
    status = (SDMA_GetChannelInterruptStatus(handle->base) & 0xFFFFFFFEU);
    SDMA_ClearChannelInterruptStatus(handle->base, status);

    /* Set the current BD address to the CCB */
    if (handle->BDPool)
    {
        /* Set the DONE bits */
        handle->bdIndex = (handle->bdIndex + 1U) % handle->bdCount;
        s_SDMACCB[0][handle->channel].currentBDAddr = (uint32_t)(&handle->BDPool[handle->bdIndex]);
    }
    else
    {
        s_SDMACCB[0][handle->channel].currentBDAddr = s_SDMACCB[0][handle->channel].baseBDAddr;
    }

    if (handle->callback != NULL)
    {
        (handle->callback)(handle, handle->userData, true, handle->bdIndex);
    }
}

#if defined(SDMAARM)
void SDMA_DriverIRQHandler(void)
{
    uint32_t i = 1U, val;

    /* Clear channel 0 */
    SDMA_ClearChannelInterruptStatus(SDMAARM, 1U);
    /* Ignore channel0, as channel0 is only used for download */
    val = (SDMAARM->INTR) >> 1U;
    while (val)
    {
        if (val & 0x1U)
        {
            SDMA_HandleIRQ(s_SDMAHandle[0][i]);
        }
        i++;
        val >>= 1U;
    }
}
#endif
