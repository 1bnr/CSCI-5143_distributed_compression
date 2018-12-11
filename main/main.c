#ifndef F_CPU
#define F_CPU 16000000UL
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


// Uncomment this to print out debugging statements.
//#define DEBUG 1

// mutex access to ms_ticks
volatile uint8_t in_ui_mode = 0;

void initialize_fram() {
     // initialize power supply portD pin4 astar pin 4
    DDRD |= (1 << DDD4);
    PORTD |= (1 << PORTD4);

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
}


/****************************************************************************
   MAIN
****************************************************************************/
char c;
int main(void) {

    //  USBCON = 0;
    initialize_system(); // initialization of system
    sei();

    //*******         THE CYCLIC CONTROL LOOP            **********//
    //*************************************************************//
    for (;;) {
      USB_Mainloop_Handler();
      if ((c = fgetc(stdin)) != EOF) {
                handleInput(c);
      }
    }
}
