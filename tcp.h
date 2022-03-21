#ifndef TCP_H_

#define TCP_H_
#include "main.h"
#include "enc28j60.h"
#include "net.h"
#include "usart.h"

typedef struct tcp_pkt{
	uint16_t port_src;
	uint16_t port_dst;
	uint32_t bt_num_seg;
	uint32_t num_ask;
	uint8_t len_hdr;
	uint8_t fl;
	uint16_t size_wnd;
	uint16_t cs;
	uint16_t urg_ptr;
	uint8_t data[];
}tcp_pkt_ptr;

#define LOCAL_PORT_TCP 80

#define TCP_CWR 0x80
#define TCP_ECE 0x40
#define TCP_URG 0x20
#define TCP_ACK 0x10
#define TCP_PSH 0x08
#define TCP_RST 0x04
#define TCP_SYN 0x02
#define TCP_FIN 0x01

#define TCP_OP_SYNACK 1
#define TCP_OP_ACK_OF_FIN 2
#define TCP_OP_ACK_OF_RST 3
#define TCP_OP_ACK_OF_DATA 4

uint8_t tcp_read(enc28j60_frame_ptr *frame, uint16_t len);
uint8_t tcp_send(uint32_t ip_addr, uint16_t port, uint8_t op);
#endif /* TCP_H_ */