#include "net.h"

char str1[60] = {};
unsigned int tim_cnt=0;//счетчик тиков таймера
uint32_t clock_cnt=0;//счетчик секунд	
uint8_t net_buf[ENC28J60_MAXFRAME];
extern uint8_t macaddr[6];
extern USART_prop_ptr usartprop;

void init_timer(void)
{
	TCCR0A |= (1<<WGM01); //режим устанавливаем режим СТС (сброс по совпадению)
	OCR0A = 0xFF; //записываем в регистр число для сравнения
	TIMSK0 |= (1<<OCIE0A); //устанавливаем бит разрешения прерывания 0-ого счетчика по совпадению с OCR0A
	TCCR0B |= (0<<CS02)|(1<<CS01)|(1<<CS00); // устанавливаем предделитель 64
	//тем самым получаем - частота тактирования / предделитель / 256 = 976,5625 (около милисекунды)
}

ISR (TIMER0_COMPA_vect)
{
	tim_cnt++;
	//считаем секунды и записываем их в clock_cnt
	if(tim_cnt>=1000)
	{
		tim_cnt=0;
		clock_cnt++;
	}
}

void net_ini(void)
{
	enc28j60_ini();
}

void eth_send(enc28j60_frame_ptr *frame, uint16_t type, uint16_t len)
{
	memcpy(frame->addr_dest, frame->addr_src, 6);
	memcpy(frame->addr_src, macaddr, 6);
	frame->type=type;
	enc28j60_packetSend((void*)frame, len + sizeof(enc28j60_frame_ptr));
}

uint16_t checksum(uint8_t *ptr, uint16_t len, uint8_t type)
{
	uint32_t sum = 0;
	if(type==2)
	{
		sum+=IP_TCP;
		sum+=len-8;
	}
	while(len > 1)
	{
		sum += (uint16_t)(((uint32_t) *ptr << 8) | *(ptr + 1));
		ptr += 2;
		len -= 2;
	}
	if(len) sum+=((uint32_t) *ptr)<<8;
	while(sum>>16) sum=(uint16_t)sum+(sum >> 16);
	
	return ~be16toword((uint16_t)sum);
}


uint8_t ip_send(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res = 0;
	ip_pkt_ptr *ip_pkt = (void*) frame -> data;
	ip_pkt -> len = be16toword(len);
	ip_pkt -> fl_frg_of = 0;
	ip_pkt -> ttl = 128;
	ip_pkt -> cs = 0;
	ip_pkt -> ipaddr_dst = ip_pkt -> ipaddr_src;
	ip_pkt -> ipaddr_src = IP_ADDR;
	ip_pkt -> cs = checksum((void*)ip_pkt, sizeof(ip_pkt_ptr),0);
	eth_send(frame, ETH_IP, len);
	return res;
}
uint8_t icmp_read(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res = 0;
	ip_pkt_ptr *ip_pkt = (void*) frame -> data;
	icmp_pkt_ptr *icmp_pkt = (void*) ip_pkt -> data;
	// отфильтруем пакет по длине и типу сообщения - эхо-запрос 
	if((len >= sizeof(icmp_pkt_ptr)) && (icmp_pkt -> msg_tp == ICMP_REQ))
	{
		icmp_pkt -> msg_tp = ICMP_REPLY;
		icmp_pkt -> cs = 0;
		icmp_pkt -> cs = checksum((void*) icmp_pkt, len,0);
		ip_send(frame, len + sizeof(ip_pkt_ptr));
	}
	return res;
}

uint8_t ip_read(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res = 0;
	ip_pkt_ptr *ip_pkt = (void*)(frame -> data);
	if((ip_pkt->verlen==0x45)&&(ip_pkt->ipaddr_dst==IP_ADDR))
	{
		//длина данных
		len = be16toword(ip_pkt -> len) - sizeof(ip_pkt_ptr);
		if(ip_pkt -> prt == IP_ICMP)
		{
			icmp_read(frame, len);
		}
		else if(ip_pkt->prt==IP_TCP)
		{
			tcp_read(frame, len);
		}
	}
	return res;
}

uint32_t ip_extract(char* ip_str, uint8_t len)
{
	uint32_t ip=0;
	uint8_t offset = 0;
	uint8_t i;
	char ss2[5] = {0};
	char *ss1;
	int ch='.';
	for(i=0;i<3;i++)
	{
		ss1 = strchr(ip_str,ch);
		offset = ss1-ip_str+1;
		strncpy(ss2,ip_str,offset);
		ss2[offset]=0;
		ip|=((uint32_t)atoi(ss2))<<(i*8);
		ip_str+=offset;
		len-=offset;
	}
	strncpy(ss2,ip_str,len);
	ss2[len]=0;
	ip|=((uint32_t)atoi(ss2))<<24;
	return ip;
}

void eth_read(enc28j60_frame_ptr *frame, uint16_t len)
{
	uint8_t res = 0;
	if(len >= sizeof(enc28j60_frame_ptr))
	{
		if(frame->type == ETH_ARP) 
		{
			res = arp_read(frame, len - sizeof(enc28j60_frame_ptr));
			if(res == 1)
			{
				arp_send(frame);
			}
			else if(res == 2)
			{
				arp_table_fill(frame);
			}
		}
		 else if(frame->type == ETH_IP) 
		{
			ip_read(frame, len - sizeof(ip_pkt_ptr));
		}
	}
}

void net_pool(void)
{
 	uint16_t len;
	uint32_t ip=0;
	enc28j60_frame_ptr *frame = (void*)net_buf;
	sprintf(str1,"Hello Ethernet\r\n");
	USART_TX((uint8_t*)str1, strlen(str1));
	while((len=enc28j60_packetReceive(net_buf, sizeof(net_buf))))
	{
		eth_read(frame, len); 
	}
	if (usartprop.is_ip==1) //статус отправки ARP-запроса
	{
		USART_TX(usartprop.usart_buf,usartprop.usart_cnt);
		USART_TX((uint8_t*)"\r\n",2);
		ip=ip_extract((char*)usartprop.usart_buf,usartprop.usart_cnt);
		sprintf(str1,"%lu",ip);
		USART_TX((uint8_t*)str1, strlen(str1));
		USART_TX((uint8_t*)"\r\n",2);
		arp_request(ip);
		usartprop.is_ip = 0;
		usartprop.usart_cnt=0;
	}
}
