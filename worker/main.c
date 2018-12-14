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
#include "leds.h"


// Uncomment this to print out debugging statements.
//#define DEBUG 1

void slave_setup() {
        // set the addr to 0x51
        TWAR = (0x52<<1);
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
