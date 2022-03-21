#ifndef DS18B20_H_
#define DS18B20_H_

#include "main.h"

#define NOID 0xCC //Пропустить идентификацию
#define T_CONVERT 0x44 //Код измерения температуры
#define READ_DATA 0xBE //Передача байтов ведущему

#define PORTTEMP PORTC
#define DDRTEMP DDRC
#define PINTEMP PINC
#define BITTEMP 5

char dt_testdevice(void);
void dt_sendbit(char bt);
void dt_sendbyte(unsigned char bt);
char dt_readbit(void);
unsigned char dt_readbyte(void);
int dt_check(void);
char converttemp (unsigned int tt);

#endif /* DS18B20_H_ */