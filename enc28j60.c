#include "enc28j60.h"

static uint8_t enc28j60Bank;
static int gNextPacketPtr;
char str2[60] = {};
uint8_t macaddr[6] = MAC_ADDR;

void SPI_ini(void)
{
	DDRB |= (1<<SS) | (1<<MOSI) | (1<<SCK);
	DDRB &= ~(1<<MISO);
	SS_DESELECT();
	SPCR |= (1<<SPE) | (1<<MSTR);	
	SPSR |= (1<<SPI2X);//Double SPI Speed (fosc/2 = 8 MHz)
}

uint8_t SPI_ChangeByte(uint8_t data)
{
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
	
	return SPDR;
}

void SPI_SendByte(uint8_t data)
{
	SPI_ChangeByte(data);
}

uint8_t SPI_ReceiveByte(void)
{
	uint8_t data = SPI_ChangeByte(0xFF);
	
	return data;
}

void enc28j60_writeOp(uint8_t op, uint8_t addres, uint8_t data)
{
	SS_SELECT();
	
	SPI_SendByte(op|(addres & ADDR_MASK));
	SPI_SendByte(data);
	
	SS_DESELECT();
}

static uint8_t enc28j60_readOp(uint8_t op, uint8_t addres)
{
	uint8_t result;
	SS_SELECT();
	SPI_SendByte(op|(addres & ADDR_MASK));
	SPI_SendByte(0x00); //Пропусаем ложный байт
	
	if(addres & 0x80) SPI_ReceiveByte();
	   
	result = SPI_ReceiveByte();
	SS_DESELECT();
	
	return result;
}

static void enc28j60_readBuf(uint16_t len, uint8_t* data) //static uint8_t enc28j60_readBuf
{
	SS_SELECT();
	SPI_SendByte(ENC28J60_READ_BUF_MEM);
	
	while(len--)
	{
		SPI_SendByte(0x00);
		*data++=SPDR;
	}
	SS_DESELECT();
}

static void enc28j60_writeBuf(uint16_t len, uint8_t* data) // static uint8_t enc28j60_writeBuf
{
	SS_SELECT();
	SPI_SendByte(ENC28J60_WRITE_BUF_MEM);
			
	while(len--)
	{
		SPI_SendByte(*data++);
	}
	SS_DESELECT();
}

static void enc28j60_SetBank(uint8_t addres) //static uint8_t enc28j60_SetBank
{
	if((addres & BANK_MASK) != enc28j60Bank){
		enc28j60_writeOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_BSEL1 | ECON1_BSEL0);
		enc28j60Bank = addres & BANK_MASK;
		enc28j60_writeOp(ENC28J60_BIT_FIELD_SET, ECON1, enc28j60Bank >> 5); //enc28j60_writeOp(ENC28J60_BIT_FIELD_CLR, ECON1, enc28j60Bank >> 5);
	}
}

static void enc28j60_writeRegByte(uint8_t addres, uint8_t data)
{
	enc28j60_SetBank(addres);
	enc28j60_writeOp(ENC28J60_WRITE_CTRL_REG, addres, data);
	
}

static uint8_t enc28j60_readRegByte(uint8_t addres)
{
	enc28j60_SetBank(addres);
	return enc28j60_readOp(ENC28J60_READ_CTRL_REG, addres);
}

static void enc28j60_writeReg(uint8_t addres, uint16_t data)
{
	enc28j60_writeRegByte(addres, data);
	enc28j60_writeRegByte(addres+1, data>>8);
}

static void enc28j60_writePhy(uint8_t addres, uint16_t data)
{
	enc28j60_writeRegByte(MIREGADR, addres);
	enc28j60_writeRegByte(MIWR, data);
	while(enc28j60_readRegByte(MISTAT) & MISTAT_BUSY); 
}

