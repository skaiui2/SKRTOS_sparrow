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
#ifndef _FSL_DEBUG_CONSOLE_CONF_H_
#define _FSL_DEBUG_CONSOLE_CONF_H_

/*****************Select Debug console device***************************/
/*! @brief Debug console device select.
* Device must be selected per the platform's capability.
* Support device list at fsl_common.h:
*	#define DEBUG_CONSOLE_DEVICE_TYPE_NONE 0U
*	#define DEBUG_CONSOLE_DEVICE_TYPE_UART 1U
*	#define DEBUG_CONSOLE_DEVICE_TYPE_LPUART 2U
*	#define DEBUG_CONSOLE_DEVICE_TYPE_LPSCI 3U
*	#define DEBUG_CONSOLE_DEVICE_TYPE_USBCDC 4U
*	#define DEBUG_CONSOLE_DEVICE_TYPE_FLEXCOMM 5U
*	#define DEBUG_CONSOLE_DEVICE_TYPE_IUART 6U
*	#define DEBUG_CONSOLE_DEVICE_TYPE_VUSART 7U
*	#define DEBUG_CONSOLE_DEVICE_TYPE_RTT 8U
* UART is the default device for debug console, and this macro must be defined as same as the
* BOARD_DEBUG_UART_TYPE value, otherwise the DebugConsole_Init function will be failed.
*/
#define DEBUG_CONSOLE_DEVICE DEBUG_CONSOLE_DEVICE_TYPE_IUART

/**********************************************************************/

/****************Debug console configuration********************/

#if (DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_UART)

/*! @brief Debug console device transfer function.
* Polling transfer is used for bare-metal software, and buffer is disable for polling.
*/
#ifdef FSL_RTOS_FREE_RTOS
#define DEBUG_CONSOLE_TRANSFER_POLLING (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_POLLING (1U)
#endif

/*! @brief select interrupt transfer if polling is not select
* Interrupt transfer is combined with BUFFER , it can be used by rtos software or bare metal
* software.
*/
#if DEBUG_CONSOLE_TRANSFER_POLLING
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (1U)

/*! @brief define the transmit buffer length which is used to store the multi task log, buffer is enabled when interrupt
* transfer is using,
* This value will affect the RAM's ultilization, should be set per paltform's capability and software requirement.
* If it is configured too small, log maybe missed under RTOS environment, because the log will not be
* buffered if the buffer is full, and the print will return immediately with -1.
*/
#define DEBUG_CONSOLE_PRINT_BUFFER_LEN (512U)
#endif /* DEBUG_CONSOLE_TRANSFER_POLLING */

/*!@ brief define the MAX log length debug console support
* This macro decide the local log buffer length, the buffer locate at stack, the stack maybe overflow if
* the buffer is too long.
*/
#define DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN (128U)
/*!@ brief define the buffer support buffer scanf log length
* As same as the DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN.
*/
#define DEBUG_CONSOLE_BUFFER_SCANF_MAX_LOG_LEN (20U)

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_LPUART)
/*! @brief Debug console device transfer function.
* Polling transfer is used for bare-metal software, and buffer is disable for polling.
*/
#ifdef FSL_RTOS_FREE_RTOS
#define DEBUG_CONSOLE_TRANSFER_POLLING (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_POLLING (1U)
#endif

/*! @brief select interrupt transfer if polling is not select
* Interrupt transfer is combined with BUFFER , it can be used by rtos software or bare metal
* software.
*/
#if DEBUG_CONSOLE_TRANSFER_POLLING
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (1U)

/*! @brief define the transmit buffer length which is used to store the multi task log, buffer is enabled when interrupt
* transfer is using,
* This value will affect the RAM's ultilization, should be set per paltform's capability and software requirement.
* If it is configured too small, log maybe missed under RTOS environment, because the log will not be
* buffered if the buffer is full.
*/
#define DEBUG_CONSOLE_PRINT_BUFFER_LEN (512U)
#endif /* DEBUG_CONSOLE_TRANSFER_POLLING */

/*!@ brief define the MAX log length debug console support
* This macro decide the local log buffer length, the buffer locate at stack, the stack maybe overflow if
* the buffer is too long.
*/
#define DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN (128U)
/*!@ brief define the buffer support buffer scanf log length
* As same as the DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN.
*/
#define DEBUG_CONSOLE_BUFFER_SCANF_MAX_LOG_LEN (20U)

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_LPSCI)
/*! @brief Debug console device transfer function.
* Polling transfer is used for bare-metal software, and buffer is disable for polling.
*/
#ifdef FSL_RTOS_FREE_RTOS
#define DEBUG_CONSOLE_TRANSFER_POLLING (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_POLLING (1U)
#endif

