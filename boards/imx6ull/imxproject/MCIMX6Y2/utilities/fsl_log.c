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
#include "fsl_log.h"
#include "fsl_debug_console_conf.h"
#include "fsl_io.h"

#include "sem.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief check queue is full or not  */
#define IS_PRINT_LOG_FULL(buffer) (buffer.ctrl.totalIndex == DEBUG_CONSOLE_PRINT_BUFFER_LEN)
/*! @brief check queue is empty or not  */
#define IS_PRINT_LOG_EMPTY(buffer) (buffer.ctrl.totalIndex == 0U)
/*! @brief buffer index increase with overflow check */
#define LOG_INDEX_INCREASE_WITH_CHECK_OVERFLOW(index, len) \
    \
{                                                   \
        index += len;                                      \
        if (index >= DEBUG_CONSOLE_PRINT_BUFFER_LEN)       \
        {                                                  \
            index -= DEBUG_CONSOLE_PRINT_BUFFER_LEN;       \
        }                                                  \
    \
}

/*! @brief increase push member */
#define LOG_SET_PUSH(buffer, index) (buffer.ctrl.pushIndex = index)
/*! @brief reset push member */
#define LOG_PUSH_RESET(buffer) (buffer.ctrl.pushIndex = 0U)
/*! @brief DECREASE push member */
#define LOG_PUSH_DECREASE(buffer, len) (buffer.ctrl.pushIndex -= len)
/*! @brief get push member */
#define LOG_GET_PUSH(buffer) (buffer.ctrl.pushIndex)

/*! @brief reset pop member */
#define LOG_POP_RESET(buffer) (buffer.ctrl.popIndex = 0U)
/*! @brief DECREASE total member */
#define LOG_POP_DECREASE(buffer, len) (buffer.ctrl.popIndex -= len)
/*! @brief get pop member */
#define LOG_GET_POP(buffer) (buffer.ctrl.popIndex)
/*! @brief increase pop member */
#define LOG_SET_POP(buffer, index) (buffer.ctrl.popIndex = index)

/*! @brief DECREASE total member */
#define LOG_TOTAL_DECREASE(buffer, len) (buffer.ctrl.totalIndex -= len)
/*! @brief increase total member */
#define LOG_SET_TOTAL(buffer, len) (buffer.ctrl.totalIndex = len)
/*! @brief increase total member */
#define LOG_GET_TOTAL(buffer) (buffer.ctrl.totalIndex)
/*! @brief buffer push log */
#define LOG_PUSH_LOG(buffer, index, val) (buffer[index] = val)
/*! @brief Get log addr */
#define LOG_GET_LOG_ADDR(buffer, index) ((uint8_t *)(&(buffer[index])))

#define FSL_RTOS_FREE_RTOS 1
#if DEBUG_CONSOLE_TRANSFER_INTERRUPT
#ifdef FSL_RTOS_FREE_RTOS
/* define for rtos preemption and event */
#define LOG_LOCK_CREATE()                         \
    {                                             \
        LOG_xSemaphore = semaphore_creat(1); \
    \
}

#define LOG_LOCK_TAKE()                                \
    {                                                  \
        semaphore_take(LOG_xSemaphore, 100); \
    }

#define LOG_LOCK_TAKE_FROM_ISR()                     \
    {                                                \
        semaphore_take(LOG_xSemaphore, 0); \
    }

#define LOG_LOCK_RELEASE()              \
    {                                   \
        semaphore_release(LOG_xSemaphore); \
    }

#define LOG_LOCK_RELEASE_FROM_ISR()                  \
    {                                                \
        semaphore_release(LOG_xSemaphore); \
    }

#else
#define LOG_LOCK_CREATE()
#define LOG_LOCK_TAKE()
#define LOG_LOCK_RELEASE()
#endif
#else
#define LOG_LOCK_CREATE()
#define LOG_LOCK_TAKE()
#define LOG_LOCK_RELEASE()
#endif

