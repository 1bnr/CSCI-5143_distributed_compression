/*
   main/main.c
   The main module is the controler image for the distributed compression system
   the controler device will present a readable/writable serial device to the
   attached system and send jobs to attached slave devices to be processed.

   incoming data is simply raw bytes. There two jobs that can be accomplished:
    data compression utilizing huffman encoding; job code ENCODE
    data expansion reversing the huffman encoding job code DECODE

   Compressed blocks are prefixed by the number of bytes in the encoded block,
   raw (uncompressed) input is simply a block of bytes.

 */
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define BLOCK_SIZE 128 // size of encoding blocks
#define A_PRESS 0x01
#define A_RELEASE 0x02
#define C_PRESS 0x04
#define C_RELEASE 0x08

// library includes
#include <avr/interrupt.h>
#include <util/delay.h> /*_delay_ms */
#include <string.h> /* strlen */
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include "i2c_master.h"

#include <compat/twi.h>


#include "common.h"
#include "leds.h"
#include "buttons.h"


extern void SetupHardware(void);
extern void USB_Mainloop_Handler(void);
void initialize_fs();

typedef struct block_seq {
  uint16_t start;
  uint8_t size;
} block_seq_t;

// record of start and end of stored data on fram
typedef struct file {
  uint16_t start;
  uint16_t end;
} file_t;

typedef struct fs{
  struct file input;
  struct file output;
} fs_t;
volatile uint8_t FS[8];
volatile fs_t *_fs = (fs_t*) FS;

volatile uint16_t bytes_read = 0;
//struct file * f; // a file
// declarations for input/output functions
int input_datasteam(struct file * f); // read datastream from serial
int output_datastream(struct file * f); // write data to sereal
// button flag variables
volatile uint8_t buttonHoldCount = 0;
volatile uint8_t button_status = 0;

// Uncomment this to print out debugging statements.
//#define DEBUG 1


// button A press action;; called by interrupt
void A_press () {
  printf("A_press\r\n");
  button_status = (( button_status | A_PRESS ) ^A_RELEASE );

}
// button A release action; called by interrupt
void A_release() {
  printf("A_Release\r\n");
  button_status = (( button_status | A_RELEASE ) ^A_PRESS );
}
// polling function for button A; detects a 5 second hold
void pollButtonA(){
  buttonHoldCount = 0;
  while ((button_status & A_PRESS) && !(button_status & A_RELEASE)) {
    busy_light();
    _delay_ms(500);
    if ( ++buttonHoldCount >10) { // 5 second hold; reformat storage
      printf("calling format_storage\r\n");
      format_storage(MB85RC_DEFAULT_ADDRESS);
      ready_light();
      break;
    } else if (button_status & A_RELEASE) {// short press and release; read in data from serial connection
      bytes_read = input_datastream(&(_fs->input));
      output_datastream(&(_fs->input)); // print back out what we just read in
      ready_light();
      break;
    }
  }
}
// button C press action; called by interrupt
void C_press(){
  printf("C press\r\n");
  button_status = (( button_status | C_PRESS ) ^C_RELEASE );
}
// button C release action; called by interrupt
void C_release() {
  printf("C Release\r\n");
  button_status = (( button_status | C_RELEASE ) ^C_PRESS );;
}
// polling function for button C; detects a five second hold
void pollButtonC(){
  buttonHoldCount = 0;
  while ((button_status & C_PRESS) && !(button_status & C_RELEASE)) {
    busy_light();
    _delay_ms(500);
    if ( ++buttonHoldCount >10) {
      processData();
      ready_light();
      break;
    } else if (button_status & A_RELEASE) {// short press and release
      output_datastream(&(_fs->output)); // print out processed data
      ready_light();
      break;
    }
  }
}


