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
#include "buttons.h"



// record of start and end of stored data on fram
struct file {
        uint16_t start;
        uint16_t end;
};

struct fs {
  struct file input;
  struct file output;
} *_fs;

//struct file * f; // a file
// declarations for input/output functions
int input_data(struct file * f);
int output_data(struct file * f);
// button C release action
void C_release() { initialize_fs(); output_data(&(_fs->input));}

volatile uint8_t buttonHoldCount = 0;
volatile uint8_t buttonRelease = 0;
// Uncomment this to print out debugging statements.
//#define DEBUG 1


// press and hold button A to reformat storage
void A_press () {
  printf("A_press\r\n");
  buttonRelease = 0;
  buttonHoldCount = 0;
  while (!buttonRelease) {
    _delay_ms(500);
    if ( ++buttonHoldCount >10) { // 5 second hold; reformat storage
      printf("calling format_storage\r\n");
      format_storage(MB85RC_DEFAULT_ADDRESS);
      break;
    } else if (buttonRelease){// short press and release; read in data from serial connection
      input_data(&(_fs->input));
      break;
    }
  }

}
void A_release() {
  printf("buttonRelease\r\n");
  buttonRelease = 1;
}


// Uncomment this to print out debugging statements.
//#define DEBUG 1
// initialize the FRAM chip
void initialize_fram() {

        // initialize power supply portD pin4 astar pin 4
        DDRD |= (1 << DDD4);
        PORTD |= (1 << PORTD4);

        // address pins on PB4,PB5,PB6 (astar 8,9,10)
        // sda PD1 (astar 2)
        // SCL PD0 (astar 3)
        i2c_init(); // initialize I2C library

}
// read data from stdin, write to FRAM; file f is a struct that defines start
// and end of data
int input_data(struct file * f){
  printf("calling input_data\r\n");
        busy_light();
        char buffer[9];
        int bytes_read = 0;
        memset(buffer, 0, 9);
        while (fgets(buffer, 9, stdin)) {
                USB_Mainloop_Handler();
                bytes_read = strlen(buffer);
                USB_Mainloop_Handler();
                i2c_writeReg(MB85RC_DEFAULT_ADDRESS, f->end, (uint8_t*) buffer, bytes_read);
                f->end +=bytes_read;
                if (buffer[bytes_read-1] == '\0') break;
                memset(buffer, 0, 9);
        }
        // update file end

        memcpy(buffer, &(f->end), 2);
        i2c_writeReg(MB85RC_DEFAULT_ADDRESS, 2, (uint8_t*) buffer, 2);
        ready_light();
        return bytes_read;
}
// read size bytes from fram at address into buffer
int read_data(struct file * f,char * buffer,uint16_t address, uint8_t size){
        int bytes_read = 0;
        // this may return junk if size reads past _file f
        // also may overflow buffer if size is larger than buffer
        if (0 == i2c_readReg(MB85RC_DEFAULT_ADDRESS, address, (uint8_t*) buffer, size))
                bytes_read = strlen(buffer);
        else bytes_read = -1;
        return bytes_read;
}
// print out the file f stored on FRAM; f is a struct that defines the start
// and end of the data on the FRAM chip
int output_data(struct file * f) {
        printf("\r\noutput_data\r\n");
        printf("file size: %d\r\n", f->end - f->start);
        busy_light();
        uint16_t address = f->start;
        char buffer[9];
        memset(buffer, 0, 9);
        while ( address < f->end) {
                int size = ((f->end - address) > 8) ? 8 : (f->end - address);
                if (size) {
                        i2c_readReg(MB85RC_DEFAULT_ADDRESS, address, (uint8_t*) buffer, size);
                        address+=8;
                        printf(buffer);
                        USB_Mainloop_Handler();
                        memset(buffer, 0, 9);
                }
        }
        ready_light();
        return (f->end - f->start);
}

void initialize_fs(){
  uint16_t metadata[4];
  //read the fs metadata from the first 8 bytes of FRAM
  i2c_readReg(MB85RC_DEFAULT_ADDRESS, 0, (uint8_t*) metadata, 8);

  // storage for raw input; offset for this metadata
  _fs = (struct fs*) malloc(8);
  printf("start %d\r\n", metadata[0]);
  printf("end %d\r\n", metadata[1]);
  _fs->input.start = metadata[0];
  _fs->input.end = metadata[1];
  // storage for finished work
  _fs->output.start = metadata[2];
  _fs->output.end = metadata[3];
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
        SetUpButton(&_button_C);
        SetUpButtonAction(&_button_A, 0, A_press);
        SetUpButtonAction(&_button_A, 1, A_release);
        SetUpButtonAction(&_button_C, 1, C_release);
        // initialize the FRAM
        initialize_fram();
        initialize_fs();
        light_show(); // so we know when formating is done
}

/****************************************************************************
   MAIN
****************************************************************************/
int main(void) {

        //  USBCON = 0;
        initialize_system(); // initialization of system

        sei();
        //*******         THE CYCLIC CONTROL LOOP            **********//
        //*************************************************************//
        for (;;) {
                USB_Mainloop_Handler();
        }
}
