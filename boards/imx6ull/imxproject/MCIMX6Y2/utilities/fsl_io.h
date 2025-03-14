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
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _FSL_IO_H
#define _FSL_IO_H

#include "fsl_common.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief define a notify callback for IO
* @param size , transfer data size.
* @param rx, indicate a rx transfer is success.
* @param tx, indicate a tx transfer is success.
* @return the next avaliable log address.
*/
typedef uint8_t *(*notify)(size_t *size, bool rx, bool tx);

/*! @brief transfer function pointer
typedef status_t (*dataTransfer)(void *base, const char *fmt, va_list ap);*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif
/*!
 * @brief io init function.
 *
 * Call this function to init IO.
 *
 * @param io base address.
 * @param baud rate
 * @param clock freq
 * @param
 */
void IO_Init(uint32_t baseAddr, uint32_t baudRate, uint32_t clkSrcFreq, void *callBack);

/*!
 * @brief Deinit IO.
 *
 * Call this function to Deinit IO.
 *
 */
void IO_Deinit(void);

/*!
 * @brief io transfer function.
 *
 * Call this function to transmit log.
 * This function can be call by multi thread, so before transmit data, lock must be get,
 * the lock for IO should not blocking the thread, since the interrupt callback will automatically
 * flush the buffer if avaliable.
 * The IO lock should be implement per different rtos environment, if baremental software, lock can
 * be ignored.
 *
 * @param   buffer pointer
 * @param	buffer size
 */
status_t IO_Transmit(uint8_t *ch, size_t size);

/*!
 * @brief io recieve function.
 *
 * Call this function to recieve log.
 * This function can be called by one thread only.Log layer responsible for the
 * call.
 *
 * @param   buffer pointer
 * @param	buffer size
 */
status_t IO_Receive(uint8_t *ch, size_t size);

/*!
 * @brief io power down.
 *
 * Call this function to wait the io idle
 *
 * @param   buffer pointer
 * @param	buffer size
 * @return Indicates whether wait idle was successful or not.
 */
status_t IO_WaitIdle(void);

#if defined(__cplusplus)
}
#endif

#endif /* _FSL_IO_H */
