/*
 * Copyright (c) 2016, NXP Semiconductors, Inc.
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

#ifndef _FSL_SDMA_H_
#define _FSL_SDMA_H_

#include "fsl_common.h"

/*!
 * @addtogroup sdma
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief SDMA driver version */
#define FSL_SDMA_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0. */
/*@}*/

/*! @brief SDMA transfer configuration */
typedef enum _sdma_transfer_size
{
    kSDMA_TransferSize1Bytes = 0x1U, /*!< Source/Destination data transfer size is 1 byte every time */
    kSDMA_TransferSize2Bytes = 0x2U, /*!< Source/Destination data transfer size is 2 bytes every time */
    kSDMA_TransferSize4Bytes = 0x0U, /*!< Source/Destination data transfer size is 4 bytes every time */
} sdma_transfer_size_t;

/*! @brief SDMA buffer descriptor status */
typedef enum _sdma_bd_status
{
    kSDMA_BDStatusDone = 0x1U,       /*!< BD ownership, 0 means ARM core owns the BD, while 1 means SDMA owns BD. */
    kSDMA_BDStatusWrap = 0x2U,       /*!< While this BD is last one, the next BD will be the first one */
    kSDMA_BDStatusContinuous = 0x4U, /*!< Buffer is allowed to transfer/receive to/from multiple buffers */
    kSDMA_BDStatusInterrupt = 0x8U,  /*!< While this BD finished, send an interrupt. */
    kSDMA_BDStatusError = 0x10U,     /*!< Error occured on buffer descriptor command. */
    kSDMA_BDStatusLast =
        0x20U, /*!< This BD is the last BD in this array. It means the transfer ended after this buffer */
    kSDMA_BDStatusExtend = 0x80, /*!< Buffer descriptor extend status for SDMA scripts */
} sdma_bd_status_t;

/*! @brief SDMA buffer descriptor command */
typedef enum _sdma_bd_command
{
    kSDMA_BDCommandSETDM = 0x1U,  /*!< Load SDMA data memory from ARM core memory buffer. */
    kSDMA_BDCommandGETDM = 0x2U,  /*!< Copy SDMA data memory to ARM core memory buffer. */
    kSDMA_BDCommandSETPM = 0x4U,  /*!< Load SDMA program memory from ARM core memory buffer. */
    kSDMA_BDCommandGETPM = 0x6U,  /*!< Copy SDMA program memory to ARM core memory buffer. */
    kSDMA_BDCommandSETCTX = 0x7U, /*!< Load context for one channel into SDMA RAM from ARM platform memory buffer. */
    kSDMA_BDCommandGETCTX = 0x3U, /*!< Copy context for one channel from SDMA RAM to ARM platform memory buffer. */
} sdma_bd_command_t;

/*! @brief SDMA context switch mode */
typedef enum _sdma_context_switch_mode
{
    kSDMA_ContextSwitchModeStatic = 0x0U,     /*!< SDMA context switch mode static */
    kSDMA_ContextSwitchModeDynamicLowPower,   /*!< SDMA context switch mode dynamic with low power */
    kSDMA_ContextSwitchModeDynamicWithNoLoop, /*!< SDMA context switch mode dynamic with no loop */
    kSDMA_ContextSwitchModeDynamic,           /*!< SDMA context switch mode dynamic */
} sdma_context_switch_mode_t;

/*! @brief SDMA core clock frequency ratio to the ARM DMA interface. */
typedef enum _sdma_clock_ratio
{
    kSDMA_HalfARMClockFreq = 0x0U, /*!< SDMA core clock frequency half of ARM platform */
    kSDMA_ARMClockFreq,            /*!< SDMA core clock frequency equals to ARM platform */
} sdma_clock_ratio_t;

/*! @brief SDMA transfer type */
typedef enum _sdma_transfer_type
{
    kSDMA_MemoryToMemory = 0x0U, /*!< Transfer from memory to memory */
    kSDMA_PeripheralToMemory,    /*!< Transfer from peripheral to memory */
    kSDMA_MemoryToPeripheral,    /*!< Transfer from memory to peripheral */
} sdma_transfer_type_t;

