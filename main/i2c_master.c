#ifndef  F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/twi.h>
#include <stdio.h>
//#include <stdlib.h>
#include "i2c_master.h"

#define FRAM_W 0xA1
#define FRAM_R 0xA0

#define F_SCL 100000UL // SCL frequency
#define Prescaler 1
#define TWBR_val ((((F_CPU / F_SCL) / Prescaler) - 16 ) / 2)

void i2c_init(void)
{
	TWBR = (uint8_t)TWBR_val;
}

uint8_t i2c_start(uint8_t address)
{
	// reset TWI control register
	TWCR = 0;
	// transmit START condition
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	// wait for end of transmission
	while( !(TWCR & (1<<TWINT)) );

	// check if the start condition was successfully transmitted
	if((TWSR & 0xF8) != TW_START){
		printf("Error: start condition transmission did not succeed\r\n");
		return 1; }

	// load slave address into data register
//	TWDR = (address<<1);
	// start transmission of address
//	TWCR = (1<<TWINT) | (1<<TWEN);
	// wait for end of transmission
//	while( !(TWCR & (1<<TWINT)) );

	// check if the device has acknowledged the READ / WRITE mode
	//uint8_t twst = TWSR & 0xF8;
	//  if ( (twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK)){
	//	printf("Error: device has not acknowledged READ / WRITE mode\r\n");
	//	return 1;}

	return 0;
}

uint8_t i2c_write(uint8_t data)
{
	// load data into data register
	TWDR = data;
	// start transmission of data
	printf("starting write\r\n");
	TWCR = (1<<TWINT) | (1<<TWEN);
	// wait for end of transmission
	while( !(TWCR & (1<<TWINT)) );
	printf("write completed\r\n");

	if( (TWSR & 0xF8) != TW_MT_DATA_ACK )  {
		printf("write acknowledment failed\r\nTW_MT_DATA_ACK: %04x\r\n(TWSR & 0xF8): %04x\r\n",TW_MT_DATA_ACK,(TWSR & 0xF8));
		return 1; } // TW_MT_DATA_ACK: 11100 TWSR:11110

	return 0;
}

uint8_t i2c_read_ack(void)
{

	// start TWI module and acknowledge data after reception
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	// wait for end of transmission
	while( !(TWCR & (1<<TWINT)) );
	// return received data from TWDR
	return TWDR;
}

uint8_t i2c_read_nack(void)
{

	// start receiving without acknowledging reception
	TWCR = (1<<TWINT) | (1<<TWEN);
	// wait for end of transmission
	while( !(TWCR & (1<<TWINT)) );
	// return received data from TWDR
	return TWDR;
}

uint8_t i2c_transmit(uint8_t address, uint8_t* data, uint16_t length)
{
	if (i2c_start(address | I2C_WRITE)) return 1;

	for (uint16_t i = 0; i < length; i++)
	{
		if (i2c_write(data[i])) return 1;
	}

	i2c_stop();

	return 0;
}

uint8_t i2c_receive(uint8_t address, uint8_t* data, uint16_t length)
{
	if (i2c_start(address | I2C_READ)) return 1;

	for (uint16_t i = 0; i < (length-1); i++)
	{
		data[i] = i2c_read_ack();
	}
	data[(length-1)] = i2c_read_nack();

	i2c_stop();

	return 0;
}

uint8_t i2c_writeReg(uint8_t devaddr, uint8_t regaddr, uint8_t* data, uint16_t length)
{
	if (i2c_start(FRAM_W)) {
		printf("i2c_start error\r\n");
		return 1;}
		// load slave address into data register
		TWDR = (regaddr);
		// start transmission of address
		TWCR = (1<<TWINT) | (1<<TWEN);
		// wait for end of transmission
		while( !(TWCR & (1<<TWINT)) );
	/* first write is the destinatino address */
	i2c_write((regaddr<<1) | 1);
	printf("address sent\r\n");
	/* now we can start writting data */
	for (uint16_t i = 0; i < length; i++)
	{
		printf("writing data[%d]: %c\r\n",i , (char)data[i] );
		if (i2c_write(data[i])) {
			printf("i2c_write error\r\n");
			return 1;}
	}

	i2c_stop();

	return 0;
}

uint8_t i2c_readReg(uint8_t devaddr, uint8_t regaddr, uint8_t* data, uint16_t length)
{
	/* start device connection */
	if (i2c_start(FRAM_R)) {
		printf("Error: readReg::start(devaddr)\r\n");
		return 1;}
	/* write the source address */
	i2c_write(regaddr);

	if (i2c_start(FRAM_R)) {
		printf("Error in readingReg::start(devaddr | 0x01)\r\n");
		return 1;}

	for (uint16_t i = 0; i < (length-1); i++)
	{
		data[i] = i2c_read_nack();
	}
	data[(length-1)] = i2c_read_nack();

	i2c_stop();

	return 0;
}

void i2c_stop(void)
{
	// transmit STOP condition
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
}
