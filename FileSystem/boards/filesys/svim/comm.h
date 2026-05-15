#ifndef COMM_H
#define COMM_H

#include <stdint.h>
typedef struct {
    void (*putc)(char c);                 /* 输出一个字符 */
    char (*getc)(void);                   /* 阻塞读一个字符 */
    void (*write)(const char *buf, int len); /* 输出一段缓冲区 */

    int  (*peek)(void);                   /* 非阻塞查看下一个字节，-1 表示无数据 */
} comm_t;

/* 当前使用的通信通道（全局指针） */
extern const comm_t *comm;

/* 初始化：绑定到 UART 通道（例如 USART1） */
void comm_init_uart(void *huart_handle);

/* 方便上层用的包装函数 */
static inline void comm_putc(char c)
{
    comm->putc(c);
}

static inline char comm_getc(void)
{
    return comm->getc();
}

static inline void comm_write(const char *buf, int len)
{
    comm->write(buf, len);
}

#endif /* COMM_H */
