/*
   based on the widely used i2c_master library written by Peter Fleury, but
   I had to make alterations to suit the needs of this project.
   http://homepage.hispeed.ch/peterfleury/doxygen/avr-gcc-libraries/group__pfleury__ic2master.html

 */

#ifndef  F_CPU
#define F_CPU 16000000UL
#endif

#include <util/delay.h>
#include <avr/io.h>
#include <util/twi.h>
#include <stdio.h>
#include <string.h>
//#include <stdlib.h>
#include "i2c_master.h"

#define F_SCL     100000UL // SCL frequency
#define Prescaler 1
#define TWBR_val  ((((F_CPU / F_SCL) / Prescaler) - 16) / 2)

void i2c_init(void) {
  TWBR = (uint8_t)TWBR_val;
}
/******************************************************************************
    A start with a timeout is used to auto sense clear addresses; slave starts
    in Master mode, begins trying addresses starting with the default 0x40, then
    incrementing every time it recives a response until a request timesout.

    A timeout of 0 is the same as no timeout.
*******************************************************************************/
uint8_t i2c_start_timeout(uint8_t address, uint8_t ms_timeout) {
  // reset TWI control register
  TWCR = 0;
  // transmit START condition
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

  // wait for end of transmission
  while (!(TWCR & (1 << TWINT)));

  // check if the start condition was successfully transmitted
  if ((TWSR & 0xF8) != TW_START) {
    printf("Error: start condition transmission did not succeed\r\n");
    return 1;
  }

  // send device address
  TWDR = address;
  TWCR = (1 << TWINT) | (1 << TWEN);

  // wail until transmission completed and ACK/NACK has been received or timeout occurs
  if (ms_timeout > 0){
      // will timeout after ms_timeout miliseconds
      while ((ms_timeout--) && (!(TWCR & (1 << TWINT)))) _delay_ms(1);
  } else // no timeout; wait until response is received
    while (!(TWCR & (1 << TWINT)));

  // check value of TWI Status Register. Mask prescaler bits.
  if ( ((TWSR & 0xF8) != TW_MT_SLA_ACK) && ((TWSR & 0xF8) != TW_MR_SLA_ACK) ) {
    //printf("Error: i2c_start failed acknowledment\r\n");
    return 1; // will occur if timed out waiting for ack/nack
  }

  return 0;
}


uint8_t i2c_start(uint8_t address){
  return i2c_start_timeout(address, 0); // no timeout
}

uint8_t i2c_start_wait(uint8_t address) {
  while (1) {
    // send START condition
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

    // wait until transmission completed
    while (!(TWCR & (1 << TWINT)));

    // check value of TWI Status Register. Mask prescaler bits.
    if ( ((TWSR & 0xF8) != TW_START) && ((TWSR & 0xF8) != TW_REP_START)) continue;

    // send device address
    TWDR = address;
    TWCR = (1 << TWINT) | (1 << TWEN);

    // wail until transmission completed
    while (!(TWCR & (1 << TWINT)));

    // check value of TWI Status Register. Mask prescaler bits.
    if ( ((TWSR & 0xF8) == TW_MT_SLA_NACK) || ((TWSR & 0xF8) == TW_MR_DATA_NACK) ) {
      /* device busy, send stop condition to terminate write operation */
      TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);

      // wait until stop condition is executed and bus released
      while (TWCR & (1 << TWSTO));

      continue;
    }

    if ( (TWSR & 0xF8) != TW_MT_SLA_ACK) return 1;

    break;
  }
  return 0;
}/* i2c_start_wait */

uint8_t i2c_read_ack(void) {
  // start TWI module and acknowledge data after reception
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

  // wait for end of transmission
  while (!(TWCR & (1 << TWINT)) );
  // return received data from TWDR
  return TWDR;
}

/*************************************************************************
   Send one byte to I2C device

   Input:    byte to be transfered
   Return:   0 write successful
      1 write failed
*************************************************************************/
uint8_t i2c_write(uint8_t data) {
  // send data to the previously addressed device
  TWDR = data;
  TWCR = (1 << TWINT) | (1 << TWEN);

  // wait until transmission completed
  while (!(TWCR & (1 << TWINT)));

  // check value of TWI Status Register. Mask prescaler bits

  if ( (TWSR & 0xF8) != TW_MT_DATA_ACK) {
    printf("TWSR & 0xF8 : %04x\r\nTW_MT_DATA_ACK : %04x\r\n", (TWSR & 0xF8), TW_MT_DATA_ACK);
    return 1;
  }

  return 0;
}/* i2c_write */

