#include "buf.h"
#include "heap.h"

struct buf *buf_get(uint16_t size)
{
    uint16_t tol_len = sizeof(struct buf) + MAX_HDR_LEN + size;
    struct buf *sk = heap_malloc(tol_len);
    *sk = (struct buf) {
        .data_len = size,
        .data_mes_len = size,
        .data = sk->data_buf + MAX_HDR_LEN,
        .type = BUF_DATA,
    };
    list_node_init(&(sk->node));
    return sk;
}

void buf_free(void *sk) 
{
    heap_free(sk);
}