void enc28j60_ini(void) 
{
	SPI_ini();
	enc28j60_writeOp(ENC28J60_SOFT_RESET,0,ENC28J60_SOFT_RESET);
	_delay_ms(2);
	
	while(!enc28j60_readOp(ENC28J60_READ_CTRL_REG, ESTAT) & ESTAT_CLKRDY); 
 
	//Настраиваем буферы
	enc28j60_writeReg(ERXST, RXSTART_INIT);
	enc28j60_writeReg(ERXRDPT, RXSTART_INIT);
	enc28j60_writeReg(ERXND, RXSTOP_INIT);
	enc28j60_writeReg(ETXST, TXSTART_INIT);
	enc28j60_writeReg(ETXND, TXSTOP_INIT);
	
	//Enable Broadcast
	enc28j60_writeRegByte(ERXFCON, enc28j60_readRegByte(ERXFCON) | ERXFCON_BCEN);
	
	//настраиваем канальный уровень
	enc28j60_writeRegByte(MACON1, MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);
	enc28j60_writeRegByte(MACON2, 0x00);
	enc28j60_writeOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN);
	enc28j60_writeReg(MAIPG, 0x0C12);
	enc28j60_writeRegByte(MABBIPG, 0x12); //промежуток между фреймами
	enc28j60_writeReg(MAMXFL, MAX_FRAMELEN); //максимальный размер фрейма
	enc28j60_writeRegByte(MAADR5, macaddr[0]); //Set MAC addres
	enc28j60_writeRegByte(MAADR4, macaddr[1]);
	enc28j60_writeRegByte(MAADR3, macaddr[2]);
	enc28j60_writeRegByte(MAADR2, macaddr[3]);
	enc28j60_writeRegByte(MAADR1, macaddr[4]);
	enc28j60_writeRegByte(MAADR0, macaddr[5]);

	//настраиваем физический уровень
	enc28j60_writePhy(PHCON2,PHCON2_HDLDIS); // turning off the loop-back
	enc28j60_writePhy(PHLCON, PHLCON_LACFG3 | PHLCON_LBCFG3 | PHLCON_LBCFG1); //LEDA - display duplex status, LEDB - display transmit and receive activity
	enc28j60_writePhy(PHLCON, 0x476);
	enc28j60_SetBank(ECON1);
	enc28j60_writeOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE | EIE_PKTIE);
	enc28j60_writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN); // allowing to receive data 
	
	enc28j60_writeRegByte(ECOCON,0x03); // Снижаем частоту генератора на самой ENC28J60 в три раза (8.333333Мгц) 
	_delay_us(15);
	sprintf(str2,"ENC28J60 has been initialized\r\n");
	USART_TX((uint8_t*)str2, strlen(str2));
}

uint16_t enc28j60_packetReceive(uint8_t *buf, uint16_t buflen) 
{
	uint16_t len = 0;
	
	if(enc28j60_readRegByte(EPKTCNT)>0) {
		enc28j60_writeReg(ERDPT, gNextPacketPtr);
		
		struct {
			uint16_t nextPacket;
			uint16_t byteCount;
			uint16_t status;
		} header;
		enc28j60_readBuf(sizeof header, (uint8_t*) & header);		
		gNextPacketPtr = header.nextPacket;
		len = header.byteCount - 4; // remove CRC
		if(len > buflen) len = buflen;
		if((header.status & 0x80) == 0) 
		    len = 0;
		else
		    enc28j60_readBuf(len, buf);
		buf[len] = 0;
		if(gNextPacketPtr - 1 > RXSTOP_INIT)
		    enc28j60_writeReg(ERXRDPT, RXSTOP_INIT);
	    else
		    enc28j60_writeReg(ERXRDPT, gNextPacketPtr - 1);
		
		enc28j60_writeOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);	
	}
	return len;
}

void enc28j60_packetSend(uint8_t *buf, uint16_t buflen) 
{
	while(enc28j60_readOp(ENC28J60_READ_CTRL_REG, ECON1) & ECON1_TXRTS)
	{
		if(enc28j60_readRegByte(EIR) & EIR_TXERIF)
		{
			enc28j60_writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
			enc28j60_writeOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);	
		}
		
	}
	enc28j60_writeReg(EWRPT, TXSTART_INIT);
	enc28j60_writeReg(ETXND, TXSTART_INIT + buflen);
	enc28j60_writeBuf(1, (uint8_t*)"\x00");
	enc28j60_writeBuf(buflen, buf);
	enc28j60_writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

//void readREG()
//{
	//uint8_t reg = enc28j60_readRegByte(MAADR0);
	//if(reg == 0x45){
		//PORTC |= (1<<4);
		//_delay_ms(50);
		//PORTC &= ~(1<<4);
	//}
//}