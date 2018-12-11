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
#include "leds.h"
#define write_control PORTD
#define write_pin PORTD2
#define ro_mode() SET_BIT(write_control, write_pin)
#define wr_mode() CLEAR_BIT(write_control, write_pin)


#define slaveaddr = 0x78;
// Uncomment this to print out debugging statements.
//#define DEBUG 1

// used for system time
volatile uint64_t ms_ticks = 0;

// mutex access to ms_ticks
volatile uint8_t in_ui_mode = 0;


/****************************************************************************
   ms timer using TIMER0 (TIMSK0)
****************************************************************************/
void setupMStimer(void) {
    TCCR0A |= (1 << WGM01); // set CTC mode (clear-timer-on-compare)
    TCCR0B |= (1 << CS00) | (1 << CS01);
    OCR0A = 249;
    TIMSK0 |= (1 << OCIE0A);
}


uint16_t bytes_in_buffer;
uint8_t* buffer;





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
/****************************************************************************
   ALL INITIALIZATION
****************************************************************************/
void initialize_system(void) {
    initialize_leds(); // set up the leds
    SetupHardware(); // usb communication
    initialize_fram();
    setupMStimer(); // the ms_ticks timer0
}

void setup_slave_io() {
  // PORTD 6 is the ready pin
  DDRD |= (1<<DDD6);
  // Set the pin to not ready
  PORTD &= ~(1<<PORTD6);
}

// This is the buffer that is received/sent from/to the master
uint8_t* byte_buffer;
uint16_t* buffer_length;

void setup_salve_twi() {
  TWAR = (slaveaddr<<1);
  TWCR = (1<<TWEN) | (1<<TWEA);
  byte_buffer = (uint8_t*)malloc(0);
  buffer_length = (uint16_t*)malloc(sizeof(uint16_t));
}



/****************************************************************************
   MAIN
****************************************************************************/
char c;
int main(void) {

    //  USBCON = 0;
    initialize_system(); // initialization of system
    ms_ticks = 0; // initialize 'tick' counter to 0
    sei();
    setup_slave_io();
    setup_slave_twi();
    while(1) {
      // Set the flag that the slave is ready
      // That flag is set on PORTD6
      PORTD |= (1<<PORTD6);


      // Send the buffer to master
      int result = send_to_master(byte_buffer,buffer_length);
      if(result == -1) {
        // the buffer was not sent to the master successfully so it should
        // try again at the start of the loop
        continue;
      }

      // Now that it was sent to master we don't need the buffer
      free(byte_buffer);
      *buffer_length = 0;


      byte_buffer = receive_bytes_from_master(buffer_length);
      if(buffer_length == 0) {
        free(byte_buffer);
        // unable to receive bytes from master so we should just try again
        continue;
      }

      // Do work with the new buffer
    }
}

/****************************************************************************
   ISR for TIMER used as SCHEDULER
****************************************************************************/
// Timer set up in timers.c always enables COMPA
ISR(TIMER0_COMPA_vect) {
    ms_ticks++;
}