#if DEBUG_CONSOLE_TRANSFER_INTERRUPT
/*! @brief Define the buffer
* The total buffer size should be calucate as (BUFFER_SUPPORT_LOG_LENGTH + 1) * BUFFER_SUPPORT_LOG_NUM * 4
*/
typedef struct _log_buffer
{
    struct
    {
        volatile uint32_t totalIndex : 10U; /*!< indicate the total usage of the buffer */
        volatile uint32_t pushIndex : 10U;  /*!< indicate the next push index */
        volatile uint32_t popIndex : 10U;   /*!< indicate the pop index */
    } ctrl;
    char log[DEBUG_CONSOLE_PRINT_BUFFER_LEN]; /*!< buffer to store log */
} log_buffer_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* A global log buffer */
static log_buffer_t g_log_buffer;

#ifdef FSL_RTOS_FREE_RTOS
Semaphore_Handle LOG_xSemaphore = NULL;
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief callback when log print finish
 *
 * @param the finished log size, also used to store the avaliable log size
 * @param recieve status
 * @param transmit status
 * @return the next avaliable log addr
 *
 */
static uint8_t *LOG_PrintFinish(size_t *size, bool recieve, bool transmit);

/*!
 * @brief get avaliable log length and address
 *
 * @param size to store available log length
 * @return the next avaliable log addr
 */
static uint8_t *LOG_GetAvailableLog(size_t *size);

/*!
 * @brief take log lock
 *
 */
static void LOG_LockTake(void);

/*!
 * @brief release log lock
 *
 */
static void LOG_LockRelease(void);

#endif /* DEBUG_CONSOLE_TRANSFER_INTERRUPT */

/*******************************************************************************
 * Code
 ******************************************************************************/
#if DEBUG_CONSOLE_TRANSFER_INTERRUPT
static void LOG_LockTake(void)
{
#ifdef __CA7_REV
    if (SystemGetIRQNestingLevel())
#else
    if (__get_IPSR())
#endif
    {
        LOG_LOCK_TAKE_FROM_ISR();
    }
    else
    {
        LOG_LOCK_TAKE();
    }
}

static void LOG_LockRelease(void)
{
#ifdef __CA7_REV
    if (SystemGetIRQNestingLevel())
#else
    if (__get_IPSR())
#endif
    {
        LOG_LOCK_RELEASE_FROM_ISR();
    }
    else
    {
        LOG_LOCK_RELEASE();
    }
}
#endif

status_t LOG_Init(uint32_t baseAddr, uint32_t baudRate, uint32_t clkSrcFreq)
{
#if DEBUG_CONSOLE_TRANSFER_INTERRUPT
    memset(&g_log_buffer, 0, sizeof(g_log_buffer));
    /* io init function */
    IO_Init(baseAddr, baudRate, clkSrcFreq, LOG_PrintFinish);
#else
    IO_Init(baseAddr, baudRate, clkSrcFreq, NULL);
#endif
    /* debug console lock create */
    LOG_LOCK_CREATE();

    return kStatus_Success;
}

void LOG_Deinit(void)
{
#if DEBUG_CONSOLE_TRANSFER_INTERRUPT
    /* memset the global queue */
    memset(&g_log_buffer, 0, sizeof(g_log_buffer));
#endif

    /* Deinit IO */
    IO_Deinit();
}

status_t LOG_WaitIdle(void)
{
#if DEBUG_CONSOLE_TRANSFER_INTERRUPT
    /* wait buffer empty */
    while (!IS_PRINT_LOG_EMPTY(g_log_buffer))
        ;
#endif
    /* call io power down */
    IO_WaitIdle();

    return kStatus_Success;
}

