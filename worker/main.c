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
#define write_control PORTD
#define write_pin PORTD2
#define ro_mode() SET_BIT(write_control, write_pin)
#define wr_mode() CLEAR_BIT(write_control, write_pin)

// record of start and end of stored data on fram
struct file {
        uint16_t start;
        uint16_t end;
};

struct fs {
  struct file input;
  struct file output;
} *_fs;

struct file * f;

int input_data(struct file * f);

int output_data(struct file * f);

void Arelease() {

        input_data(&_fs->input);
}
void Crelease() {
//        TOGGLE_BIT(*(&_yellow)->port, _yellow.pin);
        output_data(&_fs->input);
}

volatile uint8_t buttonA = 0;
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
        i2c_init(); // initialize I2C library

}
// read data from stdin, write to fram
int input_data(struct file * f){
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
        ready_light();
        return bytes_read;
}
// read size bytes from fram at address into buffer
int read_file(struct file * f,char * buffer,uint16_t address, uint8_t size){
        int bytes_read = 0;
        // this may return junk if size reads past _file f
        // also may overflow buffer if size is larger than buffer
        if (0 == i2c_readReg(MB85RC_DEFAULT_ADDRESS, address, (uint8_t*) buffer, size))
                bytes_read = strlen(buffer);
        else bytes_read = -1;
        return bytes_read;
}

int output_data(struct file * f) {
        printf("output_data\r\n");
        printf("file size: %d\r\n", f->end - f->start);
        busy_light();
        uint16_t address = f->start;
        char buffer[9];
        memset(buffer, 0, 9);
        while ( address < f->end) {
                int size = ((f->end - address) > 8) ? 8 : (f->end - address);
                if (size) {
                        if (-1 == read_file(f, buffer, address, size)){
                          printf("Error reading fram\r\n");
                          USB_Mainloop_Handler();
                          return -1;
                        }
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
        initialize_buttons();

        SetUpButton(&_button_A);
        SetUpButton(&_button_C);
        SetUpButtonAction(&_button_A, 1, Arelease);
        SetUpButtonAction(&_button_C, 1, Crelease);
        initialize_fram();
        //format_storage(MB85RC_DEFAULT_ADDRESS); // only run this to format the storage
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