uint8_t i2c_read_nack(void) {
  // start receiving without acknowledging reception
  TWCR = (1 << TWINT) | (1 << TWEN);

  // wait for end of transmission
  while (!(TWCR & (1 << TWINT)) );
  // return received data from TWDR
  return TWDR;
}

uint8_t i2c_transmit(uint8_t address, uint8_t *data, uint16_t length) {
  if (i2c_start(address | I2C_WRITE)) return 1;

  for (uint16_t i = 0; i < length; i++) {
    if (i2c_write(data[i])) return 1;
  }

  i2c_stop();

  return 0;
}

uint8_t i2c_receive(uint8_t address, uint8_t *data, uint16_t length) {
  if (i2c_start(address | I2C_READ)) return 1;

  for (uint16_t i = 0; i < (length - 1); i++) {
    data[i] = i2c_read_ack();
  }

  data[(length - 1)] = i2c_read_nack();

  i2c_stop();

  return 0;
}

uint8_t i2c_write_start(uint8_t devaddr, uint16_t regaddr)
{
  i2c_init();

  if (i2c_start((devaddr << 1) | I2C_WRITE)) {
    printf("i2c_start error\r\n");
    return 1;
  }

  // wait for end of transmission
  while (!(TWCR & (1 << TWINT)) );
  // first write is the destinatino address
  i2c_write(regaddr >> 8);
  i2c_write(regaddr & 0xFF);

  return 0;
}
// write data to fram over i2c
uint8_t i2c_writeReg(uint8_t devaddr, uint16_t regaddr, uint8_t *data, uint16_t length) {
  if (i2c_write_start(devaddr, regaddr)) {
    printf("write_start failed");
    return 1;
  }
  // write the data to the fram one byte at a time
  for (uint16_t i = 0; i < length; i++) {
    if (i2c_write(data[i])) {
      printf("i2c_write error\r\n");
      return 1;
    }
  }

  i2c_stop();

  return 0;
}

// read length bytes starting at regaddr, one byte at a time over i2c bus
uint8_t i2c_readReg(uint8_t devaddr, uint16_t regaddr, uint8_t *data, uint16_t length) {
  // start device connection
  i2c_init();

  if (i2c_start((devaddr << 1)) | I2C_WRITE) {
    printf("Error: readReg::start(devaddr)\r\n");
    return 1;
  }

  // write the source address to read from
  i2c_write(regaddr >> 8);
  i2c_write(regaddr & 0xFF);

  // start again in read mode
  if (i2c_start((devaddr << 1) | I2C_READ)) {
    printf("Error in readingReg::start(devaddr | 0x%02x)\r\n", I2C_READ);
    return 1;
  }

  // now read, with acknowledment
  for (uint16_t i = 0; i < (length - 1); i++) {
    data[i] = i2c_read_ack();
  }

  // read last byte without acknowledgment
  data[(length - 1)] = i2c_read_nack();

  i2c_stop();

  return 0;
}

void format_storage(uint8_t devaddr){
  uint32_t zeros = 0; // four bytes
  for (int i=0; i< 8000; i++) {
    i2c_writeReg( devaddr, i * 4,(uint8_t*) &zeros, 4);
  }
// add blank file struct data; first half input, second half output
  uint16_t metadata[4] = {8,8,16000,16000};
  i2c_writeReg(devaddr, 0, (uint8_t*) metadata, 8);
}


/*************************************************************************
   Issues a repeated start condition and sends address and transfer direction

   Input:   address and transfer direction of I2C device

   Return:  0 device accessible
    1 failed to access device
*************************************************************************/
unsigned char i2c_rep_start(unsigned char address) {
  return i2c_start(address);
}/* i2c_rep_start */

/*************************************************************************
   Terminates the data transfer and releases the I2C bus
*************************************************************************/
void i2c_stop(void) {
  /* send stop condition */
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);

  // wait until stop condition is executed and bus released
  while (TWCR & (1 << TWSTO));
}/* i2c_stop */