// Uncomment this to print out debugging statements.
//#define DEBUG 1
// initialize the FRAM chip
void initialize_fram() {
  // initialize power supply portB pin4 astar pin 8
  DDRB |= (1 << DDB4);
  PORTB |= (1 << PORTB4);

  // address pins on PB4,PB5,PB6 (astar 8,9,10)
  // sda PD1 (astar 2)
  // SCL PD0 (astar 3)
  i2c_init(); // initialize I2C library

}
// read data from stdin, write to FRAM; file f is a struct that defines start
// and end of data
int input_datastream(struct file * f){
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
  //output_datastream(f); // print out what was just loaded in
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
int output_datastream(struct file * f) {
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
/******************************************************************************
  Step through stored data building array of address and block sizes for final
  assembly of inflated data
******************************************************************************/
void sequence_blocks (block_seq_t * seq, uint8_t block_count){
  uint16_t address = 8; //start at begining of "file"
  for (int i=0; i<block_count; i++){
    uint8_t buffer[2];
    i2c_readReg(MB85RC_DEFAULT_ADDRESS, address, (uint8_t*) buffer, 2);
    block_seq_t * block = &seq[(buffer[0]>>1)]; // select block by seq_num
    block->start = address;
    block->size = buffer[1];
    address =+ buffer[1] + 2; // 2 bytes for header [seq_num|job][size]
  }
}

/******************************************************************************
  scan the i2c bus for workers; record found addresses in workers array, and
  return the number of found worker devices.
  param uint8_t workers : array to collect worker i2c addresses
  returns the number of found workers
******************************************************************************/
uint8_t scan_i2c(uint8_t * workers) {
  uint8_t count = 0;
  uint8_t workerID = I2C_SLAVE_BASE_ID;
  // worker boards must have i2c slave addresses starting at 0x20, incrementing
  // without any gaps; the first open address will stop scan and return current
  // number of found devices
  while (!i2c_start_timeout(((workerID << 1) | I2C_READ), 10)) {
    printf("address %hhx in use; trying next address.\r\n", workerID);
    workers[count++] = workerID++; // store the found address; then check next
  } // while loop breaks on first open address
  return count; // the number of found slave addresses
}

/*******************************************************************************
  begin sending jobs to workers
*******************************************************************************/
void processData(uint8_t job){

  file_t * output = &(_fs->output); // output file on FRAM
  file_t * input = &(_fs->input); // input file on FRAM
  uint8_t irp = input->start; // read "pointer"; next byte to read in "file"
  uint8_t orp = output->start; // read "pointer"; next byte to read in "file"
  uint8_t block_count = 0;
  struct block_seq_t * seq ;

  uint8_t workers[10]; // allows for 10 worker board addresses
  uint8_t worker_count = scan_i2c(workers); // how many workers were located

  if (ENCODE == job) {
    block_count = ((input->end - input->start)/ BLOCK_SIZE) \
                + ((input->end - input->start)%BLOCK_SIZE)? 1:0;
  } else { // job is decode, use sequence bytes
    // retrieve number of blocks from end of input "file"
    i2c_readReg(MB85RC_DEFAULT_ADDRESS, input->end -1, (uint8_t*) &block_count, 1);
    block_seq_t seq[block_count];
    sequence_blocks(seq, block_count);
  }

  // once data has been loaded onto the FRAM input "file", begin processing
  uint8_t block = 0;
  while ( block < block_count) {
    // first and second bytes are for worker status and message size
    uint8_t * status = in_buffer;
    uint8_t * bytes_in = &in_buffer[1];
    // loop over workers checking for readiness
    for (int i=0; i< worker_count; i++) {
      i2c_readReg(workers[i], 0, (uint8_t*) in_buffer, 2); // read 2 bytes
      uint8_t seq_num = (in_buffer[0]>>1); // strip sequencing number
      if (*bytes_in) { // worker has data to send
        // receive data from worker
        i2c_readReg(workers[i], 2, (uint8_t*) &in_buffer[2], *bytes_in);
        // store data on FRAM, first byte has sequencing number and job code,
        // second is length of block, write length  bytes plus 2 header bytes
        i2c_writeReg(MB85RC_DEFAULT_ADDRESS, output->end, (uint8_t*) in_buffer, *bytes_in + 2);
        output->end += (*bytes_in) + 1; // move output file write pointer forward
        if (++block == block_count)
        i2c_writeReg(MB85RC_DEFAULT_ADDRESS, output->end, (uint8_t*) &block, 1); // write number of blocks to FRAM
      } // done receiving bytes from worker

      if ((*status) & I2C_READY) { // if worker is ready to recieve job
        out_buffer[0] = ((block<<1) | job); // pack block sequence number in with job code
        if (job == ENCODE) { // can take fixed size blocks of input data from FRAM, upto end of "file"
          uint8_t bytes_read = ((input->end - irp) > BLOCK_SIZE) ? BLOCK_SIZE : (input->end - irp);
          out_buffer[1] = bytes_read;
          // fill out_buffer from FRAM input "file" starting at irp
          i2c_readReg(MB85RC_DEFAULT_ADDRESS, irp, &out_buffer[2], bytes_read );
          irp += bytes_read; // advance "read pointer" for next read
          // send data to worker; first two bytes are job code and data size
          i2c_writeReg(workers[i], 0, out_buffer, bytes_read + 2);
        } else { // job is decode; encoded data will be available sized
          // read first byte at irp to determine size of encoded block on FRAM
          i2c_readReg(MB85RC_DEFAULT_ADDRESS, irp++, &out_buffer[1], 1 );
          // fill out_buffer from FRAM input "file" starting at irp
          i2c_readReg(MB85RC_DEFAULT_ADDRESS, irp, &out_buffer[2], out_buffer[1] );
          irp += out_buffer[1]; // advance "read pointer" for next block to read
          // send data to worker; first two bytes are job code and data size
          i2c_writeReg(workers[i], 0, out_buffer, out_buffer[1] + 2);
        }
      }
    }
  }

}

/*******************************************************************************
  Create a "filesystem" on the FRAM that consists of tracking the start and end
  of two "Files" in the first four bytes of the FRAM. The first file starts at
  FRAM byte 4 is input; raw data written in through the serial inteface.
  The input is the data that is to be processed by the system. The second file,
  starting at FRAM byte 1604 is output, the post processed data, prefixed by the
  length of the data block, recorded in the first byte of the block.
*******************************************************************************/
void initialize_fs(){
  uint16_t metadata[4];
  //read the fs metadata from the first 8 bytes of FRAM
  i2c_readReg(MB85RC_DEFAULT_ADDRESS, 0, (uint8_t*) metadata, 8);

  // storage for raw input; offset for this metadata
//  _fs = (struct fs*) malloc(8);
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
void initialize_system() {
  // initialize the FRAM
  initialize_fram();
  initialize_fs();
  initialize_leds(); // set up the leds
  SetupHardware(); // usb communication
  // setup the buttons
  initialize_buttons();
  SetUpButton(&_button_A);
  SetUpButton(&_button_C);
  SetUpButtonAction(&_button_A, 0, A_press);
  SetUpButtonAction(&_button_A, 1, A_release);
  SetUpButtonAction(&_button_C, 1, C_press);
  SetUpButtonAction(&_button_C, 1, C_release);
  light_show(); // so we know when formating is done
}

/****************************************************************************
   MAIN
****************************************************************************/
int main(void) {
  //  USBCON = 0;
  initialize_system(); // initialization of system
  sei();

  //*******   THE CYCLIC CONTROL LOOP      **********//
  //*************************************************************//
  for (;;) {
    pollButtonA();
    pollButtonC();
    USB_Mainloop_Handler();
  }
}