/*! @brief Peripheral type use SDMA */
typedef enum sdma_peripheral
{
    kSDMA_PeripheralTypeMemory = 0x0, /*!< Peripheral DDR memory */
    kSDMA_PeripheralTypeUART,         /*!< UART use SDMA */
    kSDMA_PeripheralTypeUART_SP,      /*!< UART instance in SPBA use SDMA */
    kSDMA_PeripheralTypeSPDIF,        /*!< SPDIF use SDMA */
    kSDMA_PeripheralNormal,           /*!< Normal peripheral use SDMA */
    kSDMA_PeripheralNormal_SP         /*!< Normal peripheral in SPBA use SDMA */
} sdma_peripheral_t;

/*! @brief SDMA transfer status */
enum _sdma_transfer_status
{
    kStatus_SDMA_ERROR = MAKE_STATUS(kStatusGroup_SDMA, 0), /*!< SDMA context error. */
    kStatus_SDMA_Busy = MAKE_STATUS(kStatusGroup_SDMA, 1),  /*!< Channel is busy and can't handle the
                                                                 transfer request. */
};

/*! @brief SDMA global configuration structure.*/
typedef struct _sdma_config
{
    bool enableRealTimeDebugPin;   /*!< If enable real-time debug pin, default is closed to reduce power consumption.*/
    bool isSoftwareResetClearLock; /*!< If software reset clears the LOCK bit which prevent writing SDMA scripts into
                                      SDMA.*/
    sdma_clock_ratio_t ratio;      /*!< SDMA core clock ratio to ARM platform DMA interface */
} sdma_config_t;

/*!
 * @brief SDMA transfer configuration
 *
 * This structure configures the source/destination transfer attribute.
 */
typedef struct _sdma_transfer_config
{
    uint32_t srcAddr;                      /*!< Source address of the transfer */
    uint32_t destAddr;                     /*!< Destination address of the transfer */
    sdma_transfer_size_t srcTransferSize;  /*!< Source data transfer size. */
    sdma_transfer_size_t destTransferSize; /*!< Destination data transfer size. */
    uint32_t bytesPerRequest;              /*!< Bytes to transfer in a minor loop*/
    uint32_t transferSzie;                 /*!< Bytes to transfer for this descriptor */
    uint32_t scriptAddr;                   /*!< SDMA script address located in SDMA ROM. */
    uint32_t eventSource; /*!< Event source number for the channel. 0 means no event, use software trigger */
    bool isEventIgnore;   /*!< True means software trigger, false means hardware trigger */
    bool
        isSoftTriggerIgnore; /*!< If ignore the HE bit, 1 means use hardware events trigger, 0 means software trigger */
    sdma_transfer_type_t type; /*!< Transfer type, transfer type used to decide the SDMA script. */
} sdma_transfer_config_t;

/*!
 * @brief SDMA buffer descriptor structure.
 *
 * This structure is a buffer descriptor, this structure describes the buffer start address and other options
 */
typedef struct _sdma_buffer_descriptor
{
    uint32_t count : 16;       /*!< Bytes of the buffer length for this buffer descriptor. */
    uint32_t status : 8;       /*!< E,R,I,C,W,D status bits stored here */
    uint32_t command : 8;      /*!< command mostlky used for channel 0 */
    uint32_t bufferAddr;       /*!< Buffer start address for this descriptor. */
    uint32_t extendBufferAddr; /*!< External buffer start address, this is an optional for a transfer. */
} sdma_buffer_descriptor_t;

/*!
 * @brief SDMA channel control descriptor structure.
 */
typedef struct _sdma_channel_control
{
    uint32_t currentBDAddr; /*!< Address of current buffer descriptor processed  */
    uint32_t baseBDAddr;    /*!< The start address of the buffer descriptor array */
    uint32_t channelDesc;   /*!< Optional for transfer */
    uint32_t status;        /*!< Channel status */
} sdma_channel_control_t;

/*!
 * @brief SDMA context structure for each channel. This structure can be load into SDMA core, with this structure, SDMA
 * scripts can start work.
 */
