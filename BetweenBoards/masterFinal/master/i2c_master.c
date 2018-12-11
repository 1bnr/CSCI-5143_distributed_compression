#ifndef  F_CPU
#define F_CPU 16000000UL
#endif

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

uint8_t i2c_start(uint8_t address) {
    // reset TWI control register
    TWCR = 0;
    // transmit START condition
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

    // wait for end of transmission
    while (!(TWCR & (1 << TWINT)) );

    // check if the start condition was successfully transmitted
    if ((TWSR & 0xF8) != TW_START) {
        printf("Error: start condition transmission did not succeed\r\n");
        return 1;
    }
    printf("Start sent\r\n");
    // send device address
    TWDR = address;
    printf("AddrSent:%hhx\r\n",TWDR);
    TWCR = (1 << TWINT) | (1 << TWEN);

    // wail until transmission completed and ACK/NACK has been received
    while (!(TWCR & (1 << TWINT))) {
      USB_Mainloop_Handler();
      printf("Waiting for ack from the device\r\n");
    }

    // check value of TWI Status Register. Mask prescaler bits.
    if ( ((TWSR & 0xF8) != TW_MT_SLA_ACK) && ((TWSR & 0xF8) != TW_MR_SLA_ACK) ) {
        printf("Error: i2c_start failed acknowledment\r\n");
        printf("TWSR & 0xF8 : %04x\r\nTW_MT_DATA_ACK : %04x\r\n", (TWSR & 0xF8), TW_MT_DATA_ACK);
        return 1;
    }

    return 0;
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
    while (!(TWCR & (1 << TWINT))) {
      USB_Mainloop_Handler();
      printf("Waiting for write to finish sending\r\n");
    }

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

uint8_t i2c_writeReg(uint8_t devaddr, uint16_t regaddr, uint8_t *data, uint16_t length) {
    i2c_init();

    if (i2c_start((devaddr << 1) | I2C_READ)) {
        printf("i2c_start error\r\n");
        return 1;
    }


    uint8_t val =0;
    uint8_t val2 = 5;
    i2c_write(val);
    i2c_write(val2);
    i2c_write('h');
    i2c_write('e');
    i2c_write('l');
    i2c_write('l');
    i2c_write('o');

    while(1) {
      USB_Mainloop_Handler();
      printf("Start Signal Sent\r\n");
    }
    i2c_stop();

    return 0;
}

uint8_t* i2c_readReg(uint8_t devaddr, uint16_t regaddr, uint8_t *data, uint16_t *length) {
    // start device connection
    i2c_init();

    if (i2c_start((devaddr << 1) | I2C_READ)) {
        printf("Error: readReg::start(devaddr)\r\n");
    }

    uint8_t one = i2c_read_ack();
    uint8_t two = i2c_read_ack();
    uint16_t length2 = (one << 8) | two;
    uint8_t *buffer = (uint8_t*) malloc(length2);
    uint8_t data_in;
    // now read, with acknowledment
    for (int i = 0; i < length2; i++) {
      printf("On byte:%d\r\n",i);
        data_in = i2c_read_ack();
        printf("With data:%hhx\r\n", data_in);
        buffer[i] = data_in;
        printf("Data[%d]=%hhx\r\n",i,buffer[i]);
    }

    // read last byte without acknowledgment
    //data[(length - 1)] = i2c_read_nack();

    //i2c_stop();
    *length = length2;
    return buffer;
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
