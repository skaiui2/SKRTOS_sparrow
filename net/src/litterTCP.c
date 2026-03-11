#include "litterTCP.h"
#include "buf.h"
#include "queue.h"
#include "port.h"
#include "heap.h"

ifnet_class *if_que = NULL;

struct list_node EthReadyQue;
struct list_node EthOutQue;

struct arp_cache AcHead;
struct list_node ArpInQue;

unsigned char broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct list_node IpInQue;

unsigned int ISS = 10086;
struct list_node TcpInpcb;

struct list_node InpQue;
int SocketFd = 0;
struct socket *SocketHash[100];

void buf_dump(struct buf *sk)
{
    if (!sk) {
        printf("buf_dump: sk=NULL\n");
        return;
    }

    printf("---- buf_dump ----\n");
    printf("buf=%p\n", sk);
    printf("data=%p\n", sk->data);
    printf("data_len=%u\n", sk->data_len);
    printf("data_mes_len=%u\n", sk->data_mes_len);
    printf("type=%u\n", sk->type);
    printf("-------------------\n");

    uint8_t *p = sk->data;
    uint16_t len = sk->data_len;

    for (uint16_t i = 0; i < len; i += 16) {
        printf("%04x  ", i);

        // hex part
        for (uint16_t j = 0; j < 16; j++) {
            if (i + j < len)
                printf("%02x ", p[i + j]);
            else
                printf("   ");
        }

        printf(" ");

        // ascii part
        for (uint16_t j = 0; j < 16; j++) {
            if (i + j < len) {
                uint8_t c = p[i + j];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
        }

        printf("\n");
    }

    printf("-------------------\n");
}

void ifnet_register(ifnet_class *ifp)
{
    ifp->if_next = NULL;

    if (if_que == NULL) {
        if_que = ifp;
        return;
    }

    ifnet_class *cur = if_que;
    while (cur->if_next != NULL)
        cur = cur->if_next;

    cur->if_next = ifp;
}

void eth_init()
{
    list_node_init(&EthOutQue);
    list_node_init(&EthReadyQue);
}

void ether_send(ifnet_class *ifp)
{
    struct list_node *sk_node;
    struct buf *sk;
    if (list_empty(&EthOutQue)) return;

    while(sk_node = queue_dequeue(&EthOutQue)) {
        sk = container_of(sk_node, struct buf, node);
        ifp->output(sk);
        buf_free(sk);
    }
}

void ether_input(ifnet_class *ifp)
{
    struct buf *sk = ifp->input();
    if (sk == NULL) {
        SYS_ERROR("SK none!");
        goto freeit;
    }
    //buf_dump(sk);

    struct eth_hdr *eh = (struct eth_hdr *)sk->data;
    if ((memcmp(eh->ether_dhost, ifp->hwaddr, 6) == 0) || 
        (memcmp(eh->ether_dhost, broadcast_mac, 6) == 0)) {
        printf("ether input\r\n");
    } else {
        goto freeit;
    }

    sk->data += sizeof(struct eth_hdr);
    sk->data_len -= sizeof(struct eth_hdr);
    
    switch (ntohs(eh->ether_type))
    {
    case ETH_P_ARP:

        printf("ARP!\r\n");
        queue_enqueue(&ArpInQue, &(sk->node));
        arp_input(ifp);
        
        break;

    case ETH_P_IP:
        printf("IPv4\r\n");
        queue_enqueue(&IpInQue, &(sk->node));
        ip_input(ifp);
        
        break;
    case ETH_P_IPV6:
        printf("ipv6!\r\n");
        break;

    
    default:
        break;
    }
    return;

freeit:
    buf_free(sk);
}

void ether_output(ifnet_class *ifp, struct buf *sk, struct _sockaddr *dst)
{
    struct eth_hdr *eh;
    struct eth_hdr *pkt;
    struct list_node *sk_node;

    sk->data -= sizeof(struct eth_hdr);
    sk->data_len += sizeof(struct eth_hdr);
    eh = (struct eth_hdr *)sk->data;
    memcpy(eh->ether_shost, ifp->hwaddr, 6);

    switch (dst->sa_family)
    {
        case AF_INET:
            
            eh->ether_type = htons(ETHERTYPE_IP);
            if (arp_resolve(ifp, sk, dst)) {
                pkt = (struct eth_hdr *)dst;
                memcpy(eh->ether_dhost, pkt->ether_dhost, 6); 
                queue_enqueue(&EthOutQue, &sk->node);
            } 
            
            break;
        case AF_UNSPEC:
            pkt = (struct eth_hdr *)dst->sa_data;
            memcpy(eh->ether_dhost, pkt->ether_dhost, 6);
            eh->ether_type = pkt->ether_type;  
            queue_enqueue(&EthOutQue, &sk->node);
            break;
    
        default:
            break;
    } 
    //buf_dump(sk);
    ether_send(ifp);
}

void arp_init()
{
    list_node_init(&(AcHead.node));
    list_node_init(&(ArpInQue));
}

int arp_resolve(ifnet_class *ifp, struct buf *sk, struct _sockaddr *dst)
{
    struct arp_cache *acc;
    struct list_node *ac_node;
    struct _sockaddr_in *sa;
    struct eth_hdr *eh;
    unsigned int    ipaddr;

    sa = (struct _sockaddr_in *)dst;
    ipaddr = sa->sin_addr.addr;

    eh = (struct eth_hdr *)dst;
    for (ac_node = AcHead.node.next; ac_node != &(AcHead.node); ac_node = ac_node->next) {
        acc = container_of(ac_node, struct arp_cache, node);
        if (acc->ipaddr == sa->sin_addr.addr) {
            memcpy(eh->ether_dhost, acc->hwaddr, 6);
            return 1;
        }
    }

    if (ac_node == &(AcHead.node)) {
        queue_enqueue(&EthReadyQue, &sk->node);
        arp_request(ifp, &(ifp->ipaddr.addr), &ipaddr);        
    }
    return 0;
}


void arp_request(ifnet_class *ifp, unsigned int *sip, unsigned int *tip)
{
    struct buf *sk;
    struct eth_hdr *eh;
    struct arp_ether  *ae;
    struct arp_hdr *ah;
    struct _sockaddr sa;

    sk = buf_get(0);
    sk->data -= sizeof(struct arp_ether);
    sk->data_len += sizeof(struct arp_ether);

    ae = (struct arp_ether *)sk->data;
    ah = &(ae->ea_hdr);

    ah->ar_hrd = htons(ARPHRD_ETHER);
    ah->ar_pro = htons(ETHERTYPE_IP);
    ah->ar_hln = sizeof(ae->arp_sha);
    ah->ar_pln = sizeof(ae->arp_spa);
    ah->ar_op  = htons(ARPOP_REQUEST);

    memcpy(ae->arp_sha, ifp->hwaddr, 6);
    memcpy(&(ae->arp_spa), sip, 4);

    memset(ae->arp_tha, 0, 6);
    memcpy(&(ae->arp_tpa), tip, 4);

    eh = (struct eth_hdr *)sa.sa_data;
    eh->ether_type = htons(ETHERTYPE_ARP);
	memcpy(eh->ether_dhost, broadcast_mac, 6);

    sa.sa_family = AF_UNSPEC;
    sa.sa_len = sizeof(sa);
    ether_output(ifp, sk, &sa);
}


static void arp_reply(ifnet_class *ifp, struct buf *sk)
{
    struct _sockaddr sa;
    struct eth_hdr *eh;  
    struct arp_ether *pkt  = (struct arp_ether *)sk->data;

    pkt->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
    pkt->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
    pkt->ea_hdr.ar_hln = 6;              
    pkt->ea_hdr.ar_pln = 4;            
    pkt->ea_hdr.ar_op  = htons(ARPOP_REPLY); 

    memcpy(pkt->arp_tha, pkt->arp_sha, 6);
    pkt->arp_tpa = pkt->arp_spa;

    memcpy(pkt->arp_sha, ifp->hwaddr, 6);
    pkt->arp_spa = ifp->ipaddr.addr;
   
    eh = (struct eth_hdr *)sa.sa_data;
	memcpy(eh->ether_dhost, pkt->arp_tha, 6);
	eh->ether_type = htons(ETHERTYPE_ARP);
	sa.sa_family = AF_UNSPEC;
	sa.sa_len = sizeof(sa);

    ether_output(ifp, sk, &sa);  
}

void arp_eth_ready_que_cpy_mac(ifnet_class *ifp, struct arp_ether *ap)
{
    struct list_node *sk_node;
    struct list_node *next_node;
    struct buf *sk;
    struct ip_struct *ip;
    struct eth_hdr *eh;

    sk_node = EthReadyQue.next;
    while(sk_node != &EthReadyQue) {
        next_node = sk_node->next;
        sk = container_of(sk_node, struct buf, node);
        ip = (struct ip_struct *)(sk->data + sizeof(struct eth_hdr));
        eh = (struct eth_hdr *)(sk->data);
        if (ip->ip_dst.addr == ap->arp_spa) { 
            memcpy(eh->ether_dhost, ap->arp_sha, 6);
            list_remove(sk_node);
            queue_enqueue(&EthOutQue, sk_node);
        }

        sk_node = next_node;
    }
    ether_send(ifp);
}


void arp_input(ifnet_class *ifp)
{ 
    struct buf *sk;
    struct arp_ether *ap;
    struct arp_hdr *ah;
    struct list_node *ai_node;
    struct list_node *ac_node;
    struct arp_cache *ac;

    ai_node = ArpInQue.next;
    while(ai_node != &ArpInQue) 
    {
        sk  = container_of(ai_node, struct buf, node);
        list_remove(ai_node);
        ai_node = ai_node->next;
        
        ap = (struct arp_ether *)sk->data;
        ah = (struct arp_hdr *)&(ap->ea_hdr);

        for (ac_node = AcHead.node.next; ac_node != &(AcHead.node); ac_node = ac_node->next) {
            ac = container_of(ac_node, struct arp_cache, node);
            if (ac->ipaddr == ap->arp_spa) {
                break;
            }
        }
        if (ac_node == &(AcHead.node)) {
            ac = heap_malloc(sizeof(struct arp_cache));
            ac->ipaddr = ap->arp_spa;
            memcpy(ac->hwaddr, ap->arp_sha, 6);
            printf("ac.ip has add\r\n");
            queue_enqueue(&AcHead.node, &ac->node);
        }

        arp_eth_ready_que_cpy_mac(ifp, ap);
        
        switch (ntohs(ah->ar_op))
        {
        case ARPOP_REQUEST:
            arp_reply(ifp, sk);
        break;
        case ARPOP_REPLY:

        break;

        default:
        break;
        }
    }

}



unsigned short in_checksum(void *b, int len) 
{
    unsigned short *addr = (unsigned short *)b;
    long sum = 0;

    for (len; len > 1; len -= 2) {
        sum += *(unsigned short *)addr++;
        if (sum & 0x80000000) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
    }
    if (len) {
        sum += (unsigned short)(*(unsigned char *)addr);
    }
    while(sum >> 16) {
        sum = (sum >> 16) + (sum & 0xFFFF);
    }

    return ~sum;
}



#define MAX_ROUTES 16
struct rtentry routing_table[MAX_ROUTES];
int route_count = 0;

int route_add(uint32_t dest, uint32_t netmask, uint32_t gateway, ifnet_class *ifp)
{
    if (route_count >= MAX_ROUTES) {
        printf("route_add: routing table full!\n");
        return -1;
    }

    routing_table[route_count] = (struct rtentry) {
        .dest    = dest,
        .netmask = netmask,
        .gateway = gateway,
        .ifp     = ifp,
    };

    route_count++;
    return 0;
}


struct rtentry* rtlookup(uint32_t dst) {
    struct rtentry *best = NULL;
    uint32_t best_mask = 0;
    for (int i = 0; i < route_count; i++) {
        if ((dst & routing_table[i].netmask) == routing_table[i].dest) {
            if (routing_table[i].netmask >= best_mask) {
                best = &routing_table[i];
                best_mask = routing_table[i].netmask;
            }
        }
    }
    return best;
}

void ip_init()
{
    list_node_init(&IpInQue);
}

void ip_InQue_add_tail(struct buf *sk)
{
    queue_enqueue(&IpInQue, &(sk->node));
}

void ip_InQue_remove_tail(struct buf *sk)
{
    queue_dequeue(&IpInQue);
}

int is_local_ip(struct _in_addr ip)
{
    ifnet_class *ifp;

    uint32_t target = ntohl(ip.addr);  

    for (ifp = if_que; ifp != NULL; ifp = ifp->if_next) {
        uint32_t my_ip   = ifp->ipaddr.addr;  
        uint32_t netmask = ifp->netmask.addr;

        if (target == my_ip)
            return 1;

        if ((target & netmask) == (my_ip & netmask))
            return 1;

        uint32_t broadcast = (my_ip & netmask) | (~netmask);
        if (target == broadcast)
            return 1;

        if (target == 0)
            return 1;

        if ((target & 0xFF000000) == 0x7F000000)
            return 1;
    }

    return 0;
}

void ip_forward(struct buf *sk, struct ip_struct *ip)
{
    struct rtentry *rt;
    struct ifnet *ifp;
    uint32_t nexthop;

    if (ip->ip_ttl <= 1) {
        printf("ip_forward: TTL expired\n");
        buf_free(sk);
        return;
    }
    ip->ip_ttl--;

    rt = rtlookup(ip->ip_dst.addr);
    if (!rt) {
        printf("ip_forward: no route\n");
        buf_free(sk);
        return;
    }
    ifp = rt->ifp;
    nexthop = (rt->gateway != 0) ? rt->gateway : ip->ip_dst.addr;

    ip->ip_sum = 0;
    ip->ip_sum = in_checksum(ip, ip->ip_hl << 2);

    struct _sockaddr dst;
    dst.sa_family = AF_INET;
    ((struct _sockaddr_in*)&dst)->sin_addr.addr = nexthop;

    ether_output(ifp, sk, &dst);
}

void ip_input()
{
    struct list_node *first_node;
    struct buf *sk;
    struct ip_struct *ip;
    unsigned short csum;
    int hlen;

    first_node = IpInQue.next;
    while(first_node != &IpInQue) {
        list_remove(first_node);

        sk  = container_of(first_node, struct buf, node);
        first_node = first_node->next;

        ip = (struct ip_struct *)sk->data;
        if (ip->ip_v != IPVERSION) {
            SYS_ERROR("It is not IPv4\r\n");
            goto freeit;
        }

        csum = in_checksum((void *)ip, (ip->ip_hl) << 2); 
        if (csum != 0) {
            SYS_ERROR("ip input check sum error!\r\n");
            goto freeit;
        }

        hlen = ip->ip_hl << 2; 

        if (!is_local_ip(ip->ip_dst)) {
            ip_forward(sk, ip);
            continue; 
        }

        sk->data  += hlen;
        sk->data_len -= hlen;

        switch (ip->ip_p) 
        {
        case IPPROTO_ICMP:
            printf("ICMP!\r\n");
            icmp_input(sk, hlen); 
            break;

        case IPPROTO_UDP:
            printf("udp input\r\n");
            udp_input(sk, hlen);
            break;

        case IPPROTO_TCP:
            printf("tcp input!\r\n");
            tcp_input(sk, hlen);
            break;
    
        default:
            buf_free(sk);
            break;
        }
    }
    return;

freeit:
    buf_free(sk);
}


unsigned short	ip_id = 2;

#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
#define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */


int ip_output(struct buf *sk, struct _sockaddr_in *sa)
{
    struct ip_struct *ip;
    int hlen;
    struct rtentry *rt;
    ifnet_class *ifp;
    uint32_t nexthop;

    rt = rtlookup(sa->sin_addr.addr);
    if (rt == NULL) {
        printf("ip_output: no route to host\n");
        return false;
    }
    ifp = rt->ifp;

    if (rt->gateway != 0)
        nexthop = rt->gateway;
    else
        nexthop = sa->sin_addr.addr;

    ip = (struct ip_struct *)sk->data;
    hlen = sizeof(struct ip_struct);

    ip->ip_hl  = hlen >> 2;
    ip->ip_v   = IPVERSION;
    ip->ip_tos = 0;
    ip->ip_len = htons(sk->data_len);
    ip->ip_id  = htons(ip_id++);
    //now no fragment
    ip->ip_off = 0;      
    ip->ip_ttl = 64;
    ip->ip_p   = sk->type;
    ip->ip_src = ifp->ipaddr;      
    ip->ip_dst = sa->sin_addr;     
    ip->ip_sum = 0;
    ip->ip_sum = in_checksum(ip, hlen);

    struct _sockaddr dst;
    dst.sa_family = AF_INET;
    ((struct _sockaddr_in*)&dst)->sin_addr.addr = nexthop;
    //printf("ip out\n");
    //buf_dump(sk);
    ether_output(ifp, sk, &dst);
    return true;
}

void icmp_send(struct buf *sk)
{
    struct ip_struct *ip;
    struct _sockaddr_in sa;

    sk->data -= sizeof(struct ip_struct);
    sk->data_len += sizeof(struct ip_struct);

    ip = (struct ip_struct *)sk->data;
    sk->type = ip->ip_p;

    sa.sin_family = AF_INET;
    sa.sin_addr = ip->ip_src;
    sa.sin_len = sizeof(sa);

    ip_output(sk, &sa);
}


void icmp_reflect(struct buf *sk)
{
    unsigned short len = sk->data_len + sizeof(struct ip_struct) + sizeof(struct eth_hdr);
    struct icmp *send_icp = (struct icmp *)sk->data;
    send_icp->icmp_type = ICMP_ECHOREPLY;
    send_icp->icmp_cksum = 0;
    send_icp->icmp_cksum = in_checksum((void *)send_icp, len);
    icmp_send(sk);
}

void icmp_input(struct buf *sk, int hlen)
{
    struct icmp *icp;
    icp = (struct icmp *)sk->data;

    switch (icp->icmp_type)
    {
    case ICMP_ECHO:
        icmp_reflect(sk);
        
        break;
    
    default:
        break;
    }

}

int in_pcballoc(struct socket *so, struct list_node *head)
{
	struct inpcb *inp = heap_malloc(sizeof(struct inpcb));
	if (inp == NULL) {
		SYS_ERROR("socket NULL!!!");
		return ERROR;
	}
	*inp = (struct inpcb) {
		.inp_socket = so,
	};
	list_node_init(&(inp->node));
	queue_enqueue(head, &inp->node);
	so->so_pcb = (void *)inp;	
	return 0;
}


int in_pcbfree(struct inpcb *inp)
{
	list_remove(&inp->node);
	heap_free(inp);
}

struct inpcb *in_pcblookup(struct list_node *head, struct _in_addr faddr, struct _in_addr laddr, 
						unsigned int fport, unsigned int lport)
{
	struct list_node *inp_node; 
	struct inpcb *inp = NULL;
	struct inpcb *match_pcb = NULL;
	char count = 0;
	char max_match = 0;
	for(inp_node = head->next; inp_node != head; inp_node = inp_node->next) {
		inp = container_of(inp_node, struct inpcb, node);
		printf("inpcb 38 is %p\n", inp);
		if ((lport == INADDR_ANY) || 
			(inp->inp_lport == lport)) {
			count++;
		} 
		if ((inp->inp_laddr.addr == INADDR_ANY) || 
			(inp->inp_laddr.addr == laddr.addr)) {
			count++;
		}

		if (inp->inp_fport == fport) {
			count++;
		}
		if (inp->inp_faddr.addr == faddr.addr) {
			count++;
		}

		if (count > max_match) {
			max_match = count;
			match_pcb = inp;
		}
		count = 0;
	}

	if (match_pcb == NULL) {
		SYS_ERROR("inpcb no find!");
	}

	return match_pcb;
}


int in_pcbbind(struct inpcb *inp, struct _sockaddr_in *sin)
{
	if (sin->sin_family != AF_INET) {
		SYS_ERROR("sin_family is error!");
		return ERROR;
	}

	inp->inp_laddr = sin->sin_addr;
	inp->inp_lport = sin->sin_port;  
}

int in_pcbconnect(struct inpcb *inp, struct _sockaddr_in *fsin)
{
	if (fsin->sin_family != AF_INET) {
		SYS_ERROR("sin_family is error!");
		return ERROR;
	}
	
	if (fsin->sin_port == 0) {
		SYS_ERROR("port is 0");
		return ERROR;
	}
	
	inp->inp_faddr = fsin->sin_addr;
	inp->inp_fport = fsin->sin_port;
	return	0;
}

int in_pcbdisconnect(struct inpcb *inp)
{
	inp->inp_faddr.addr = INADDR_ANY;
	inp->inp_fport = 0;
	return true;
}

int in_setsockaddr(struct inpcb *inp, struct _sockaddr_in *lsin)
{
	lsin->sin_family = AF_INET;
	lsin->sin_len = sizeof(*lsin);
	lsin->sin_port = inp->inp_lport;
	lsin->sin_addr = inp->inp_laddr;
}

int in_setpeeraddr(struct inpcb *inp, struct _sockaddr_in *fsin)
{
	fsin->sin_family = AF_INET;
	fsin->sin_len = sizeof(*fsin);
	fsin->sin_port = inp->inp_fport;
	fsin->sin_addr = inp->inp_faddr;
}

void udp_input(struct buf *sk, int iphlen)
{
    struct udpiphdr *ui;
    struct udphdr *uh;
    struct ip_struct *ip;
    struct ip_struct save_ip;
    unsigned short udp_len;
    unsigned char *payload;

    ip = (struct ip_struct *)(sk->data - iphlen);
    save_ip = *ip;
	
    if (iphlen < sizeof(struct ip_struct)) {
		SYS_ERROR("iphlen error!");
        goto freeit; 
    }

    ui = (struct udpiphdr *)ip;
    uh = &(ui->ui_u);

    udp_len = ntohs(uh->uh_ulen);

    if (udp_len < sizeof(struct udphdr) ||
        udp_len > sk->data_len) {
        goto freeit; 
    }

	if (uh->uh_sum) {
		((struct ipovly *)ip)->ih_next = 0;
		((struct ipovly *)ip)->ih_prev = 0;
		((struct ipovly *)ip)->ih_x1 = 0;
		((struct ipovly *)ip)->ih_len = uh->uh_ulen;
		if (uh->uh_sum = in_checksum(ip, sk->data_len + sizeof(struct ip_struct))) {
			SYS_ERROR("udp error!!!\r\n");
			goto freeit;
		} 
	}
	*ip = save_ip;

	sk->data += sizeof(struct udphdr);
	sk->data_len -= sizeof(struct udphdr);

	
	struct list_node *inp_node;
	for(inp_node = InpQue.next; inp_node != &InpQue; inp_node = inp_node->next) {
		struct inpcb *inp = container_of(inp_node, struct inpcb, node);
		inp->sk = sk;
		inp->inp_fport = uh->uh_sport;
		sem_post(&inp->recv_sem);
	}
	return;

freeit:
	buf_free(sk);
}



int udp_output(struct inpcb *inp, struct buf *sk, struct _sockaddr  *sa)
{
    register struct udpiphdr *ui;
	struct ip_struct *ip;
	int error = 0;
	int len;
	
	printf("udpout\r\n");

	len = sk->data_len;
	ui = (struct udpiphdr *)(sk->data - sizeof(struct udpiphdr));
	ip = (struct ip_struct *)ui;
	

	sk->data -= sizeof(struct udpiphdr);
	sk->data_len += sizeof(struct udpiphdr);
	
	ui->ui_i.ih_next = ui->ui_i.ih_prev = 0;
	ui->ui_i.ih_x1 = 0;
	ui->ui_i.ih_pr = IPPROTO_UDP;
	ui->ui_i.ih_len = htons((unsigned short)len + sizeof (struct udphdr));
	ui->ui_i.ih_src = inp->inp_laddr;
	ui->ui_i.ih_dst = inp->inp_faddr;
	ui->ui_u.uh_sport = inp->inp_lport;
	ui->ui_u.uh_dport = inp->inp_fport;
	
	ui->ui_u.uh_ulen = ui->ui_i.ih_len;
	ui->ui_u.uh_sum = 0;
	if ((ui->ui_u.uh_sum = in_checksum(ui, sizeof (struct udpiphdr) + len)) == 0) {
		ui->ui_u.uh_sum = 0xffff;
	}
    
    ip_output(sk, sa);
	return error;
}

void tcp_init()
{
    list_node_init(&TcpInpcb);
}


void tcp_input(struct buf *sk, int iphlen)
{
    struct tcpiphdr *ti;
    unsigned short csum;
    struct ip_struct *ip;
    struct ip_struct save_ip;
    int len, tlen, off;

    sk->data -= iphlen;
    sk->data_len += iphlen;

    ti = (struct tcpiphdr *)sk->data;
    ip = (struct ip_struct *)ti;
    save_ip = *ip;
	
    if (iphlen < sizeof(struct ip_struct)) {
		SYS_ERROR("iphlen error!");
        goto free; 
    }

    ip->ip_len = ntohs(ip->ip_len);
    len = ip->ip_len;
    
    tlen = ip->ip_len - iphlen;
    
    ti->ti_i.ih_next = ti->ti_i.ih_prev = 0;
    ti->ti_i.ih_x1 = 0;
    ti->ti_i.ih_len = htons(tlen);
    ti->ti_i.ih_src = ip->ip_src; 

    csum = -1;
    csum = in_checksum((void *)ti, len); 
    if (csum != 0) {
        SYS_ERROR("tcp input check sum error!\r\n");
        goto free;
    }

    *ip =save_ip;

    off = ti->ti_t.th_off << 2;
    if ((off < sizeof(struct tcphdr)) || (off > tlen)) {
        SYS_ERROR("off length error!");
        goto free;
    }
    tlen -= off;
    ti->ti_i.ih_len = tlen;


    struct inpcb *inp = in_pcblookup(&TcpInpcb, ip->ip_src, ip->ip_dst, ti->ti_t.th_sport, ti->ti_t.th_dport);
    struct tcpcb *tp = inp->inp_ppcb;

    struct _sockaddr_in *sin;
    unsigned int ack = ntohl(ti->ti_t.th_ack);
    unsigned int seq = ntohl(ti->ti_t.th_seq);
    int flags = ti->ti_t.th_flags;
    printf("flags:%d\r\n", flags);
    printf("tp->state:%d\r\n", tp->t_state);

    printf("ack for sk:%u\r\n", ack);
    printf("seq for sk:%u\r\n", seq);

    switch (tp->t_state)
    {
    case TCP_LISTEN:
        if (flags & TH_RST)
			goto free;
		if (flags & TH_ACK)
			goto free;
		if ((flags & TH_SYN) == 0)
			goto free;

        sin = heap_malloc(sizeof(struct _sockaddr_in));
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(*sin);
		sin->sin_addr = ti->ti_i.ih_src;
		sin->sin_port = ti->ti_t.th_sport;
        
		struct _in_addr laddr = inp->inp_laddr;
		if (inp->inp_laddr.addr == INADDR_ANY)
			inp->inp_laddr = ti->ti_i.ih_dst;
		if (in_pcbconnect(inp, sin)) {
			inp->inp_laddr = laddr;
			goto free;
		}
		heap_free(sin);

        tp->iss = ISS;
        tp->snd_una = ISS;
        tp->snd_nxt = ISS + 1;
        tp->rcv_nxt = ntohl(ti->ti_t.th_seq) + 1;

        printf("listen\r\n");
        tp->t_state = TCP_SYN_RECEIVED;
        struct buf *sk_syn_ack = buf_get(0);
        sk_syn_ack->data_len = sizeof(struct tcpiphdr);
        tcp_respond(tp, sk_syn_ack, tp->rcv_nxt, tp->iss, TH_SYN | TH_ACK);

        return;

    case TCP_SYN_SENT:
        if (flags & TH_RST) {
            goto free;
        }

        if ((flags & (TH_SYN|TH_ACK)) == (TH_SYN|TH_ACK)) {
            if (SEQ_GEQ(ack, ISS + 1) && SEQ_LEQ(ack, tp->snd_nxt)) { 
                tp->snd_una = ack;
            }
            tp->rcv_nxt = ntohl(ti->ti_t.th_seq) + 1;

            struct buf *sk_ack = buf_get(0);
            sk_ack->data_len = sizeof(struct tcpiphdr);
            tcp_respond(tp, sk_ack, tp->rcv_nxt, tp->snd_nxt, TH_ACK);

            tp->t_state = TCP_ESTABLISHED;   
            sem_post(&inp->sem_connected);
            return;
        }

        if ((flags & TH_SYN) && !(flags & TH_ACK)) {
            tp->rcv_nxt = ntohl(ti->ti_t.th_seq) + 1;
            tp->t_state = TCP_SYN_RECEIVED;
            struct buf *sk_syn_ack = buf_get(0);
            sk_syn_ack->data_len = sizeof(struct tcpiphdr);
            
            tcp_respond(tp, sk_syn_ack, tp->rcv_nxt, tp->iss, TH_SYN | TH_ACK);
            return;
        }

        goto free;


    case TCP_SYN_RECEIVED:
        if (flags & TH_RST) {
            printf("syn_received: got RST\r\n");
            tp->t_state = TCP_CLOSED;
            goto free;
        }

        if (flags & TH_ACK) {
            if (SEQ_GT(ack, tp->snd_una) && SEQ_LEQ(ack, tp->snd_nxt)) {
                tp->snd_una = ack;  
                tp->t_state = TCP_ESTABLISHED;
                sem_post(&inp->sem_connected);
                printf("syn_received: got valid ACK, enter ESTABLISHED\n");
            } else {
                printf("syn_received: invalid ACK %u (expect between %u and %u)\n",ack, tp->snd_una+1, tp->snd_nxt);
                goto free;
            }
        }

        // TODO: if simultaneous open!
        return;


    case TCP_ESTABLISHED: {
        uint32_t seqno = ntohl(ti->ti_t.th_seq);
        uint32_t ackno = ntohl(ti->ti_t.th_ack);

        if ((flags & (TH_PUSH|TH_ACK)) == (TH_PUSH|TH_ACK)) {
            if (SEQ_EQ(seqno, tp->rcv_nxt)) {
                sk->data_mes_len = sk->data_len - iphlen - off;
                printf("sk->data_mes_len:%d\r\n", sk->data_mes_len);
                inp->inp_fport = ti->ti_t.th_sport;
                inp->sk = sk;
                void *data = sk->data + iphlen + off;
                inp->recv_data = heap_malloc(sk->data_mes_len);
                inp->recv_len = sk->data_mes_len;
                memcpy(inp->recv_data, data, inp->recv_len);

                tp->rcv_nxt = seqno + sk->data_mes_len;

                struct buf *sk_ack = buf_get(0);
                sk_ack->data_len = sizeof(struct tcpiphdr);

                printf("tp->snd_nxt:seq:%d\r\n", tp->snd_nxt);
                tcp_respond(tp, sk_ack, tp->rcv_nxt, tp->snd_nxt, TH_ACK);

                sem_post(&inp->recv_sem);
                printf("TCP_EST: received data, posted recv_sem\n");
            } else {
                goto free; 
            }
        }

        // ACK 
        if ((flags & TH_ACK) && !(flags & TH_PUSH) && !(flags & TH_FIN)) {
            if (SEQ_GT(ackno, tp->snd_una) && SEQ_LEQ(ackno, tp->snd_nxt)) {
                tp->snd_una = ackno;
                sem_post(&inp->send_sem);
                printf("TCP_EST: pure ACK, advanced snd_una\n");
            } else {
                goto free;
            }
        }

        // FIN 
        if (flags & TH_FIN) {
            if (SEQ_EQ(seqno, tp->rcv_nxt)) {
                tp->rcv_nxt++;

                struct buf *sk_ack = buf_get(0);
                sk_ack->data_len = sizeof(struct tcpiphdr);
                tcp_respond(tp, sk_ack, tp->rcv_nxt, tp->snd_nxt, TH_ACK);

                tp->t_state = TCP_CLOSE_WAIT;
                // TODO: notice application
                printf("TCP_EST: received FIN, enter CLOSE_WAIT\n");
            } else {
                goto free;
            }
        }
        return;
    }

    case TCP_FIN_WAIT_1: {
        uint32_t seqno = ntohl(ti->ti_t.th_seq);
        uint32_t ackno = ntohl(ti->ti_t.th_ack);

        if (flags & TH_ACK) {
            if (SEQ_GT(ackno, tp->snd_una) && SEQ_LEQ(ackno, tp->snd_nxt)) {
                tp->snd_una = ackno;
                if (ackno == tp->snd_nxt) {
                    tp->t_state = TCP_FIN_WAIT_2;
                }
            }
        }   

        if (flags & TH_FIN) {
            if (SEQ_EQ(seqno, tp->rcv_nxt)) {
                tp->rcv_nxt++;

                struct buf *sk_ack = buf_get(0);
                sk_ack->data_len = sizeof(struct tcpiphdr);
                tcp_respond(tp, sk_ack, tp->rcv_nxt, tp->snd_nxt, TH_ACK);

                tp->t_state = TCP_TIME_WAIT;
            }
        }
        return;
    }

    case TCP_CLOSE_WAIT:
        printf("CLOSE_WAIT\r\n");
        tp->t_state = TCP_CLOSED;
    break;

    case TCP_LAST_ACK:
        printf("TCP_LAST_ACK\r\n");

        uint32_t ackno = ntohl(ti->ti_t.th_ack);
        if (SEQ_GT(ackno, tp->snd_una) && SEQ_LEQ(ackno, tp->snd_nxt)) {
            tp->snd_una = ackno;
            tp->t_state = TCP_CLOSED;
            printf("Connection closed gracefully\n");
            // TODO: free control block
        } else {
            struct buf *sk_last_ack = buf_get(0);
            sk_last_ack->data_len = sizeof(struct tcpiphdr);
            tcp_respond(tp, sk, tp->rcv_nxt, tp->snd_nxt, (TH_FIN | TH_ACK));
        }
        return;


    case TCP_FIN_WAIT_2: {
        uint32_t seqno = ntohl(ti->ti_t.th_seq);
        uint32_t ackno = ntohl(ti->ti_t.th_ack);

        if (flags & TH_FIN) {
            if (SEQ_EQ(seqno, tp->rcv_nxt)) {
                tp->rcv_nxt++;

                if (flags & TH_ACK) {
                    if (SEQ_GT(ackno, tp->snd_una) && SEQ_LEQ(ackno, tp->snd_nxt)) {
                        tp->snd_una = ackno;
                    }
                }   

                struct buf *sk_ack = buf_get(0);
                sk_ack->data_len = sizeof(struct tcpiphdr);
                tcp_respond(tp, sk_ack, tp->rcv_nxt, tp->snd_nxt, TH_ACK); 

                tp->t_state = TCP_TIME_WAIT;
            }
        }   
        return;
    }

    default:

    break;
    }

free: 
    buf_free(sk);

}

void tcp_respond(struct tcpcb *tp, struct buf *sk, tcp_seq ack, tcp_seq seq, int flags)
{
    struct tcpiphdr *ti;
    struct _sockaddr_in sa;
    int win = 65535;

    ti = (struct tcpiphdr *)sk->data;

    ti->ti_i.ih_next = ti->ti_i.ih_prev = 0;
    ti->ti_i.ih_x1   = 0;
    ti->ti_i.ih_pr   = IPPROTO_TCP;
    ti->ti_i.ih_len  = htons(sk->data_len - sizeof(struct ip_struct));

    ti->ti_i.ih_dst  = tp->t_inpcb->inp_faddr;
    ti->ti_i.ih_src  = tp->t_inpcb->inp_laddr;

    ti->ti_t.th_dport = tp->t_inpcb->inp_fport;
    ti->ti_t.th_sport = tp->t_inpcb->inp_lport;
    ti->ti_t.th_seq   = htonl(seq);
    ti->ti_t.th_ack   = htonl(ack);
    ti->ti_t.th_x2    = 0;

    if (sk->data_mes_len == 0) {
        ti->ti_t.th_off = (sizeof(struct tcphdr)) >> 2;
    } else {
        ti->ti_t.th_off =
            (sk->data_len - sizeof(struct ip_struct) - sk->data_mes_len) >> 2;
    }

    ti->ti_t.th_flags = flags;
    ti->ti_t.th_win   = htons((unsigned short)win);
    ti->ti_t.th_urp   = 0;
    ti->ti_t.th_sum   = 0;
    ti->ti_t.th_sum   = in_checksum(ti, sk->data_len);

    memset(&sa, 0, sizeof(sa));
    sa.sin_family    = AF_INET;
    sa.sin_addr      = ti->ti_i.ih_dst;
    sa.sin_len       = sizeof(sa);

    sk->type = IPPROTO_TCP;

    ip_output(sk, &sa);
}

void tcp_send_syn(struct tcpcb *tp, struct buf *sk) 
{
    sk->data -= sizeof(struct tcpiphdr); 
    sk->data_len += sizeof(struct tcpiphdr);
    tp->t_state = TCP_SYN_SENT;
    tp->iss = ISS;
    tp->snd_una = ISS;
    tp->snd_nxt = ISS + 1;

    tcp_respond(tp, sk, tp->rcv_nxt, tp->iss, TH_SYN);
}

void tcp_send_fin(struct tcpcb *tp, struct buf *sk) 
{
    printf("send FIN\n");
    tcp_respond(tp, sk, tp->rcv_nxt, tp->snd_nxt, TH_FIN | TH_ACK);
    tp->snd_nxt++;

    tp->t_state = TCP_FIN_WAIT_1;
}

void socket_init()
{
    list_node_init(&InpQue);
} 

#define CWND_SIZE  256
int _socket(int domain, int type, int protocol)
{
    int ret = SocketFd++;
    struct socket *so = heap_malloc(sizeof(struct socket));
    SocketHash[ret] = so;
    
    if (protocol == IPPROTO_TCP) {
        in_pcballoc(so, &TcpInpcb);
    }

    if (protocol == IPPROTO_UDP) {
        in_pcballoc(so, &InpQue);
    }
    
    struct inpcb *inp = so->so_pcb;
    printf("_socket 37 inp is:%p\r\n", inp);

    sem_init(&inp->recv_sem, 0, 0);
    sem_init(&inp->send_sem, 0, 0);
    sem_init(&inp->sem_connected, 0, 0);
    inp->inp_protocol = protocol;

    if (protocol == IPPROTO_TCP) {
        struct tcpcb *tp = heap_malloc(sizeof(struct tcpcb));
        *tp = (struct tcpcb) {
            .t_state = TCP_CLOSED,
            .t_inpcb = inp,
            .snd_cwnd = CWND_SIZE,
        };
        inp->inp_ppcb = (void *)tp;
    }

    return ret;
}

int _bind(int sockfd, const struct _sockaddr *addr, socklen_t addrlen)
{
    struct socket *so = SocketHash[sockfd];
    struct inpcb *inp = so->so_pcb;
    struct _sockaddr_in *s_in = (struct _sockaddr_in *)addr;
    inp->inp_laddr = s_in->sin_addr; 
    inp->inp_lport = s_in->sin_port;
}

int _listen(int sockfd, int backlog)
{   
    struct socket *so = SocketHash[sockfd];
    struct inpcb *inp = so->so_pcb;
    struct tcpcb *tp = inp->inp_ppcb;
    tp->t_state = TCP_LISTEN;

    so->so_q0 = heap_malloc(sizeof(struct list_node));
    list_node_init(so->so_q0);
    so->so_q = heap_malloc(sizeof(struct list_node));
    list_node_init(so->so_q);
}

int _accept(int sockfd, struct _sockaddr *addr, socklen_t *addrlen)
{
    struct socket *listen_so = SocketHash[sockfd];
    if (!listen_so) return -1;
    struct inpcb *listen_pcb = listen_so->so_pcb;
    struct list_node *que = listen_so->so_q;
    if (list_empty(que)) {
        sem_wait(&listen_pcb->sem_connected);
        printf("sem has waited: sem_connected\n");
    }

    struct list_node *node = queue_dequeue(que);
    struct socket *new_so = container_of(node, struct socket, so_list);

    int new_fd = SocketFd++;
    SocketHash[new_fd] = new_so;

    if (addr && addrlen) {
        struct inpcb *inp = new_so->so_pcb;
        struct _sockaddr_in *s_in = (struct _sockaddr_in *)addr;
        s_in->sin_addr = inp->inp_faddr;
        s_in->sin_port = inp->inp_fport;
        *addrlen = sizeof(struct _sockaddr_in);
    }

    return new_fd;
}

int _connect(int sockfd, struct _sockaddr *addr)
{
    struct socket *so = SocketHash[sockfd];
    struct inpcb *inp = so->so_pcb;
    struct _sockaddr_in *s_in = (struct _sockaddr_in *)addr; 
    inp->inp_faddr = s_in->sin_addr;
    inp->inp_fport = s_in->sin_port;

    if (inp->inp_protocol == IPPROTO_TCP) {
        struct buf *sk = buf_get(0);
        sk->type = IPPROTO_TCP;
        tcp_send_syn(inp->inp_ppcb, sk);
    }
    sem_wait(&inp->sem_connected);
     
}

int _recvfrom(int sockfd, char *str, struct _sockaddr *addr)
{  
    struct socket *so = SocketHash[sockfd];
    struct inpcb *inp = so->so_pcb;
    struct _sockaddr_in *socket;
    socket = (struct _sockaddr_in *)addr;

    printf("recvfrom inp:%p\r\n", inp);
    sem_wait(&inp->recv_sem);

    memcpy(str, inp->recv_data, inp->recv_len);
    heap_free(inp->recv_data);

    socket->sin_addr = inp->inp_laddr;
    socket->sin_port = inp->inp_fport;
    printf("inp->recv_len: %d\r\n", inp->recv_len);
    return inp->recv_len;
}

int _sendto(int sockfd, char *str, int len, struct _sockaddr *addr)
{
    struct _sockaddr_in *socket;
    struct buf *sk;
    struct socket *so = SocketHash[sockfd];
    struct inpcb *inp = so->so_pcb;
    struct tcpcb *tp = inp->inp_ppcb;
    uint32_t cwnd_len = tp->snd_cwnd;
    int the_len = 0;

    inp->send_data = str;
    inp->send_len = len;

    socket = (struct _sockaddr_in *)addr;
    printf("sendto!!!\n");

    switch (inp->inp_protocol) {
    case IPPROTO_UDP:
        sk = buf_get(len);
        memcpy(sk->data, inp->send_data, len);
        sk->type = IPPROTO_UDP;
        udp_output(0, sk, addr);

        break;
    case IPPROTO_TCP: 

    while(inp->send_len > 0) {
        int flags = (TH_PUSH | TH_ACK);;
        if (inp->send_len >= cwnd_len) {
            sk = buf_get(cwnd_len);
            the_len = cwnd_len;
            memcpy(sk->data, inp->send_data, cwnd_len);
        } else {
            sk = buf_get(inp->send_len);
            the_len = inp->send_len;
            memcpy(sk->data, inp->send_data, inp->send_len);
        }
        inp->send_data += cwnd_len;
        inp->send_len -= cwnd_len;

        sk->type = IPPROTO_TCP;
        sk->data -= sizeof(struct tcpiphdr); 
        sk->data_len += sizeof(struct tcpiphdr);
        tp->t_state = TCP_ESTABLISHED;
        tcp_respond(tp, sk, tp->rcv_nxt, tp->snd_nxt, flags);
        tp->snd_nxt += the_len;
        if (inp->send_len > 0) {
            sem_wait(&inp->send_sem);
            printf("Get sem!!!\n");
        }
    }

        break;
    case IPPROTO_IPV6:
        break;
    default:
        break;
    }
    return 1;
}

int _shutdown(int sockfd, int how)
{
    struct socket *so = SocketHash[sockfd];
    struct inpcb *inp = so->so_pcb;
    struct buf *sk = buf_get(0);
    sk->type = IPPROTO_TCP;
    sk->data -= sizeof(struct tcpiphdr); 
    sk->data_len += sizeof(struct tcpiphdr);
    
    tcp_send_fin(inp->inp_ppcb, sk);
    return 1;
}

int _close(int sockfd)
{
    return _shutdown(sockfd, SHUT_WR);
}


/*
 * It is all litterTCP!
 * The following is a demo. You can compile and run it in Linux user space, or port it to a specific operating system.
 * If you are porting the code, remember to replace the semaphore operations with the specific operating system's API calls.
*/

//Your code like this:
/*
ifnet_class *net_init()
{
    eth_init();
    arp_init();
    ip_init();
    tcp_init();
    socket_init();

    ifnet_class *ifp = new_ifnet_class("192.168.1.200",
                                       "9e:4d:9e:e3:48:9f",
                                       1500);
    ifnet_register(ifp);                                  

    route_add(inet_addr("192.168.1.0"),
              inet_addr("255.255.255.0"),
              0,
              ifp);

    route_add(0,
              0,
              inet_addr("192.168.1.1"),
              ifp);

    return ifp;
}

void *net_thread(void *arg) 
{
    fd_set readfds;
    ifnet_class *ifp = (ifnet_class *)arg;
    int ret;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(ifp->fd, &readfds);
        ret = select(ifp->fd + 1, &readfds, NULL, NULL, NULL);
        if (ret == -1) {
            perror("select error");
            break;
        } else if (ret > 0 && FD_ISSET(ifp->fd, &readfds)) {
            ether_input(ifp);
        }
    }
}

#define SERVER_IP "192.168.1.200"  
#define SERVER_PORT 1234        
#define BUFFER_SIZE 1024

// TCP Client
void *tcp_thread(void *arg) 
{
    int so_fd = _socket(0, 0,IPPROTO_TCP);

    struct sockaddr_in server_addr = {0}, client_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("192.168.1.1"); 
    server_addr.sin_port = htons(8080); 

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr("192.168.1.200"); 
    client_addr.sin_port = htons(1234); 

    _bind(so_fd, (struct _sockaddr *)&client_addr, 0);
    _connect(so_fd, (struct _sockaddr *)&server_addr);

    char buffer[20] = {0};
    strcpy(buffer, "Hello,linux Server");
    _sendto(so_fd, buffer, sizeof(buffer), (struct _sockaddr *)&server_addr);

    memset(buffer, 0, sizeof(buffer));
    int n = _recvfrom(so_fd, buffer, (struct _sockaddr *)&server_addr);
    if (n < 0) {
        perror("Recvfrom error");
    }
    buffer[n] = '\0'; 
    printf("Received: %s\n", buffer);

    _close(so_fd);  
    while(1) 
    {
        
    }
}


int main() {
    pthread_t thread_id1, thread_id2, thread_id3;

    ifnet_class *ifp = net_init();
    ifp->init("192.168.1.200", "9e:4d:9e:e3:48:9f", 1460);

    if (pthread_create(&thread_id1, NULL, net_thread, ifp) != 0) {
        perror("Failed to create thread1");
        return EXIT_FAILURE;
    }

    if (pthread_create(&thread_id3, NULL, tcp_thread, NULL) != 0) {
        perror("Failed to create thread3");
        return EXIT_FAILURE;
    }

    while(1) {
        
    }
}
*/