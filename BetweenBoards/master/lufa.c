
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#include "lufa.h"
#include "leds.h"
#include "i2c_master.h"

#define menuString "\
R/r: read\r\n\
W/w: write\r\n\
S/s: Set address\r\n"

#define F_CPU 16000000UL
#define F_SCL     100000UL // SCL frequency
void wait_for_start(void) {
  TWAR = (0x51<<1);
  TWBR = ((((16000000UL / F_SCL) / 1) - 16) / 2);
	TWSR = 0x00; // Clears the Status Code Register and all prescalers.
	TWCR |= (1<<TWEN)|(1<<TWEA); // Enables the TWI Interface.
  //TWCR = 0b01000100;//init address
  

  while ((TWCR & (1<<TWINT)) != (1 << TWINT)){
    USB_Mainloop_Handler();
    if(TWCR != 0x44) {
      printf("Waiting %hhx\r\n", TWCR);
    } else {
      printf("-------");
    }
  }
}




//extern volatile uint8_t in_ui_mode;
volatile uint8_t ui_stage = 0;
volatile uint8_t mem_location = 0x0;
void handleInput(char c) {
// WARNING: This uses a busy-wait, thus will block main loop until done
    const int COMMAND_BUFF_LEN = 16;
    int i;
    char command[COMMAND_BUFF_LEN + 1];

    USB_Mainloop_Handler();

    // Get chars until end-of-line received or buffer full
    for (i = 0; i < COMMAND_BUFF_LEN; i++) {
        // first char received as input parameter. next char fetched at bottom of for-loop

        // if its backspace, reset index
        if (c == '\b') {
            i -= 2;
            printf("\b \b");
            continue;
        }
        // if newline, go back 1 to write over
        else if (c == '\n') {
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
        command[i] = c;

        // in busy-wait until char received
        c = EOF;

        while (c == EOF) {
            USB_Mainloop_Handler();
            c = fgetc(stdin);
        }
    }

    // echo back the return
    printf("\r\n");
    USB_Mainloop_Handler();

    // buffer is full - ignore input
    if (i == COMMAND_BUFF_LEN) {
        printf("Buffer full\r\n");
        command[COMMAND_BUFF_LEN - 1] = '\r';
        USB_Mainloop_Handler();
        return;
    }

    command[i] = '\0';

    if (strlen(command) == 0) {
        return;
    }

    // handle input per ui_stage
    if (!ui_stage) handleCommand(command);
    else {
        // set new goal here
        mem_location = atoi(command);
        printf("new address is %d\r\n\n", mem_location);
        ui_stage = 0;
        return;
    }

    return;
}

void handleCommand(char *command) {
    uint8_t buffer[16];

    // Start with a char - need to change this to strings

    switch (command[0]) {
        case ('R'):
        case ('r'): {
            // READ COMMAND
          // This should receive bytes
          uint8_t* buffer;
          uint16_t * buffer_length = (uint16_t *) malloc(sizeof(uint16_t));
          *buffer_length = 0;

          buffer = receive_bytes_from_slave(0x51,buffer_length);
          if(*buffer_length != 0) {
            printf("Youre god damn right I forgot a message\r\n");
          } else {
            printf("This class sucks\r\n");
          }
            break;
        }

        // W/w write
        case ('W'):
        case ('w'): {
          // This should send the bytes to the slave device
          uint8_t* buffer;
          uint16_t* buffer_length = (uint16_t*)malloc(sizeof(uint16_t));
          *buffer_length = 5;
          buffer = (uint8_t *)malloc(sizeof(uint8_t) * 5);
          buffer[0]='h';
          buffer[1]='e';
          buffer[2]='l';
          buffer[3]='l';
          buffer[4]='o';
          printf("Buffer0:%hhx\r\n",buffer[0]);
          printf("Buffer1:%hhx\r\n",buffer[1]);
          printf("Buffer2:%hhx\r\n",buffer[2]);
          printf("Buffer3:%hhx\r\n",buffer[3]);
          printf("Buffer4:%hhx\r\n",buffer[4]);
          int result = send_bytes_to_slave(0x51,buffer, buffer_length);
        if(result != 0) {
          printf("Now some babies need punchin'\r\n");
        } else {
          printf("WE FUCKIN DID BOIS\r\n");
        }
            break;
        }

        // S/s: Set read/write address
        case ('S'):
        case ('s'):
            printf("enter new address: \r\n");
            ui_stage = 1;
            break;
    case ('Z'):
    case ('z'):
      printf("waiting\r\n");
      wait_for_start();
        default:
            printf(menuString);
    }

    USB_Mainloop_Handler();
}
