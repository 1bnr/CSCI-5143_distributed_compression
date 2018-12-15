#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define F_SCL     100000UL // SCL frequency
// library includes
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <compat/twi.h>
#include "i2c_master.h"
#include "i2c_slave.h"
#include "common.h"
#include "buttons.h"
#include "leds.h"



extern void SetupHardware(void);
extern void USB_Mainloop_Handler(void);

// Uncomment this to print out debugging statements.
//#define DEBUG 1

// the i2c address may be changed by the auto_address() function
volatile uint8_t workerID = I2C_SLAVE_BASE_ID;
/*
  auto sense used addresses on i2c bus by temporarily becoming master and
  polling devices until a free address is found, then switch to slave mode
*/
void auto_address() {
  i2c_slave_stop(); // clears control register
  USB_Mainloop_Handler();
  i2c_init();
  // i2c_start_timeout returns 0 if workerID is alredy on bus;
  // wait for timeout and an error response to an address not present
  while (!i2c_start_timeout(((workerID << 1) | I2C_READ), 10)){
    printf("address %hhx in use; trying next address.\r\n", workerID);
    workerID++;
  }
  printf("address %hhx is clear, setting up slave\r\n", workerID );
  USB_Mainloop_Handler();
  i2c_stop();
  i2c_slave_init(workerID);
}

// this is interrupt launched function to set button A status flags
void A_release() {
  busy_light();
  auto_address();
  ready_light();
}
// send power to i2c bus; astar pin8
void initialize_i2c(){
    DDRB |= (1 << DDB4);
    PORTB |= (1 << PORTB4);
}


/****************************************************************************
   ALL INITIALIZATION
****************************************************************************/
void initialize_system(void) {
  initialize_i2c();
  initialize_leds(); // set up the leds
  SetupHardware(); // usb communication
  // setup the buttons
  initialize_buttons();
  SetUpButton(&_button_A);
  SetUpButtonAction(&_button_A, 1, A_release);

  light_show();
}

/****************************************************************************
   MAIN
****************************************************************************/
int main(void) {
  // master will poll status prior to sending/receiving data
  volatile uint8_t status = 0; // 0 receive data, 1 busy, 2 send data
  USBCON = 0;
  initialize_system(); // initialization of system
  sei();
  //*******   THE CYCLIC CONTROL LOOP      **********//
  //*************************************************************//
  for (;;) {
    USB_Mainloop_Handler();
  }
}
