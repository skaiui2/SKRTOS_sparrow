#ifndef MACRO_H
#define MACRO_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <memory.h>
#include <unistd.h> 
 
#include "config.h"

#define class(class)    \
typedef struct class##_class class##_class;\
struct class##_class

#define Class(class)    \
typedef struct  class  class;\
struct class


#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))


#define STRUCT_SIZE_PRE(str, member) offsetof(str, member)

/*
 * If we know a,b both less than the half of number axis.
 * Spill or not,if a is ahead of b, return true.Or else, return flase.
 */
#define COMPARE_GT(a, b) ((int)(a - b) > 0)
#define COMPARE_GET(a, b) ((int)(a - b) >= 0)


#define min(a, b) ((a) < (b) ? (a) : (b))
#define imin(a, b) ((int)(a) < (int)(b) ? (int)(a) : (int)(b))




typedef char err_rt;
typedef int  err_code;

#define GET   2
#define BLOCK 1
#define READY 0
#define ERROR (-1)


#define true    1
#define false   0

#define SYS_ERROR(msg) \
do {    \
    printf("Error: %s | File: %s | Function: %s | Line: %d\n", msg, __FILE__, __func__, __LINE__); \
} while(0)




#define ETH_P_LOOP	0x0060		/* Ethernet Loopback packet	*/
#define ETH_P_PUP	0x0200		/* Xerox PUP packet		*/
#define ETH_P_PUPAT	0x0201		/* Xerox PUP Addr Trans packet	*/
#define ETH_P_TSN	0x22F0		/* TSN (IEEE 1722) packet	*/
#define ETH_P_IP	0x0800		/* Internet Protocol packet	*/
#define ETH_P_X25	0x0805		/* CCITT X.25			*/
#define ETH_P_ARP	0x0806		/* Address Resolution packet	*/
#define	ETH_P_BPQ	0x08FF		/* G8BPQ AX.25 Ethernet Packet	[ NOT AN OFFICIALLY REGISTERED ID ] */
#define ETH_P_IEEEPUP	0x0a00		/* Xerox IEEE802.3 PUP packet */
#define ETH_P_IEEEPUPAT	0x0a01		/* Xerox IEEE802.3 PUP Addr Trans packet */
#define ETH_P_BATMAN	0x4305		/* B.A.T.M.A.N.-Advanced packet [ NOT AN OFFICIALLY REGISTERED ID ] */
#define ETH_P_DEC       0x6000          /* DEC Assigned proto           */
#define ETH_P_DNA_DL    0x6001          /* DEC DNA Dump/Load            */
#define ETH_P_DNA_RC    0x6002          /* DEC DNA Remote Console       */
#define ETH_P_DNA_RT    0x6003          /* DEC DNA Routing              */
#define ETH_P_LAT       0x6004          /* DEC LAT                      */
#define ETH_P_DIAG      0x6005          /* DEC Diagnostics              */
#define ETH_P_CUST      0x6006          /* DEC Customer use             */
#define ETH_P_SCA       0x6007          /* DEC Systems Comms Arch       */
#define ETH_P_TEB	0x6558		/* Trans Ether Bridging		*/
#define ETH_P_RARP      0x8035		/* Reverse Addr Res packet	*/
#define ETH_P_ATALK	0x809B		/* Appletalk DDP		*/
#define ETH_P_AARP	0x80F3		/* Appletalk AARP		*/
#define ETH_P_8021Q	0x8100          /* 802.1Q VLAN Extended Header  */
#define ETH_P_ERSPAN	0x88BE		/* ERSPAN type II		*/
#define ETH_P_IPX	0x8137		/* IPX over DIX			*/
#define ETH_P_IPV6	0x86DD		/* IPv6 over bluebook		*/
#define ETH_P_PAUSE	0x8808		/* IEEE Pause frames. See 802.3 31B */



/* Ethernet protocol ID's */
#define	ETHERTYPE_PUP		0x0200          /* Xerox PUP */
#define ETHERTYPE_SPRITE	0x0500		/* Sprite */
#define	ETHERTYPE_IP		0x0800		/* IP */
#define	ETHERTYPE_ARP		0x0806		/* Address resolution */
#define	ETHERTYPE_REVARP	0x8035		/* Reverse ARP */
#define ETHERTYPE_AT		0x809B		/* AppleTalk protocol */
#define ETHERTYPE_AARP		0x80F3		/* AppleTalk ARP */
#define	ETHERTYPE_VLAN		0x8100		/* IEEE 802.1Q VLAN tagging */
#define ETHERTYPE_IPX		0x8137		/* IPX */
#define	ETHERTYPE_IPV6		0x86dd		/* IP protocol version 6 */
#define ETHERTYPE_LOOPBACK	0x9000		/* used to test interfaces */



