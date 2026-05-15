#ifndef PORT_H
#define PORT_H
#include <stdint.h>

struct _in_addr {
    unsigned int   addr;
};


#define class(class)    \
typedef struct class##_class class##_class;\
struct class##_class

class(ifnet) {
    struct ifnet *if_next;
    struct list_node *if_addrlist;
    struct _in_addr ipaddr;
    struct _in_addr netmask;
    struct _in_addr gw;
    uint8_t hwaddr[6];
    uint16_t mtu;
    int fd;         //device fd
    void *state;
    void *private;

    int  (*init)(char *ip, char *mac, uint16_t mtu);
    struct buf* (*input)();
    int  (*output)(struct buf *sk);
};

ifnet_class *new_ifnet_class();


#endif 
