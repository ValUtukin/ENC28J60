#include "usart.h"

USART_prop_ptr usartprop;

void USART_Ini(unsigned int speed)
{
	//----------USART SPEED INIT------------
	UBRR0H = (unsigned char)(speed>>8); 
	UBRR0L = (unsigned char) speed;
	
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);// Разрешаем прием и передачу
	UCSR0B |= (1<<RXCIE0);// Разрешаем прерывание при передаче
	UCSR0A |= (1<<U2X0);// Удвоение частоты
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
	
	usartprop.usart_buf[0]=0;
	usartprop.usart_cnt=0;
	usartprop.is_ip=0;
}

void USART_Transmit(unsigned char data)
{
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}

void USART_TX(uint8_t* str1, uint8_t cnt)
{
	uint8_t i;
	for(i=0;i<cnt;i++)
	USART_Transmit(str1[i]);
}

ISR(USART_RX_vect)
{
	uint8_t b;
	b = UDR0;
	//если вдруг случайно превысим длину буфера
	if (usartprop.usart_cnt>20)
	{
		usartprop.usart_cnt=0;
	}
	else if (b == 'a')
	{
		usartprop.is_ip=1;//статус отрпавки ARP-запроса
	}
	else
	{
		usartprop.usart_buf[usartprop.usart_cnt] = b;
		usartprop.usart_cnt++;
	}
}