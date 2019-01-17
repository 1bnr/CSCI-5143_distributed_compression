/*
	based on code found here:
	https://github.com/g4lvanix/I2C-slave-lib
	some changes were made to better suit the project

*/
#include <avr/io.h>
#include <util/twi.h>
#include <avr/interrupt.h>
#include <string.h> /* strlen */

#include "i2c_slave.h"
#include "huffman.h"

void i2c_slave_init(uint8_t address){
	// load address into TWI address register
	TWAR = (address << 1);
	// enable address matching and enable TWI, clear TWINT, enable TWI interrupt
	TWCR = (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWEN);
}

void i2c_slave_stop(void){
	// clear acknowledge and enable bits from the control register
	TWCR &= ~( (1<<TWEA) | (1<<TWEN) );
}

ISR(TWI_vect){

	// temporary stores the received data
	uint8_t data;

	// own address has been acknowledged
	if( TW_SR_SLA_ACK == (TWSR & 0xF8) ){
		buffer_address = 0xFF;
		// clear TWI interrupt flag, prepare to receive next byte and acknowledge
		TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
	}
	else if( TW_SR_DATA_ACK == (TWSR & 0xF8) ){ // data has been received in slave receiver mode

		// save the received byte inside data
		data = TWDR;

		// check wether an address has already been transmitted or not
		if( 0xFF == buffer_address ){

			buffer_address = data;

			// clear TWI interrupt flag, prepare to receive next byte and acknowledge
			TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
		}
		else{ // if a data byte has already been received

			// store the data at the current address
			in_buffer[buffer_address] = data;

			// increment the buffer address
			buffer_address++;

			// if there is still enough space inside the buffer
			if(buffer_address < 0xFF){
				// clear TWI interrupt flag, prepare to receive next byte and acknowledge
				TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
			}
			else{
				// Don't acknowledge
				TWCR &= ~(1<<TWEA);
				// clear TWI interrupt flag, prepare to receive last byte.
				TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEN);
			}
		}
		// process job; status byte / metadata bits: [1_status][2_job][3-8_seq_num]
		// emptying out_buffer will set I2C_BUSY to status byte of buffer
		memset((void*) out_buffer, 0, 256);
		uint8_t seq_num = (in_buffer[0]>>2); // strip off status and job bits
		uint8_t job = ((1<<JOB_BIT) & in_buffer[0]);
		if (ENCODE == (job & ENCODE)){ // if job is encode
			huffman_compress(&in_buffer[2], in_buffer[1], &out_buffer[2]);
			out_buffer[1] = strlen(&out_buffer[2]);
		} else { // else job is decode
			huffman_inflate(&in_buffer[2], in_buffer[1], &out_buffer[2]);
			out_buffer[1] = strlen(&out_buffer[2]);
		} // fill output status bit with block seq_num, job code, and ready flag
		out_buffer[0] = ((seq_num<<2) | job | I2C_READY);
	}
	else if( TW_ST_DATA_ACK == (TWSR & 0xF8) ){ // device has been addressed to be a transmitter

		// copy data from TWDR to the temporary memory
		data = TWDR;

		// if no buffer read address has been sent yet
		if( buffer_address == 0xFF ){
			buffer_address = data;
		}

		// copy the specified buffer address into the TWDR register for transmission
		TWDR = out_buffer[buffer_address];
		// increment buffer read address
		buffer_address++;

		// if there is another buffer address that can be sent
		if(buffer_address < 0xFF){
			// clear TWI interrupt flag, prepare to send next byte and receive acknowledge
			TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
		}
		else{
			// no acknowledgement
			TWCR &= ~(1<<TWEA);
			// clear i2c interrupt flag, prepare to receive last byte.
			TWCR |= (1<<TWEA) | (1<<TWEN) | (1<<TWIE);
		}

	}
	else{
		//  prepare i2c connection for next request
		TWCR |= (1<<TWEA) | (1<<TWEN) | (1<<TWIE);
	}
}
