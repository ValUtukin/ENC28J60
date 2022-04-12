#include "main.h"

void port_ini(void)
{
	//Включим ножку INT0(PD2) на вход
	DDRD &= ~(0b00000100);
	//Информационная ножка 1-Wire
	PORTC=0x00;
	DDRC=0xFF;
}

void int_ini(void)
{
	//включим прерывания INT0 по нисходящему фронту
	EICRA |= (1<<ISC01);
	//разрешим внешнее прерывание INT0
	EIMSK |= (1<<INT0);
}

ISR(INT0_vect)
{
	net_pool();
}

int main(void)
{
	//--------------GLOBAL INTERRUPT ENABLE---------------
	sei();
	
	//--------------TIMER INIT-----------------
	init_timer();
	
	//--------------INTERRUPT INIT-------------------
	port_ini();
	int_ini();
	
	// -------------LED DEBUGGING INIT------------------
	PORTC &= ~((1<<5)|(1<<4));
	DDRC |= (1<<5)|(1<<4);
	
	// -------------CODE FOR USART USING----------------
	USART_Ini(103);
	//sprintf(str1,"USART's working!\r\n");
	//USART_TX((uint8_t*)str1, strlen(str1));
	
	// -------------RUN ENC28J60---------------
	net_ini();
	
	while (1)
	{	
		// --------------CHECKING ENC28J60 BUFFER FOR PACKAGES--------------
		//net_pool();
		
		// --------------INIT DEBUGGING BY WATCHING ENC28J60 REGISTERS-----------
		//PORTC |= (1<<5);
		//readREG();
		//_delay_ms(20);
		//PORTC &= ~(1<<5);
		PORTC |= (1<<4);
		_delay_ms(5000);
		PORTC &= ~(1<<4);
		_delay_ms(1000);
	}
}

