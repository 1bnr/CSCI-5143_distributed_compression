
#include <stdlib.h>
#include <math.h>
#include <i2cmaster.h>
//#include <util/twi.h>

#define 	I2C_READ   1
#define 	I2C_WRITE   0
#define 	i2c_read(ack)   (ack) ? i2c_readAck() : i2c_readNak();
#define MB85RC_DEFAULT_ADDRESS        (0x50) /* 1010 + A2 + A1 + A0 = 0x50 default */
#define MB85RC_SLAVE_ID       (0xF8)

volatile uint8_t i2c_addr;


int main(void)
{
    unsigned char ret;
    i2c_init();                             // initialize I2C library
    // write 0x75 to FRAM address 5 (Byte Write)
    i2c_start_wait(Dev24C02+I2C_WRITE);     // set device address and write mode
    i2c_write(0x05);                        // write address = 5
    i2c_write(0x75);                        // write value 0x75 to FRAM
    i2c_stop();                             // set stop conditon = release bus
    // read previously written value back from EEPROM address 5
    i2c_start_wait(Dev24C02+I2C_WRITE);     // set device address and write mode
    i2c_write(0x05);                        // write address = 5
    i2c_rep_start(Dev24C02+I2C_READ);       // set device address and read mode
    ret = i2c_readNak();                    // read one byte from FRAM
    i2c_stop();
    for(;;);
}
