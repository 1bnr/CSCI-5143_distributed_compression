#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#define I2C_READ 0x01
#define I2C_WRITE 0x00
#define I2C_READY 1
#define I2C_BUSY 0
#define i2c_read(ack)   (ack) ? i2c_readAck() : i2c_readNak();
#define MB85RC_DEFAULT_ADDRESS (0x50) /* 1010 + A2 + A1 + A0 = 0x50 default */
#define MB85RC_SLAVE_ID (0xF8)
#define I2C_SLAVE_BASE_ID (0x20) // base i2c address for worker boards
// job codes
#define JOB_BIT 1
#define ENCODE (1<<JOB_BIT)
#define DECODE (0<<JOB_BIT)

volatile uint8_t out_buffer[256];
volatile uint8_t in_buffer[256];

void i2c_init(void);
uint8_t i2c_start(uint8_t address);
uint8_t i2c_start_wait(uint8_t address);
uint8_t i2c_start_timeout(uint8_t address, uint8_t ms_timeout);
uint8_t i2c_write(uint8_t data);
uint8_t i2c_read_ack(void);
uint8_t i2c_read_nack(void);
uint8_t i2c_transmit(uint8_t address, uint8_t* data, uint16_t length);
uint8_t i2c_receive(uint8_t address, uint8_t* data, uint16_t length);
uint8_t i2c_writeReg(uint8_t devaddr, uint16_t offset_addr, uint8_t* data, uint16_t length);
uint8_t i2c_readReg(uint8_t devaddr, uint16_t offset_addr, uint8_t* data, uint16_t length);
uint8_t i2c_write_start(uint8_t devaddr, uint16_t offset_addr);
void format_storage(uint8_t devaddr);
void i2c_stop(void);

#endif // I2C_MASTER_H
