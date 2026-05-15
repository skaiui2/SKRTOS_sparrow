#ifndef BUF_H
#define BUF_H

#include "link_list.h"
#include <stdint.h>


#define BUF_DATA  0
#define BUF_DIRTY 1
#define BUF_META  2
#define BUF_INVAL 3

#define MAX_HDR_LEN 128
#define MLEN 1460

struct buf {
    struct list_node node; 
    uint16_t    data_mes_len;    //The message size
    uint16_t    data_len;        //The remaining length of data parsing and the message size
    uint8_t     *data;
    uint8_t     type;
    struct _sockaddr  *sin;
    uint8_t     data_buf[0];
};

struct buf *buf_get(uint16_t size);
void buf_free(void *sk);

#define auto_sk_free __attribute__((cleanup(buf_free)))

#endif