/* ARP protocol HARDWARE identifiers. */
#define ARPHRD_NETROM	0		/* From KA9Q: NET/ROM pseudo. */
#define ARPHRD_ETHER 	1		/* Ethernet 10/100Mbps.  */
#define	ARPHRD_EETHER	2		/* Experimental Ethernet.  */
#define	ARPHRD_AX25	3		/* AX.25 Level 2.  */
#define	ARPHRD_PRONET	4		/* PROnet token ring.  */
#define	ARPHRD_CHAOS	5		/* Chaosnet.  */
#define	ARPHRD_IEEE802	6		/* IEEE 802.2 Ethernet/TR/TB.  */
#define	ARPHRD_ARCNET	7		/* ARCnet.  */
#define	ARPHRD_APPLETLK	8		/* APPLEtalk.  */
#define	ARPHRD_DLCI	15		/* Frame Relay DLCI.  */
#define	ARPHRD_ATM	19		/* ATM.  */
#define	ARPHRD_METRICOM	23		/* Metricom STRIP (new IANA id).  */
#define ARPHRD_IEEE1394	24		/* IEEE 1394 IPv4 - RFC 2734.  */
#define ARPHRD_EUI64		27		/* EUI-64.  */
#define ARPHRD_INFINIBAND	32		/* InfiniBand.  */



/* ARP protocol opcodes. */
#define	ARPOP_REQUEST	1		/* ARP request.  */
#define	ARPOP_REPLY	2		/* ARP reply.  */
#define	ARPOP_RREQUEST	3		/* RARP request.  */
#define	ARPOP_RREPLY	4		/* RARP reply.  */
#define	ARPOP_InREQUEST	8		/* InARP request.  */
#define	ARPOP_InREPLY	9		/* InARP reply.  */
#define	ARPOP_NAK	10		/* (ATM)ARP NAK.  */



#define ICMP_ECHOREPLY		0	/* Echo Reply			*/
#define ICMP_DEST_UNREACH	3	/* Destination Unreachable	*/
#define ICMP_SOURCE_QUENCH	4	/* Source Quench		*/
#define ICMP_REDIRECT		5	/* Redirect (change route)	*/
#define ICMP_ECHO		8	/* Echo Request			*/
#define ICMP_TIME_EXCEEDED	11	/* Time Exceeded		*/
#define ICMP_PARAMETERPROB	12	/* Parameter Problem		*/
#define ICMP_TIMESTAMP		13	/* Timestamp Request		*/
#define ICMP_TIMESTAMPREPLY	14	/* Timestamp Reply		*/
#define ICMP_INFO_REQUEST	15	/* Information Request		*/
#define ICMP_INFO_REPLY		16	/* Information Reply		*/
#define ICMP_ADDRESS		17	/* Address Mask Request		*/
#define ICMP_ADDRESSREPLY	18	/* Address Mask Reply		*/
#define NR_ICMP_TYPES		18

/*
#define AF_UNSPEC      0   // Unspecified protocol family, used for generic sockets or unspecified addresses
#define AF_INET        2   // IPv4 protocol family, used for IPv4 network communication
#define AF_INET6       10  // IPv6 protocol family, used for IPv6 network communication
#define AF_UNIX        1   // UNIX local socket, used for inter-process communication on the same machine
#define AF_LOCAL       1   // Alias for AF_UNIX, represents local sockets
#define AF_PACKET      17  // Direct access to the link-layer packets, used for low-level networking
#define AF_NETLINK     16  // Communication between kernel and user-space processes
#define AF_BLUETOOTH   31  // Bluetooth protocol family, used for Bluetooth device communication


#define IPPROTO_IP      0x00  // IPv4 协议
#define IPPROTO_ICMP    0x01  // ICMP (Internet Control Message Protocol)
#define IPPROTO_IGMP    0x02  // IGMP (Internet Group Management Protocol)
#define IPPROTO_TCP     0x06  // TCP (Transmission Control Protocol)
#define IPPROTO_UDP     0x11  // UDP (User Datagram Protocol)
#define IPPROTO_IPV6    0x29  // IPv6 协议
#define IPPROTO_GRE     0x2F  // GRE (Generic Routing Encapsulation)
#define IPPROTO_ESP     0x32  // ESP (Encapsulating Security Payload)
#define IPPROTO_AH      0x33  // AH (Authentication Header)
#define IPPROTO_SCTP    0x84  // SCTP (Stream Control Transmission Protocol)
#define IPPROTO_RAW     0xFF  // RAW 数据包协议

*/


#endif 