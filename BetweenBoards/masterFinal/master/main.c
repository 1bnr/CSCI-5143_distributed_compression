#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#ifdef VIRTUAL_SERIAL
#include <VirtualSerial.h>
#else
#warning VirtualSerial not defined, USB IO may not work
#define SetupHardware()       ;
#define USB_Mainloop_Handler();
#endif



// library includes
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include "i2c_master.h"

#include <compat/twi.h>

#include "lufa.h"
#include "common.h"
#define write_control PORTD
#define write_pin     PORTD2
#define ro_mode() SET_BIT(write_control, write_pin)
#define wr_mode() CLEAR_BIT(write_control, write_pin)

#define slave1addr    0x60;
#define slave2addr    0x61;


// Uncomment this to print out debugging statements.
//#define DEBUG 1

// used for system time
volatile uint64_t ms_ticks = 0;

// mutex access to ms_ticks
volatile uint8_t in_ui_mode = 0;

uint16_t *bytes_received;
uint16_t blocks = 5;
uint16_t blocks_sent = 0;
uint16_t block_size = 256;

/****************************************************************************
   ms timer using TIMER0 (TIMSK0)
****************************************************************************/
void setupMStimer(void) {
    TCCR0A |= (1 << WGM01); // set CTC mode (clear-timer-on-compare)
    TCCR0B |= (1 << CS00) | (1 << CS01);
    OCR0A = 249;
    TIMSK0 |= (1 << OCIE0A);
}

void initialize_fram() {
    // initialize power supply portD pin4 astar pin 4
    DDRD |= (1 << DDD2);
    DDRD |= (1 << DDD4);
    DDRD |= (1 << DDD6);
    DDRD |= (1 << DDD7);
    PORTD |= (1 << PORTD2);
    PORTD |= (1 << PORTD4);
    PORTD |= (1 << PORTD6);
    PORTD |= (1 << PORTD7);

    // address pins on PB4,PB5,PB6 (astar 8,9,10)
    // sda PD1 (astar 2)
    // SCL PD0 (astar 3)
    i2c_init();     // initialize I2C library
}

void setup_slave_polling() {
    // Slave one B5 A*9
    // Slave two B6 A*10
    DDRB &= ~(1 << DDB5);
    DDRB &= ~(1 << DDB6);
}

/****************************************************************************
   ALL INITIALIZATION
****************************************************************************/
void initialize_system(void) {
    SetupHardware(); // usb communication
    initialize_fram();
}

/****************************************************************************
   MAIN
****************************************************************************/
char c;
int main(void) {
    uint8_t *fram = (uint8_t *)malloc(32);

    block_size = 2;
    blocks = 16;
    blocks_sent = 0;

    //  USBCON = 0;
    bytes_received = (uint16_t *)malloc(sizeof(uint16_t));
    *bytes_received = 0;
    initialize_system(); // initialization of system
    sei();
    // it needs to read in all of the data
    // and send it to the ram chip.  this was done by pieter
    setup_slave_polling();
    uint8_t curr_addr = 0x00;

    // While there is more data to process
    while (blocks_sent < blocks) {
        // First check if slave1 is available to process
        if ((PORTB & (1 << PORTB5)) == (1 << PORTB5)) {
            curr_addr = slave1addr;
        } else if ((PORTB & (1 << PORTB6)) == (1 << PORTB6)) {
            curr_addr = slave2addr;
        } else {
            curr_addr = 0x00;
        }

        if (curr_addr != 0x00) {
            // This means that one of the boards was ready
            // First receive the bytes from the board specified by curr_addr
            *bytes_received = 0;
            uint8_t *buffer = receive_bytes_from_slave(curr_addr, bytes_received);

            if (*bytes_received == 0) {
                // This means that the slave has not received any data and so we don't save this result
                free(buffer);
            } else {
                // send result somewhere probably to a second ram chip
                int i;

                for (i = 0; i < *bytes_received; i++) {
                    printf("buffer[%d]=%hhx\r\n", i, buffer[i]);
                }

                free(buffer);
            }

            // Get the memory location to send the board equal to blocks_sent
            uint16_t init_mem = blocks_sent * block_size;
//            uint8_t *buffer = read_from_ram(rammaddr, init_mem, block_size);
            buffer =  (uint8_t*) malloc(sizeof(uint8_t)*2);
            buffer[0] = fram[init_mem];
            buffer[1] = fram[init_mem+1];

            int result = send_bytes_to_slave(curr_addr, buffer, block_size);

            if (result != -1) {
                free(buffer);
                blocks_sent++;
            } else {
                curr_addr = 0x00;
                free(buffer);
            }
        }
    }

    // Now all the data has been sent and received so send it back to the computer
}

/****************************************************************************
   ISR for TIMER used as SCHEDULER
****************************************************************************/
// Timer set up in timers.c always enables COMPA
ISR(TIMER0_COMPA_vect) {
    ms_ticks++;
}