typedef struct _sdma_context_data
{
    uint32_t PC : 14;
    uint32_t unused1 : 1;
    uint32_t T : 1;
    uint32_t RPC : 14;
    uint32_t unused0 : 1;
    uint32_t SF : 1;
    uint32_t SPC : 14;
    uint32_t unused2 : 1;
    uint32_t DF : 1;
    uint32_t EPC : 14;
    uint32_t LM : 2;
    uint32_t GeneralReg[8]; /*!< 8 general regsiters used for SDMA RISC core */
    uint32_t MDA;
    uint32_t MSA;
    uint32_t MS;
    uint32_t MD;
    uint32_t PDA;
    uint32_t PSA;
    uint32_t PS;
    uint32_t PD;
    uint32_t CA;
    uint32_t CS;
    uint32_t DDA;
    uint32_t DSA;
    uint32_t DS;
    uint32_t DD;
    uint32_t Scratch0;
    uint32_t Scratch1;
    uint32_t Scratch2;
    uint32_t Scratch3;
    uint32_t Scratch4;
    uint32_t Scratch5;
    uint32_t Scratch6;
    uint32_t Scratch7;
} sdma_context_data_t;

/*! @brief Callback for SDMA */
struct _sdma_handle;

/*! @brief Define callback function for SDMA. */
typedef void (*sdma_callback)(struct _sdma_handle *handle, void *userData, bool transferDone, uint32_t bdIndex);

/*! @brief SDMA transfer handle structure */
typedef struct _sdma_handle
{
    sdma_callback callback;           /*!< Callback function for major count exhausted. */
    void *userData;                   /*!< Callback function parameter. */
    SDMAARM_Type *base;               /*!< SDMA peripheral base address. */
    sdma_buffer_descriptor_t *BDPool; /*!< Pointer to memory stored BD arrays. */
    uint32_t bdCount;                 /*!< How many buffer descriptor   */
    uint32_t bdIndex;                 /*!< How many buffer descriptor   */
    uint32_t eventSource;             /*!< Event source count for the channel */
    sdma_context_data_t *context;     /*!< Channel context to exectute in SDMA */
    uint8_t channel;                  /*!< SDMA channel number. */
    uint8_t priority;                 /*!< SDMA channel priority */
    uint8_t flags;                    /*!< The status of the current channel. */
} sdma_handle_t;

/*******************************************************************************
 * APIs
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name SDMA initialization and de-initialization
 * @{
 */

/*!
 * @brief Initializes the SDMA peripheral.
 *
 * This function ungates the SDMA clock and configures the SDMA peripheral according
 * to the configuration structure.
 *
 * @param base SDMA peripheral base address.
 * @param config A pointer to the configuration structure, see "sdma_config_t".
 * @note This function enables the minor loop map feature.
 */
void SDMA_Init(SDMAARM_Type *base, const sdma_config_t *config);

/*!
 * @brief Deinitializes the SDMA peripheral.
 *
 * This function gates the SDMA clock.
 *
 * @param base SDMA peripheral base address.
 */
void SDMA_Deinit(SDMAARM_Type *base);

/*!
 * @brief Gets the SDMA default configuration structure.
 *
 * This function sets the configuration structure to default values.
 * The default configuration is set to the following values.
 * @code
 *   config.enableRealTimeDebugPin = false;
 *   config.isSoftwareResetClearLock = true;
 *   config.ratio = kSDMA_HalfARMClockFreq;
 * @endcode
 *
 * @param config A pointer to the SDMA configuration structure.
 */
void SDMA_GetDefaultConfig(sdma_config_t *config);

/*!
 * @brief Sets all SDMA core register to reset status.
 *
 * If only reset ARM core, SDMA register cannot return to reset value, shall call this function to reset all SDMA
 * register to reset vaule. But the internal status cannot be reset.
 *
 * @param base SDMA peripheral base address.
 */
void SDMA_ResetModule(SDMAARM_Type *base);

/* @} */
/*!
 * @name SDMA Channel Operation
 * @{
 */