int LOG_Print(char *buf, size_t size)
{
    assert(buf != NULL);

#if DEBUG_CONSOLE_TRANSFER_POLLING
    /* call IO transfer function */
    if (IO_Transmit((uint8_t *)buf, size) != kStatus_Success)
    {
        return 0U;
    }
#else
    uint32_t pushIndex = 0U, i = 0U;
    uint8_t *availableAddr = NULL;

    /* check the buffer if have enough space to store the log */
    if (IS_PRINT_LOG_FULL(g_log_buffer) || (size > (DEBUG_CONSOLE_PRINT_BUFFER_LEN - LOG_GET_TOTAL(g_log_buffer))))
    {
        return -1;
    }

    /* take mutex lock function */
    LOG_LockTake();

    /* get push index */
    pushIndex = LOG_GET_PUSH(g_log_buffer);

    for (i = 0; i < size; i++)
    {
        /* copy log to buffer, the buffer only support a fixed length argument, if the log argument
        is longer than the fixed length, the left argument will be losed */
        LOG_PUSH_LOG(g_log_buffer.log, pushIndex, buf[i]);
        /* check the index is overflow or not */
        LOG_INDEX_INCREASE_WITH_CHECK_OVERFLOW(pushIndex, 1U);
    }
    /* update push/total index value */
    LOG_SET_PUSH(g_log_buffer, pushIndex);
    LOG_SET_TOTAL(g_log_buffer, size + LOG_GET_TOTAL(g_log_buffer));
    /* release mutex lock function */
    LOG_LockRelease();
    /* check log buffer is empty or not */
    if (!(IS_PRINT_LOG_EMPTY(g_log_buffer)))
    {
        availableAddr = LOG_GetAvailableLog(&size);
        /* call IO transfer function */
        if (IO_Transmit(availableAddr, size) != kStatus_Success)
        {
            return -1;
        }
    }
#endif

    return size;
}

int LOG_Scanf(char *buf, size_t size)
{
    assert(buf != NULL);

    int i = 0U;

#if DEBUG_CONSOLE_TRANSFER_INTERRUPT
    /* take mutex lock function */
    LOG_LockTake();
#endif

    for (i = 0; i < size; i++)
    {
        /* recieve one char every time */
        if (IO_Receive((uint8_t *)&buf[i], 1U) != kStatus_Success)
        {
            return -1;
        }
        /* analysis data */
        if (((buf[i] == '\r') || (buf[i] == '\n')) && (size != 1U))
        {
            /* End of Line. */
            if (i == 0)
            {
                buf[i] = '\0';
                i = -1;
            }
            else
            {
                break;
            }
        }
    }
    /* get char should not add '\0'*/
    if (size != 1U)
    {
        if (i == size)
        {
            buf[i] = '\0';
        }
        else
        {
            buf[i + 1] = '\0';
        }
    }

#if DEBUG_CONSOLE_TRANSFER_INTERRUPT
    /* release mutex lock function */
    LOG_LockRelease();
#endif

    return i;
}

#if DEBUG_CONSOLE_TRANSFER_INTERRUPT

static uint8_t *LOG_GetAvailableLog(size_t *size)
{
    uint32_t totalIndex, popIndex;

    /* get index */
    totalIndex = LOG_GET_TOTAL(g_log_buffer);
    popIndex = LOG_GET_POP(g_log_buffer);

    if (totalIndex > (DEBUG_CONSOLE_PRINT_BUFFER_LEN - popIndex))
    {
        *size = DEBUG_CONSOLE_PRINT_BUFFER_LEN - popIndex;
    }
    else
    {
        *size = totalIndex;
    }

    return LOG_GET_LOG_ADDR(g_log_buffer.log, popIndex);
}

static uint8_t *LOG_PrintFinish(size_t *size, bool recieve, bool transmit)
{
    uint32_t popIndex = 0U, totalIndex = 0U;

    if (transmit)
    {
        popIndex = LOG_GET_POP(g_log_buffer);
        totalIndex = LOG_GET_TOTAL(g_log_buffer);
        /* decrease the log total member */
        totalIndex -= *size;
        /* Update total */
        LOG_SET_TOTAL(g_log_buffer, totalIndex);
        /* there is more log in the queue to be pushed */
        if (totalIndex > 0U)
        {
            /* increase the pop index */
            LOG_INDEX_INCREASE_WITH_CHECK_OVERFLOW(popIndex, *size);
            /* update the pop index */
            LOG_SET_POP(g_log_buffer, popIndex);

            return LOG_GetAvailableLog(size);
        }
        else
        {
            /* reset push and pop */
            LOG_PUSH_RESET(g_log_buffer);
            LOG_POP_RESET(g_log_buffer);
        }
    }

    return NULL;
}
#endif /* DEBUG_CONSOLE_TRANSFER_INTERRUPT */
