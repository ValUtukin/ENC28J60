#include "ds18b20.h"

char dt_testdevice(void) //������� ����������� ������� �� ����
{
	char stektemp=SREG;// �������� �������� �����
	cli(); //��������� ����������
	char dt;
	
	DDRTEMP |= 1<<BITTEMP; //����������� ����
	_delay_us(485); //�������� ��� ������� �� 480 �����������
	DDRTEMP &= ~(1<<BITTEMP); //��������� ����
	_delay_us(65); //�������� ��� ������� �� 60 �����������
	if((PINTEMP & (1<<BITTEMP))==0)//���������, ������� �� ����������
	{
		PORTD |= (1<<4);
		dt=1;//���������� ����
	}
	else dt=0;//���������� ���
	
	SREG = stektemp;// ������ �������� �����
	_delay_us(420); //�������� ��� ������� �� 480 �����������, �� ������ � 420, �� ��� � ������ ������� ��������� ������
	return dt; //������ ���������
}

void dt_sendbit(char bt)
{
	char stektemp=SREG;// �������� �������� �����
	cli(); //��������� ����������
	DDRTEMP |= 1<<BITTEMP; //����������� ����
	_delay_us(2); //�������� ��� ������� �� 2 ������������
	if(bt)
	DDRTEMP &= ~(1<<BITTEMP); //��������� ����
	_delay_us(65); //�������� ��� ������� �� 60 �����������
	DDRTEMP &= ~(1<<BITTEMP); //��������� ����
	SREG = stektemp;// ������ �������� �����
}

void dt_sendbyte(unsigned char bt)//������� ������ ����� �� ����������
{
	char i;
	for(i=0;i<8;i++)//�������� �������� ������ ��� �� ����������
	{
		if((bt & (1<<i)) == 1<<i)//�������� 1
		{
			dt_sendbit(1);
			} else {//�������� 0
			dt_sendbit(0);
		}
	}
}

char dt_readbit(void)//������� ������ ���� � ����������
{
	char stektemp=SREG;// �������� �������� �����
	cli(); //��������� ����������
	char bt; //���������� �������� ����
	
	DDRTEMP |= 1<<BITTEMP; //����������� ����
	_delay_us(2); //�������� ��� ������� �� 2 ������������
	DDRTEMP &= ~(1<<BITTEMP); //��������� ����
	_delay_us(13);
	bt = (PINTEMP & (1<<BITTEMP))>>BITTEMP; //������ ���
	_delay_us(45);
	SREG = stektemp;// ������ �������� �����
	return bt; //������ ���������
}


unsigned char dt_readbyte(void)//������� ������ ����� � ����������
{
	char c=0;
	char i;

	for(i=0;i<8;i++)
	c|=dt_readbit()<<i; //������ ���

	return c;
}

int dt_check(void) //������� �������������� ��������� ������� � �����������
{
	uint8_t bt;//���������� ��� ���������� �����
	int tt=0;
	if(dt_testdevice()==1) //���� ���������� �������
	{
		dt_sendbyte(NOID);//���������� �������������, �� � ��� ������ ���� ���������� �� ����
		dt_sendbyte(T_CONVERT);//�������� �����������
		_delay_ms(750);//� 12������ ������ �������������� � 750 ����������
		
		dt_testdevice(); //����� ���������� �� �� ����������� � ����� ��� � ��� �������� �� �����������
		dt_sendbyte(NOID); //���������� �������������, �� � ��� ������ ���� ���������� �� ����
		dt_sendbyte(READ_DATA); //���� ������� �� ������ ������ � ����������
		
		bt = dt_readbyte(); //������ ������� ���
		tt = dt_readbyte(); //������ ������� ��� MS
		tt = (short)(tt<<8)|bt;//�������� ������� �����, ������� ����� �� ��� �����, ��� ����� �������� ����� ���������
	}
	return tt;
}

float converttemp (int tt)//�������������� ����������� � �������
{
	float result = 0x00;
	result = (float) tt / 16.0f;
	sei();
	return result;
}