/*!
 * @brief Enables the interrupt source for the SDMA error.
 *
 * Enable this will trigger an interrupt while SDMA occurs error while executing scripts.
 *
 * @param base SDMA peripheral base address.
 * @param channel SDMA channel number.
 */
static inline void SDMA_EnableChannelErrorInterrupts(SDMAARM_Type *base, uint32_t channel)
{
    base->INTRMASK |= (1U << channel);
}

/*!
 * @brief Disables the interrupt source for the SDMA error.
 *
 * @param base SDMA peripheral base address.
 * @param channel SDMA channel number.
 */
static inline void SDMA_DisableChannelErrorInterrupts(SDMAARM_Type *base, uint32_t channel)
{
    base->INTRMASK &= ~(1U << channel);
}

/* @} */
/*!
 * @name SDMA Buffer Descriptor Operation
 * @{
 */

/*!
 * @brief Sets buffer descriptor contents.
 *
 * This function sets the descriptor contents such as source, dest address and status bits.
 *
 * @param bd Pointer to the buffer descriptor structure.
 * @param srcAddr Source address for the buffer descriptor.
 * @param destAddr Destination address for the buffer descriptor.
 * @param busWidth The transfer width, it only can be a member of sdma_transfer_size_t.
 * @param bufferSize Buffer size for this descriptor, this number shall less than 0xFFFF. If need to transfer a big
 * size,
 * shall divide into several buffer descriptors.
 * @param isLast Is the buffer descriptor the last one for the channel to transfer. If only one descriptor used for
 * the channel, this bit shall set to TRUE.
 * @param enableInterrupt If trigger an interrupt while this buffer descriptor transfer finished.
 * @param isWrap Is the buffer descriptor need to be wrapped. While this bit set to true, it will automatically wrap
 * to the first buffer descrtiptor to do transfer.
 * @param type Transfer type, memory to memory, peripheral to memory or memory to peripheral.
 */
void SDMA_ConfigBufferDescriptor(sdma_buffer_descriptor_t *bd,
                                 uint32_t srcAddr,
                                 uint32_t destAddr,
                                 sdma_transfer_size_t busWidth,
                                 size_t bufferSize,
                                 bool isLast,
                                 bool enableInterrupt,
                                 bool isWrap,
                                 sdma_transfer_type_t type);

/*! @} */
/*!
 * @name SDMA Channel Transfer Operation
 * @{
 */

/*!
 * @brief Set SDMA channel priority.
 *
 * This function sets the channel priority. The default value is 0 for all channels, priority 0 will prevents
 * channel from starting, so the priority must be set before start a channel.
 *
 * @param base SDMA peripheral base address.
 * @param channel SDMA channel number.
 * @param priority SDMA channel priority.
 */
static inline void SDMA_SetChannelPriority(SDMAARM_Type *base, uint32_t channel, uint8_t priority)
{
    base->SDMA_CHNPRI[channel] = priority;
}

/*!
 * @brief Set SDMA request source mapping channel.
 *
 * This function sets which channel will be triggered by the dma request source.
 *
 * @param base SDMA peripheral base address.
 * @param source SDMA dma request source number.
 * @param channelMask SDMA channel mask. 1 means channel 0, 2 means channel 1, 4 means channel 3. SDMA supports
 * an event trigger multi-channel. A channel can also be triggered by several source events.
 */
static inline void SDMA_SetSourceChannel(SDMAARM_Type *base, uint32_t source, uint32_t channelMask)
{
    base->CHNENBL[source] = channelMask;
}

/*!
 * @brief Start a SDMA channel by software trigger.
 *
 * This function start a channel.
 *
 * @param base SDMA peripheral base address.
 * @param channel SDMA channel number.
 */
static inline void SDMA_StartChannelSoftware(SDMAARM_Type *base, uint32_t channel)
{
    base->HSTART = (1U << channel);
}

/*!
 * @brief Start a SDMA channel by hardware events.
 *
 * This function start a channel.
 *
 * @param base SDMA peripheral base address.
 * @param channel SDMA channel number.
 */
static inline void SDMA_StartChannelEvents(SDMAARM_Type *base, uint32_t channel)
{
    base->EVTPEND = (1U << channel);
}

