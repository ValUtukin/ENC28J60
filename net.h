#ifndef NET_H_
#define NET_H_

#include <string.h>
#include "enc28j60.h"
#include "usart.h"

void init_timer(void);
void net_ini(void);
void net_pool(void);

#define IP_ADDR ip_join (192, 168, 0, 190)
#define MAC_BROADCAST {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}
#define MAC_NULL {0x00,0x00,0x00,0x00,0x00,0x00}

typedef struct enc28j60_frame {
	uint8_t addr_dest[6];
	uint8_t addr_src[6];
	uint16_t type;
	uint8_t data[];
	} enc28j60_frame_ptr;
	
typedef struct arp_msg {
	uint16_t net_tp;
	uint16_t proto_tp;
	uint8_t macaddr_len;
	uint8_t ipaddr_len;
	uint16_t op;
	uint8_t macaddr_src[6];
	uint32_t ipaddr_src;
	uint8_t macaddr_dst[6];
	uint32_t ipaddr_dst;
	} arp_msg_ptr;
	
typedef struct ip_pkt {
	uint8_t verlen; // версия протокола и длина заголовка
	uint8_t ts; // тип сервиса
	uint16_t len; // длина
	uint16_t id; // идентифекатор пакета
	uint16_t fl_frg_of; // флаги и смещение фрагмента
	uint8_t ttl; // время жизни
	uint8_t prt; // тип протокола
	uint16_t cs; // контрольная сумма заголовка
	uint32_t ipaddr_src; // IP-адрес отправителя
	uint32_t ipaddr_dst; // IP-адрес получателя
	uint8_t data[]; // данные
} ip_pkt_ptr;

typedef struct icmp_pkt {
	uint8_t msg_tp;
	uint8_t msg_cd;
	uint16_t cs;
	uint16_t id;
	uint16_t num;
	uint8_t data[];
	} icmp_pkt_ptr;
	
#define ip_join(a,b,c,d) ( ((uint32_t)a) | ((uint32_t)b<<8) | ((uint32_t)c<<16) | ((uint32_t)d<<24) )

#define ARP_ETH be16toword(0x0001)
#define ARP_IP be16toword(0x0800)
#define ARP_REQUEST be16toword(1)
#define ARP_REPLY be16toword(2)

#define be16toword(a)  ((((a)>>8) & 0xFF) | (((a)<<8) & 0xFF00))
#define be32todword(a) ((((a)>>24)&0xff)|(((a)>>8)&0xff00)|(((a)<<8)&0xff0000)|(((a)<<24)&0xff000000))
#define ETH_ARP be16toword(0x0806)
#define ETH_IP be16toword(0x0800)

#define IP_ICMP 1
#define IP_TCP  6
#define IP_UDP  17

#define ICMP_REQ   8
#define ICMP_REPLY 0

void eth_send(enc28j60_frame_ptr *frame, uint16_t type, uint16_t len);
uint16_t checksum(uint8_t *ptr, uint16_t len, uint8_t type);

#include "arp.h"
#include "tcp.h"
#endif 