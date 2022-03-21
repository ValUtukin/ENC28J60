#include "arp.h"

extern char str1[60];
extern unsigned int tim_cnt;//счетчик тиков таймера
extern uint32_t clock_cnt;//счетчик секунд
extern uint8_t net_buf[ENC28J60_MAXFRAME];
extern uint8_t macaddr[6];
uint8_t macbroadcast[6]=MAC_BROADCAST;
uint8_t macnull[6]=MAC_NULL;
arp_record_ptr arp_rec[5];
uint8_t current_arp_index=0;

uint8_t arp_read(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res = 0;
	arp_msg_ptr *msg = (void*)(frame->data);
	if(len>=sizeof(arp_msg_ptr))
	{
		if((msg->net_tp==ARP_ETH)&&(msg->proto_tp==ARP_IP))
		{
			if((msg->ipaddr_dst==IP_ADDR))
			{
				if(msg->op==ARP_REQUEST)
				{
					res = 1;
				}
				else if (msg->op==ARP_REPLY)
				{
					res = 2;
				}
			}
		}
	}
	return res;
}

void arp_send(enc28j60_frame_ptr *frame)
{
	arp_msg_ptr *msg = (void*)frame->data;
	msg->op = ARP_REPLY;
	memcpy(msg->macaddr_dst,msg->macaddr_src,6);
	memcpy(msg->macaddr_src,macaddr,6);
	msg->ipaddr_dst = msg->ipaddr_src;
	msg->ipaddr_src = IP_ADDR;
	eth_send(frame, ETH_ARP, sizeof(arp_msg_ptr));
}

uint8_t arp_request(uint32_t ip_addr)
{
	uint8_t i;
	
	//проверим, может такой адрес уже есть в таблице ARP, а заодно и удалим оттуда просроченные записи
	for(i=0;i<5;i++)
	{
		//Если записи уже более 10 секунд, то удалим её
		if ((clock_cnt-arp_rec[i].sec)>10)
		{
			memset(arp_rec+(sizeof(arp_record_ptr)*i),0,sizeof(arp_record_ptr));
		}
		
		if (arp_rec[i].ipaddr==ip_addr)
		{
			//смотрим ARP-таблицу
			for(i=0;i<5;i++)
			{
				sprintf(str1,"%ld.%ld.%ld.%ld - %02X:%02X:%02X:%02X:%02X:%02X - %lu\r\n",
				arp_rec[i].ipaddr & 0x000000FF,(arp_rec[i].ipaddr>>8) & 0x000000FF,
				(arp_rec[i].ipaddr>>16) & 0x000000FF, arp_rec[i].ipaddr>>24,
				arp_rec[i].macaddr[0],arp_rec[i].macaddr[1],arp_rec[i].macaddr[2],
				arp_rec[i].macaddr[3],arp_rec[i].macaddr[4],arp_rec[i].macaddr[5],
				arp_rec[i].sec);
				USART_TX((uint8_t*)str1,strlen(str1));
			}
			return 0;
		}
	}
	
	enc28j60_frame_ptr *frame=(void*)net_buf;
	arp_msg_ptr *msg = (void*)frame->data;
	msg->net_tp = ARP_ETH;
	msg->proto_tp = ARP_IP;
	msg->macaddr_len = 6;
	msg->ipaddr_len = 4;
	msg->op = ARP_REQUEST;
	memcpy(msg->macaddr_src,macaddr,6);
	msg->ipaddr_src = IP_ADDR;
	memcpy(msg->macaddr_dst,macnull,6);
	msg->ipaddr_dst = ip_addr;
	memcpy(frame->addr_dest,macbroadcast,6);
	memcpy(frame->addr_src,macaddr,6);
	frame->type = ETH_ARP;
	enc28j60_packetSend((void*)frame,sizeof(arp_msg_ptr) + sizeof(enc28j60_frame_ptr));
	return 1;
}

void arp_table_fill(enc28j60_frame_ptr *frame)
{
	uint8_t i;
	arp_msg_ptr *msg = (void*)frame->data;
	
	//Добавим запись
	arp_rec[current_arp_index].ipaddr=msg->ipaddr_src;
	memcpy(arp_rec[current_arp_index].macaddr,msg->macaddr_src,6);
	arp_rec[current_arp_index].sec = clock_cnt;
	if(current_arp_index<4) current_arp_index++;
	else current_arp_index=0;
	for(i=0;i<5;i++)
	{
		sprintf(str1,"%ld.%ld.%ld.%ld - %02X:%02X:%02X:%02X:%02X:%02X - %lu\r\n",
		arp_rec[i].ipaddr & 0x000000FF,(arp_rec[i].ipaddr>>8) & 0x000000FF,
		(arp_rec[i].ipaddr>>16) & 0x000000FF, arp_rec[i].ipaddr>>24,
		arp_rec[i].macaddr[0],arp_rec[i].macaddr[1],arp_rec[i].macaddr[2],
		arp_rec[i].macaddr[3],arp_rec[i].macaddr[4],arp_rec[i].macaddr[5],
		arp_rec[i].sec);
		USART_TX((uint8_t*)str1,strlen(str1));
	}
}