/*!
 * @brief Stop a SDMA channel.
 *
 * This function stops a channel.
 *
 * @param base SDMA peripheral base address.
 * @param channel SDMA channel number.
 */
static inline void SDMA_StopChannel(SDMAARM_Type *base, uint32_t channel)
{
    base->STOP_STAT = (1U << channel);
}

/*!
 * @brief Set the SDMA context switch mode.
 *
 * @param base SDMA peripheral base address.
 * @param mode SDMA context switch mode.
 */
void SDMA_SetContextSwitchMode(SDMAARM_Type *base, sdma_context_switch_mode_t mode);

/*! @} */

/*!
 * @name SDMA Channel Status Operation
 * @{
 */

/*!
 * @brief Gets the SDMA interrupt status of all channels.
 *
 * @param base SDMA peripheral base address.
 * @return The interrupt status for all channels. Check the relevant bits for specific channel.
 */
static inline uint32_t SDMA_GetChannelInterruptStatus(SDMAARM_Type *base)
{
    return base->INTR;
}

/*!
 * @brief Clear the SDMA channel interrupt status of specific channels.
 *
 * @param base SDMA peripheral base address.
 * @param mask The interrupt status need to be cleared.
 */
static inline void SDMA_ClearChannelInterruptStatus(SDMAARM_Type *base, uint32_t mask)
{
    base->INTR = mask;
}

/*!
 * @brief Gets the SDMA stop status of all channels.
 *
 * @param base SDMA peripheral base address.
 * @return The stop status for all channels. Check the relevant bits for specific channel.
 */
static inline uint32_t SDMA_GetChannelStopStatus(SDMAARM_Type *base)
{
    return base->STOP_STAT;
}

/*!
 * @brief Clear the SDMA channel stop status of specific channels.
 *
 * @param base SDMA peripheral base address.
 * @param mask The stop status need to be cleared.
 */
static inline void SDMA_ClearChannelStopStatus(SDMAARM_Type *base, uint32_t mask)
{
    base->STOP_STAT = mask;
}

/*!
 * @brief Gets the SDMA channel pending status of all channels.
 *
 * @param base SDMA peripheral base address.
 * @return The pending status for all channels. Check the relevant bits for specific channel.
 */
static inline uint32_t SDMA_GetChannelPendStatus(SDMAARM_Type *base)
{
    return base->EVTPEND;
}

/*!
 * @brief Clear the SDMA channel pending status of specific channels.
 *
 * @param base SDMA peripheral base address.
 * @param mask The pending status need to be cleared.
 */
static inline void SDMA_ClearChannelPendStatus(SDMAARM_Type *base, uint32_t mask)
{
    base->EVTPEND = mask;
}

/*!
 * @brief Gets the SDMA channel error status.
 *
 * SDMA channel error flag is asserted while an incoming DMA request was detected and it triggers a channel
 * that is already pending or being serviced. This probably means there is an overflow of data for that channel.
 *
 * @param base SDMA peripheral base address.
 * @return The error status for all channels. Check the relevant bits for specific channel.
 */
static inline uint32_t SDMA_GetErrorStatus(SDMAARM_Type *base)
{
    return base->EVTERR;
}

/*!
 * @brief Gets the SDMA request source pending status.
 *
 * @param base SDMA peripheral base address.
 * @param source DMA request source number.
 * @return True means the request source is pending, otherwise not pending.
 */
bool SDMA_GetRequestSourceStatus(SDMAARM_Type *base, uint32_t source);

/*! @} */
/*!
 * @name SDMA Transactional Operation
 */

/*!
 * @brief Creates the SDMA handle.
 *
 * This function is called if using the transactional API for SDMA. This function
 * initializes the internal state of the SDMA handle.
 *
 * @param handle SDMA handle pointer. The SDMA handle stores callback function and parameters.
 * @param base SDMA peripheral base address.
 * @param channel SDMA channel number.
 * @param context Context structure for the channel to download into SDMA. Users shall make sure the context located
 * in a non-cacheable memory, or it will cause SDMA run fail. Users shall not touch the context contents, it only be
 * filled by SDMA driver in SDMA_SubmitTransfer function.
 */
