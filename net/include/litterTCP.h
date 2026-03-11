#ifndef LITTERTCP_H
#define LITTERTCP_H
#include "macro.h"
#include "ipc.h"
#include "port.h"
#include "link_list.h"

struct _sockaddr {
    unsigned char sa_len;
    unsigned char sa_family;
    char   sa_data[14];
}__attribute__((packed));

struct _sockaddr_in {
	unsigned char	    sin_len;
	unsigned char	    sin_family;
	unsigned short	    sin_port;
	struct	_in_addr   	sin_addr;
	char    	        sin_zero[8];
};

struct rtentry {
    uint32_t dest;      
    uint32_t netmask;   
    uint32_t gateway; 
    struct ifnet_class *ifp;  
};


struct eth_hdr {
    unsigned char   ether_dhost[6];
    unsigned char   ether_shost[6];
    unsigned short  ether_type;
}__attribute__((packed));



struct arp_hdr {
    unsigned short ar_hrd;
    unsigned short ar_pro;
    unsigned char  ar_hln;
    unsigned char  ar_pln;
    unsigned short ar_op;
}__attribute__((packed));

struct arp_ether {
    struct arp_hdr ea_hdr;
    unsigned char arp_sha[6];
    unsigned int arp_spa;
    unsigned char arp_tha[6];
    unsigned int arp_tpa;
}__attribute__((packed));


struct arp_cache {
    struct list_node node;
    unsigned int  ipaddr;
    unsigned char hwaddr[6];
};



#define IPVERSION	4

struct ip_struct {
    /*LITTLE_ENDIAN!!!*/
    unsigned char   ip_hl:4;
    unsigned char   ip_v:4;
    unsigned char   ip_tos;
    short           ip_len;
    unsigned short  ip_id;
    short           ip_off;
    unsigned char   ip_ttl;
    unsigned char   ip_p;
    unsigned short  ip_sum;
    struct _in_addr  ip_src;
    struct _in_addr  ip_dst; 

}__attribute__((packed));


struct icmp {
    unsigned char   icmp_type;
    unsigned char   icmp_code;
    unsigned short  icmp_cksum;

    union {
        unsigned char   ih_pptr;
        struct _in_addr  addr;   
        struct ih_idseq {
            unsigned short  icmp_id;
            unsigned short  icmp_seq;
        }idseq;

        struct ih_pmtu {
            short   ipm_void;
            short   ipm_nextmtu;
        }pmtu;

    }icmp_hun;
    
}__attribute__((packed));



struct ipovly {
	unsigned int 	ih_next;
    unsigned int   	ih_prev;	
	unsigned char	ih_x1;		
	unsigned char	ih_pr;		
	short	ih_len;			
	struct	_in_addr ih_src;		
	struct	_in_addr ih_dst;	
}__attribute__((packed));

struct udphdr {
	unsigned short	uh_sport;	
	unsigned short	uh_dport;		
	unsigned short	uh_ulen;		
	unsigned short	uh_sum;			
}__attribute__((packed));


struct	udpiphdr {
	struct 	ipovly ui_i;		
	struct	udphdr ui_u;		
}__attribute__((packed));


struct tcphdr {
    unsigned short  th_sport;
    unsigned short  th_dport;
    unsigned int   th_seq;
    unsigned int   th_ack;
    //little ENDIAN!!!
    unsigned char   th_x2:4;
    unsigned char   th_off:4;
    unsigned char   th_flags;
    unsigned short  th_win;
    unsigned short  th_sum;
    unsigned short  th_urp;
}__attribute__((packed));




struct tcpiphdr {
    struct ipovly   ti_i;
    struct tcphdr   ti_t;
}__attribute__((packed));


struct inpcb {
    struct list_node node;
	struct	_in_addr inp_faddr;	
	unsigned short	inp_fport;		
	struct	_in_addr inp_laddr;	
	unsigned short	inp_lport;
	struct	buf *sk;
	ipc_sem_t sem_connected;
    ipc_sem_t recv_sem;
	void  *recv_data;
	int   recv_len;
	ipc_sem_t send_sem;
	void  *send_data;
	int   send_len;
	int inp_protocol;
	void  *inp_ppcb;

	struct	socket *inp_socket;
    struct  _sockaddr_in sa_dst;
	int		inp_flags;		
	struct	ip_struct inp_ip;		
	struct	ip_moptions *inp_moptions; 
};


typedef	unsigned int	tcp_seq;
struct tcpcb {
    struct list_node node;
	short	t_state;		/* state of this connection */
	unsigned short	t_flags;

	struct	inpcb *t_inpcb;		/* back pointer to internet pcb */
/*
 * The following fields are used as in the protocol specification.
 * See RFC783, Dec. 1981, page 21.
 */
/* send sequence variables */
	tcp_seq	snd_una;		/* send unacknowledged */
	tcp_seq	snd_nxt;		/* send next */
	tcp_seq	iss;			/* initial send sequence number */
	unsigned int	snd_wnd;		/* send window */
/* receive sequence variables */
	unsigned int	rcv_wnd;		/* receive window */
	tcp_seq	rcv_nxt;		/* receive next */

/* congestion control (for slow start, source quench, retransmit after loss) */
	unsigned int	snd_cwnd;		/* congestion-controlled window */
	unsigned int	snd_ssthresh;	
};



#define TCP_CLOSED         0
#define TCP_LISTEN         1
#define TCP_SYN_SENT       2
#define TCP_SYN_RECEIVED   3
#define TCP_ESTABLISHED    4
#define TCP_FIN_WAIT_1     5
#define TCP_FIN_WAIT_2     6
#define TCP_CLOSE_WAIT     7
#define TCP_CLOSING        8
#define TCP_LAST_ACK       9
#define TCP_TIME_WAIT      10



#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PUSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20


struct socket {
	struct list_node so_list;
	short	so_type;		/* generic type, see socket.h */
	short	so_state;		/* internal state flags SS_*, below */
	void	*so_pcb;			/* protocol control block */
	struct	protosw *so_proto;	/* protocol handle */
	struct	socket *so_head;	/* back pointer to accept socket */
	struct	list_node *so_q0;		/* queue of partial connections */
	struct	list_node *so_q;		/* queue of incoming connections */
	short	so_q0len;		/* partials on so_q0 */
	short	so_qlen;		/* number of connections on so_q */
	short	so_qlimit;		/* max number queued connections */
	short	so_timeo;		/* connection timeout */
	unsigned short	so_error;		/* error affecting connection */
	pid_t	so_pgid;		/* pgid for signals */

	struct	sockbuf {
		unsigned long	sb_cc;		/* actual chars in buffer */
		unsigned long	sb_hiwat;	/* max actual char count */
		unsigned long	sb_mbcnt;	/* chars of mbufs used */
		unsigned long	sb_mbmax;	/* max chars of bufs to use */
		long	sb_lowat;	/* low water mark */
		struct	buf *sb_b;	/* the buf chain */
		short	sb_flags;	/* flags, see below */
		short	sb_timeo;	/* timeout for read/write */
	} so_rcv, so_snd;
};





#endif 
