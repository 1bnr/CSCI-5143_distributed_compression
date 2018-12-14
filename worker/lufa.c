
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "lufa.h"
#include "leds.h"
#include "i2c_master.h"
#include "huffman.h"

#define menuString "(r)ead // (w)rite // (s)et address // (p)agewrite // (h)uffman test\r\n"

typedef enum {
    HANDLE_COMMAND, HANDLE_MEM_LOC
} ui_stage_t;

typedef void (*input_handler)(char *);

static void handle_memory_location(char *input);

input_handler handler = handleCommand;
uint16_t mem_location = 0x0;


static int read_line(char *buff, int max_len)
{
    int i;
    for (i = 0; i < max_len; i++) {
        char c = EOF;
        while (c == EOF) {
            USB_Mainloop_Handler();
            c = fgetc(stdin);
        }

        if (c == '\n') {
            i--;
            continue;
        }

        // if return, we have complete input
        else if (c == '\r') {
            break;
        }

        // otherwise, we got a char - echo it back
        printf("%c", c);
        USB_Mainloop_Handler();
        buff[i] = c;
    }

    buff[i] = '\0';
    return i;
}

static int read_bytes(char *buff, int num_bytes)
{
  int i;
  for (i = 0; i < num_bytes; i++) {
      char c = EOF;
      while (c == EOF) {
          USB_Mainloop_Handler();
          c = fgetc(stdin);
      }

      USB_Mainloop_Handler();
      buff[i] = c;
  }

  return i;
}

void test_huffman() {
    char data[128];
    read_line(&data[0], 128);
    const uint16_t data_size = (uint16_t) strlen(data);

    uint8_t out_data[128];
    huffman_compress((uint8_t *) data, data_size, &out_data[0]);
    uint16_t num_bits = (out_data[1] << 8) | out_data[2];
    num_bits += 24; // header
    printf("\norignal size: %d bits, %d bytes\r\n", data_size * 8, data_size);
    printf("\ncomppressed size: %d bits, %d bytes\r\n", num_bits, num_bits / 8);
}

static void handle_memory_location(char *location)
{
    mem_location = atoi(location);
    printf("new address is %d\r\n\n", mem_location);
    handler = handleCommand;
}

static void execute_pagewrite(void)
{

  printf("In execute_pagewrite\n");
  USB_Mainloop_Handler();

  char buff[2];
  read_bytes(buff, 2);
  // read total amount of bytes to be written first
  uint16_t pagewrite_total_size = (buff[1] << 8) | buff[0];

  // read amount of bytes
  uint16_t pagewrite_block_size = 1;
  char block[pagewrite_block_size];

  uint16_t bytes_written = 0;

  i2c_write_start(MB85RC_DEFAULT_ADDRESS, 0);

  while (bytes_written < pagewrite_total_size) {
      int bytes_read = read_bytes(block, pagewrite_block_size);

      for (int i = 0; i < bytes_read; i++) {
          i2c_write(block[i]);
      }

      bytes_written += bytes_read;
  }

  i2c_stop();
  printf("wrote %d bytes successfully\r\n", bytes_written);
  USB_Mainloop_Handler();
}

void handleInput(char c) {
// WARNING: This uses a busy-wait, thus will block main loop until done
    const int COMMAND_BUFF_LEN = 16;
    char command[COMMAND_BUFF_LEN + 1];

    command[0] = c;
    printf("%c", c);
    read_line(command + 1, COMMAND_BUFF_LEN);

    // echo back the return
    printf("\r\n");
    USB_Mainloop_Handler();

    if (strlen(command) == 0) {
        return;
    }

    // buffer is full - ignore input
    if (strlen(command) == COMMAND_BUFF_LEN) {
        printf("Buffer full\r\n");
        USB_Mainloop_Handler();
        return;
    }

    handler(command);
}

void handleCommand(char *command) {
    uint8_t buffer[16];

    // Start with a char - need to change this to strings

    switch (command[0]) {
        case ('R'):
        case ('r'): {
            // READ COMMAND
            memset(&buffer, 0, 16);
            if (i2c_readReg(MB85RC_DEFAULT_ADDRESS, mem_location, buffer, 16)) {
                printf("ERROR in read\r\n");
            } else {
                printf("addr 0x%04x ", mem_location);
                for (int i=0;i< 16; i++) {
                    printf("%c ", (char) buffer[i]);
                }
                printf("\r\n");
            }
            break;
        }

        // W/w write
        case ('W'):
        case ('w'): {
            // WRITE COMMAND
            sprintf((char *)buffer, "hello world");
            if (i2c_writeReg(MB85RC_DEFAULT_ADDRESS, mem_location, buffer, 11)){
                printf("ERROR in write\r\n");
            }
            break;
        }

        // S/s: Set read/write address
        case ('S'):
        case ('s'):
            printf("enter new address: \r\n");
            USB_Mainloop_Handler();
            handler = handle_memory_location;
            break;

        case ('p'):
        case ('P'):
            printf("pagewrite mode, enter total size\r\n");
            USB_Mainloop_Handler();
            execute_pagewrite();
            break;

        case ('h'):
        case ('H'):
            printf("test huffman_compress\n");
            test_huffman();
            break;

        default:
            printf(menuString);
    }

    USB_Mainloop_Handler();
}
