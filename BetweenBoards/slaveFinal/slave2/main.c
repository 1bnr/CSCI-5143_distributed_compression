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
#define write_pin PORTD2
#define ro_mode() SET_BIT(write_control, write_pin)
#define wr_mode() CLEAR_BIT(write_control, write_pin)


#define slaveaddr 0x60
// Uncomment this to print out debugging statements.
//#define DEBUG 1


uint16_t bytes_in_buffer;
uint8_t* buffer;

/****************************************************************************
   ALL INITIALIZATION
****************************************************************************/
void initialize_system(void) {
    SetupHardware(); // usb communication
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

void setup_slave_twi() {
  TWAR = (slaveaddr<<1);
  TWCR = (1<<TWEN) | (1<<TWEA);
  byte_buffer = (uint8_t*)malloc(0);
  buffer_length = (uint16_t*)malloc(sizeof(uint16_t));
  *buffer_length = 0;
}



/****************************************************************************
   MAIN
****************************************************************************/
int main(void) {

    //  USBCON = 0;
    initialize_system(); // initialization of system
    sei();
    setup_slave_io();
    setup_slave_twi();
    while(1) {
      // Set the flag that the slave is ready
      // That flag is set on PORTD6
      PORTD |= (1<<PORTD6);


      // Send the buffer to master
      int result = send_bytes_to_master(byte_buffer,buffer_length);
      if(result != 0) {
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
      int i;
      for (i=0;i<*buffer_length;i++){
        printf("buffer[%d]=%hhx\r\n", i, byte_buffer[i]);
      }

      // Do work with the new buffer
    }
}