void SDMA_CreateHandle(sdma_handle_t *handle, SDMAARM_Type *base, uint32_t channel, sdma_context_data_t *context);

/*!
 * @brief Installs the BDs memory pool into the SDMA handle.
 *
 * This function is called after the SDMA_CreateHandle to use multi-buffer feature.
 *
 * @param handle SDMA handle pointer.
 * @param BDPool A memory pool to store BDs. It must be located in non-cacheable address.
 * @param BDCount The number of BD slots.
 */
void SDMA_InstallBDMemory(sdma_handle_t *handle, sdma_buffer_descriptor_t *BDPool, uint32_t BDCount);

/*!
 * @brief Installs a callback function for the SDMA transfer.
 *
 * This callback is called in the SDMA IRQ handler. Use the callback to do something after
 * the current major loop transfer completes.
 *
 * @param handle SDMA handle pointer.
 * @param callback SDMA callback function pointer.
 * @param userData A parameter for the callback function.
 */
void SDMA_SetCallback(sdma_handle_t *handle, sdma_callback callback, void *userData);

/*!
 * @brief Prepares the SDMA transfer structure.
 *
 * This function prepares the transfer configuration structure according to the user input.
 *
 * @param config The user configuration structure of type sdma_transfer_t.
 * @param srcAddr SDMA transfer source address.
 * @param destAddr SDMA transfer destination address.
 * @param srcWidth SDMA transfer source address width(bytes).
 * @param destWidth SDMA transfer destination address width(bytes).
 * @param bytesEachRequest SDMA transfer bytes per channel request.
 * @param transferSize SDMA transfer bytes to be transferred.
 * @param eventSource Event source number for the transfer, if use software trigger, just write 0.
 * @param peripheral Peripheral type, used to decide if need to use some special scripts.
 * @param type SDMA transfer type. Used to decide the correct SDMA script address in SDMA ROM.
 * @note The data address and the data width must be consistent. For example, if the SRC
 *       is 4 bytes, the source address must be 4 bytes aligned, or it results in
 *       source address error.
 */
void SDMA_PrepareTransfer(sdma_transfer_config_t *config,
                          uint32_t srcAddr,
                          uint32_t destAddr,
                          uint32_t srcWidth,
                          uint32_t destWidth,
                          uint32_t bytesEachRequest,
                          uint32_t transferSize,
                          uint32_t eventSource,
                          sdma_peripheral_t peripheral,
                          sdma_transfer_type_t type);

/*!
 * @brief Submits the SDMA transfer request.
 *
 * This function submits the SDMA transfer request according to the transfer configuration structure.
 *
 * @param handle SDMA handle pointer.
 * @param config Pointer to SDMA transfer configuration structure.
 */
void SDMA_SubmitTransfer(sdma_handle_t *handle, const sdma_transfer_config_t *config);

/*!
 * @brief SDMA starts transfer.
 *
 * This function enables the channel request. Users can call this function after submitting the transfer request
 * or before submitting the transfer request.
 *
 * @param handle SDMA handle pointer.
 */
void SDMA_StartTransfer(sdma_handle_t *handle);

/*!
 * @brief SDMA stops transfer.
 *
 * This function disables the channel request to pause the transfer. Users can call SDMA_StartTransfer()
 * again to resume the transfer.
 *
 * @param handle SDMA handle pointer.
 */
void SDMA_StopTransfer(sdma_handle_t *handle);

/*!
 * @brief SDMA aborts transfer.
 *
 * This function disables the channel request and clear transfer status bits.
 * Users can submit another transfer after calling this API.
 *
 * @param handle DMA handle pointer.
 */
void SDMA_AbortTransfer(sdma_handle_t *handle);

/*!
 * @brief SDMA IRQ handler for complete a buffer descriptor transfer.
 *
 * This function clears the interrupt flags and also handle the CCB for the channel.
 *
 * @param handle SDMA handle pointer.
 */
void SDMA_HandleIRQ(sdma_handle_t *handle);

/* @} */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/* @} */

#endif /*_FSL_SDMA_H_*/
