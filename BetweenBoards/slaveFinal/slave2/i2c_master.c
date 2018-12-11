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

    // send device address
    TWDR = address;
    TWCR = (1 << TWINT) | (1 << TWEN);

    // wail until transmission completed and ACK/NACK has been received
    while (!(TWCR & (1 << TWINT)));

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

uint8_t i2c_writeReg(uint8_t devaddr, uint16_t regaddr, uint8_t *data, uint16_t length) {
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
    printf("address sent\r\n");

    // now we can start writting data
    int len = strlen((char *)data);
    printf("%s: len: %d\r\n", (char *)data, len);

    for (uint16_t i = 0; i < len; i++) {
        printf("writing data[%d]: %c\r\n", i, (char)data[i]);

        if (i2c_write(data[i])) {
            printf("i2c_write error\r\n");
            return 1;
        }
    }

    i2c_stop();

    return 0;
}

uint8_t i2c_readReg(uint8_t devaddr, uint16_t regaddr, uint8_t *data, uint16_t length) {
    // start device connection
    i2c_init();

    if (i2c_start((devaddr << 1)) | I2C_WRITE) {
        printf("Error: readReg::start(devaddr)\r\n");
        return 1;
    }

    // write the source address
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

int send_bytes_to_master(uint8_t *buffer, uint16_t *bytes_to_send) {
    while ((TWCR & (1 << TWINT)) != (1 << TWINT)) {
        USB_Mainloop_Handler();
        //printf("Waiting\r\n");
        // This is just a busy wait for the master to send the start signal
        // when the TWINT is set in TWCR it means there is a valid value in
        // the TWSR
    }
    // send the ack back

    // send master the number of bytes to read
    if (TWSR == 0xA8) {
        TWDR = ((*bytes_to_send) >> 8);
        TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);

        while ((TWCR & (1 << TWINT)) != (1 << TWINT)) {
            USB_Mainloop_Handler();
            printf("Waiting for master to receive byte\r\n");
        }
    } else {
        printf("Failed to send first byte TWSR:%hhx\r\n", TWSR);
        return -1;
    }

    // send the second byte
    if (TWSR == 0xB8) {
        TWDR = *bytes_to_send;
        TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);

        while ((TWCR & (1 << TWINT)) != (1 << TWINT)) {
            USB_Mainloop_Handler();
            printf("Waiting for master to receive byte\r\n");
        }
    } else {
        printf("Failed to send second byte TWSR:%hhx\r\n", TWSR);
        return -1;
    }

    if (TWSR == 0xB8) {
        // This means that it is in the correct state
        // send back the ack
        TWDR = buffer[0];
        printf("SendingFirstBytes:%hhx\r\n", TWDR);
        printf("SendingFirstBytes:%hhx\r\n", buffer[0]);
        TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);
        int i;

        for (i = 1; i < bytes_to_send - 1; i++) {
            printf("sending byte:%d\r\n", i);

            while ((TWCR & (1 << TWINT)) != (1 << TWINT))
                USB_Mainloop_Handler();

            if (TWSR == 0xB8) {
                // This means that the last byte was received and is ready to send the next
                // load TWDR
                TWDR = buffer[i];
                printf("SendingNextBytes:%hhx\r\n", TWDR);
                printf("SendingNextBytes:%hhx\r\n", buffer[i]);
                // Send the signal to master to receive the next byte
                TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);
            } else {
                // This is in a state I don't understand
                printf("Error with sending a byte state\r\n");
                printf("TWSR:%hhx\r\n", TWSR);
                return -1;
            }
        } // end of for loop

        while ((TWCR & (1 << TWINT)) != (1 << TWINT))
            USB_Mainloop_Handler();

        if (TWSR == 0xB8) {
            // This means that the last byte was received and is ready to send the next
            // load TWDR
            TWDR = buffer[i];
            printf("SendingNextBytes:%hhx\r\n", TWDR);
            printf("SendingNextBytes:%hhx\r\n", buffer[i]);
            // Send the signal to master to receive the next byte
            TWCR = (1 << TWINT) | (1 << TWEN);
        } else {
            // This is in a state I don't understand
            printf("Error with sending a byte state\r\n");
            printf("TWSR:%hhx\r\n", TWSR);
            return -1;
        }

        while ((TWCR & (1 << TWINT)) != (1 << TWINT)) {
        }

        if (TWSR == 0xC8) {
            TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);
            return 0;
        } else {
            printf("somthing went wrong with last byte sent\r\n");
            return -1;
        }
    } else {
        // This was in a state that is not sending so return -1 to indicate
        // and error;
        printf("Error with first state\r\n");
        printf("TWSR:%hhx\r\n", TWSR);
        return -1;
    }
}

uint8_t * receive_bytes_from_master(uint16_t *buffer_length) {
    // Wait for the start signal
    while ((TWCR & (1 << TWINT)) != (1 << TWINT)) {
        USB_Mainloop_Handler();
        printf("Waiting\r\n");
        // This is just a busy wait for the master to send the start signal
        // when the TWINT is set in TWCR it means there is a valid value in
        // the TWSR
    }

    if (TWSR == 0x60) {
        // Send back the ack by setting twint and tew
        TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);

        while ((TWCR & (1 << TWINT)) != (1 << TWINT)) {
            USB_Mainloop_Handler();
            printf("Waiting for byte 1\r\n");
        }
        uint8_t one = 0;
        uint8_t two = 0;

        if (TWSR == 0x80) {
            // This means that there is a byte to read in the twdr
            one = TWDR;
        } else {
          printf("Error: first byte not received correctly.\r\n" );
          return (uint8_t*) malloc(0);
        }

        // Now send back the ack
        TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);

        // Now wait for byte two
        while ((TWCR & (1 << TWINT)) != (1 << TWINT)) {
            USB_Mainloop_Handler();
            printf("Waiting for byte 2\r\n");
        }

        // second byte
        if (TWSR == 0x80) {
            two = TWDR;
        } else {
          printf("Error: second byte not received correctly.\r\n" );
          return (uint8_t*) malloc(0);
        }

        // Let master know that we got the bytes
        TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN); // sending ack
        *buffer_length = (one << 8) | two;
        buffer = (uint8_t *) malloc(*buffer_length);
        int i;

        for (i = 0; i < read_size; i++) {
            // Wait for the byte to be snt
            while ((TWCR & (1 << TWINT)) != (1 << TWINT)) {
                USB_Mainloop_Handler();
                printf("Waiting for data bytes\r\n");
            }
            if (TWSR == 0x80) {
              buffer[i] =  TWDR;
              TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN); // sending ack
            } else {
              *buffer_length = 0;
              printf("Error receiving data bytes\r\n");
              free(buffer);
              buffer = (uint8_t *) malloc(0);
            }
        }
        // wait for stop signal
        while ((TWCR & (1 << TWINT)) != (1 << TWINT)) {
            USB_Mainloop_Handler();
            printf("Waiting for stop signal\r\n");
        }
        if (TWSR == 0xA0) {
          printf("transmission from master successful\r\n");
          return buffer;
        } else {
          printf("Error: stop not received\r\n");
          *buffer_length = 0;
          free(buffer);
          buffer = (uint8_t *) malloc(0);
        }
    } else {
      printf("Error: not in slave receiver mode\r\n");
      return (uint8_t*) malloc(0);
    }
}
