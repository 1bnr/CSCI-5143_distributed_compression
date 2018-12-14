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
#include "i2c_master.h"

#include <compat/twi.h>

#include "lufa.h"
#include "common.h"
#include "buttons.h"
#include "leds.h"

volatile uint8_t workerID = 0;
volatile uint8_t buttonHoldCount = 0;
volatile uint8_t buttonRelease = 0;
volatile uint8_t button_A_mode = 0;
// Uncomment this to print out debugging statements.
//#define DEBUG 1



void A_press () {
  buttonHoldCount = 0;
  while (!buttonRelease) {
    _delay_ms(500);
    if ( ++buttonHoldCount >10) {
      button_A_mode = 1;
      workerID = 0;
      break;
    } else if (button_A_mode)
      workerID++;
  }

}
void A_release() {buttonRelease = 1;}

void set_i2c_address(uint8_t workerID) {
        // set the address register to this workerID
        TWAR = (workerID<<1);
        // Set the clock frequency to 100khz
        TWBR = ((((16000000UL / F_SCL) / 1) - 16) / 2);
        TWCR |= (1<<TWEN)|(1<<TWEA); // Enables the TWI Interface.
}

/****************************************************************************
   ALL INITIALIZATION
****************************************************************************/
void initialize_system(void) {
        initialize_leds(); // set up the leds
        SetupHardware(); // usb communication
        // setup the buttons
        initialize_buttons();
        SetUpButton(&_button_A);
        SetUpButtonAction(&_button_A, 0, A_press);
        SetUpButtonAction(&_button_A, 1, A_release);
        light_show();
}

/****************************************************************************
   MAIN
****************************************************************************/
int main(void) {
        // master will poll status prior to sending/receiving data
        volatile uint8_t status = 0; // 0 receive data, 1 busy, 2 send data
        //  USBCON = 0;
        initialize_system(); // initialization of system

        sei();
        //*******         THE CYCLIC CONTROL LOOP            **********//
        //*************************************************************//
        for (;;) {
                USB_Mainloop_Handler();
        }
}