/*! @brief select interrupt transfer if polling is not select
* Interrupt transfer is combined with BUFFER , it can be used by rtos software or bare metal
* software.
*/
#if DEBUG_CONSOLE_TRANSFER_POLLING
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (1U)

/*! @brief define the transmit buffer length which is used to store the multi task log, buffer is enabled when interrupt
* transfer is using,
* This value will affect the RAM's ultilization, should be set per paltform's capability and software requirement.
* If it is configured too small, log maybe missed under RTOS environment, because the log will not be
* buffered if the buffer is full.
*/
#define DEBUG_CONSOLE_PRINT_BUFFER_LEN (512U)
#endif /* DEBUG_CONSOLE_TRANSFER_POLLING */

/*!@ brief define the MAX log length debug console support
* This macro decide the local log buffer length, the buffer locate at stack, the stack maybe overflow if
* the buffer is too long.
*/
#define DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN (128U)
/*!@ brief define the buffer support buffer scanf log length
* As same as the DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN.
*/
#define DEBUG_CONSOLE_BUFFER_SCANF_MAX_LOG_LEN (20U)

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_USBCDC)
/*! @brief Debug console device transfer function.
* Polling transfer is used for bare-metal software, and buffer is disable for polling.
*/
#ifdef FSL_RTOS_FREE_RTOS
#define DEBUG_CONSOLE_TRANSFER_POLLING (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_POLLING (1U)
#endif

/*! @brief select interrupt transfer if polling is not select
* Interrupt transfer is combined with BUFFER , it can be used by rtos software or bare metal
* software.
*/
#if DEBUG_CONSOLE_TRANSFER_POLLING
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (1U)

/*! @brief define the transmit buffer length which is used to store the multi task log, buffer is enabled when interrupt
* transfer is using,
* This value will affect the RAM's ultilization, should be set per paltform's capability and software requirement.
* If it is configured too small, log maybe missed under RTOS environment, because the log will not be
* buffered if the buffer is full.
*/
#define DEBUG_CONSOLE_PRINT_BUFFER_LEN (512U)
#endif /* DEBUG_CONSOLE_TRANSFER_POLLING */

/*!@ brief define the MAX log length debug console support
* This macro decide the local log buffer length, the buffer locate at stack, the stack maybe overflow if
* the buffer is too long.
*/
#define DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN (128U)
/*!@ brief define the buffer support buffer scanf log length
* As same as the DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN.
*/
#define DEBUG_CONSOLE_BUFFER_SCANF_MAX_LOG_LEN (20U)

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_FLEXCOMM)
/*! @brief Debug console device transfer function.
* Polling transfer is used for bare-metal software, and buffer is disable for polling.
*/
#ifdef FSL_RTOS_FREE_RTOS
#define DEBUG_CONSOLE_TRANSFER_POLLING (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_POLLING (1U)
#endif

/*! @brief select interrupt transfer if polling is not select
* Interrupt transfer is combined with BUFFER , it can be used by rtos software or bare metal
* software.
*/
#if DEBUG_CONSOLE_TRANSFER_POLLING
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (1U)

/*! @brief define the transmit buffer length which is used to store the multi task log, buffer is enabled when interrupt
* transfer is using,
* This value will affect the RAM's ultilization, should be set per paltform's capability and software requirement.
* If it is configured too small, log maybe missed under RTOS environment, because the log will not be
* buffered if the buffer is full.
*/
#define DEBUG_CONSOLE_PRINT_BUFFER_LEN (512U)
#endif /* DEBUG_CONSOLE_TRANSFER_POLLING */

/*!@ brief define the MAX log length debug console support
* This macro decide the local log buffer length, the buffer locate at stack, the stack maybe overflow if
* the buffer is too long.
*/
#define DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN (128U)
/*!@ brief define the buffer support buffer scanf log length
* As same as the DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN.
*/
#define DEBUG_CONSOLE_BUFFER_SCANF_MAX_LOG_LEN (20U)

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_IUART)
/*! @brief Debug console device transfer function.
* Polling transfer is used for bare-metal software, and buffer is disable for polling.
*/
#ifdef FSL_RTOS_FREE_RTOS
#if (!defined DEBUG_CONSOLE_TRANSFER_POLLING)
#define DEBUG_CONSOLE_TRANSFER_POLLING (0U)
#endif
#else
#define DEBUG_CONSOLE_TRANSFER_POLLING (1U)
#endif

/*! @brief select interrupt transfer if polling is not select
* Interrupt transfer is combined with BUFFER , it can be used by rtos software or bare metal
* software.
*/
#if DEBUG_CONSOLE_TRANSFER_POLLING
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (1U)

