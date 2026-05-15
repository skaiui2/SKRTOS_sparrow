#include "comm.h"
#include "stm32f1xx_hal.h"   /* 根据你的芯片改头文件 */

static const comm_t *g_comm = 0;
const comm_t *comm = 0;

/* ========= UART 实现 ========= */

static UART_HandleTypeDef *s_uart = NULL;

static void uart_putc(char c)
{
    uint8_t b = (uint8_t)c;
    HAL_UART_Transmit(s_uart, &b, 1, HAL_MAX_DELAY);
}

static char uart_getc(void)
{
    uint8_t b;
    HAL_UART_Receive(s_uart, &b, 1, HAL_MAX_DELAY);
    return (char)b;
}

static void uart_write(const char *buf, int len)
{
    HAL_UART_Transmit(s_uart, (uint8_t *)buf, (uint16_t)len, HAL_MAX_DELAY);
}

static const comm_t uart_comm = {
        .putc  = uart_putc,
        .getc  = uart_getc,
        .write = uart_write,
};

/* ========= 对外初始化接口 ========= */

void comm_init_uart(void *huart_handle)
{
    s_uart = (UART_HandleTypeDef *)huart_handle;
    g_comm = &uart_comm;
    comm   = g_comm;
}