/*! @brief define the transmit buffer length which is used to store the multi task log, buffer is enabled when interrupt
* transfer is using,
* This value will affect the RAM's ultilization, should be set per paltform's capability and software requirement.
* If it is configured too small, log maybe missed under RTOS environment, because the log will not be
* buffered if the buffer is full.
*/
#define DEBUG_CONSOLE_PRINT_BUFFER_LEN (1024U)
#endif /* DEBUG_CONSOLE_TRANSFER_POLLING */

/*!@ brief define the MAX log length debug console support
* This macro decide the local log buffer length, the buffer locate at stack, the stack maybe overflow if
* the buffer is too long.
*/
#define DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN (128U)
/*!@ brief define the buffer support buffer scanf log length
* As same as the DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN.
*/
#define DEBUG_CONSOLE_BUFFER_SCANF_MAX_LOG_LEN (20U)

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_VUSART)
/*! @brief Debug console device transfer function.
* Polling transfer is used for bare-metal software, and buffer is disable for polling.
*/
#ifdef FSL_RTOS_FREE_RTOS
#define DEBUG_CONSOLE_TRANSFER_POLLING (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_POLLING (1U)
#endif

/*! @brief select interrupt transfer if polling is not select
* Interrupt transfer is combined with BUFFER , it can be used by rtos software or bare metal
* software.
*/
#if DEBUG_CONSOLE_TRANSFER_POLLING
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (1U)

/*! @brief define the transmit buffer length which is used to store the multi task log, buffer is enabled when interrupt
* transfer is using,
* This value will affect the RAM's ultilization, should be set per paltform's capability and software requirement.
* If it is configured too small, log maybe missed under RTOS environment, because the log will not be
* buffered if the buffer is full.
*/
#define DEBUG_CONSOLE_PRINT_BUFFER_LEN (512U)
#endif /* DEBUG_CONSOLE_TRANSFER_POLLING */

/*!@ brief define the MAX log length debug console support
* This macro decide the local log buffer length, the buffer locate at stack, the stack maybe overflow if
* the buffer is too long.
*/
#define DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN (128U)
/*!@ brief define the buffer support buffer scanf log length
* As same as the DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN.
*/
#define DEBUG_CONSOLE_BUFFER_SCANF_MAX_LOG_LEN (20U)

#elif(DEBUG_CONSOLE_DEVICE == DEBUG_CONSOLE_DEVICE_TYPE_RTT)
/*! @brief Debug console device transfer function.
* Polling transfer is used for bare-metal software, and buffer is disable for polling.
*/
#define DEBUG_CONSOLE_TRANSFER_POLLING (0U)

/*! @brief select interrupt transfer if polling is not select
* Interrupt transfer is combined with BUFFER , it can be used by rtos software or bare metal
* software.
*/
#if DEBUG_CONSOLE_TRANSFER_POLLING
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (0U)
#else
#define DEBUG_CONSOLE_TRANSFER_INTERRUPT (1U)

/*! @brief define the transmit buffer length which is used to store the multi task log, buffer is enabled when interrupt
* transfer is using,
* This value will affect the RAM's ultilization, should be set per paltform's capability and software requirement.
* If it is configured too small, log maybe missed under RTOS environment, because the log will not be
* buffered if the buffer is full.
*/
#define DEBUG_CONSOLE_PRINT_BUFFER_LEN (512U)
#endif /* DEBUG_CONSOLE_TRANSFER_POLLING */

/*!@ brief define the MAX log length debug console support
* This macro decide the local log buffer length, the buffer locate at stack, the stack maybe overflow if
* the buffer is too long.
*/
#define DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN (128U)
/*!@ brief define the buffer support buffer scanf log length
* As same as the DEBUG_CONSOLE_BUFFER_PRINTF_MAX_LOG_LEN.
*/
#define DEBUG_CONSOLE_BUFFER_SCANF_MAX_LOG_LEN (20U)

#else
#error "no device select"
#endif
/*********************************************************************/

/***************Debug console other configuration*********************/
/*! @brief Definition to printf the float number. */
#ifndef PRINTF_FLOAT_ENABLE
#define PRINTF_FLOAT_ENABLE 1U
#endif /* PRINTF_FLOAT_ENABLE */

/*! @brief Definition to scanf the float number. */
#ifndef SCANF_FLOAT_ENABLE
#define SCANF_FLOAT_ENABLE 0U
#endif /* SCANF_FLOAT_ENABLE */

/*! @brief Definition to support advanced format specifier for printf. */
#ifndef PRINTF_ADVANCED_ENABLE
#define PRINTF_ADVANCED_ENABLE 0U
#endif /* PRINTF_ADVANCED_ENABLE */

/*! @brief Definition to support advanced format specifier for scanf. */
#ifndef SCANF_ADVANCED_ENABLE
#define SCANF_ADVANCED_ENABLE 0U
#endif /* SCANF_ADVANCED_ENABLE */

/*******************************************************************/

#endif
