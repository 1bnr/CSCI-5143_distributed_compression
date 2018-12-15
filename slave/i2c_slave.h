
#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H
#define I2C_SLAVE_BASE_ID (0x20)

volatile uint8_t buffer_address;
volatile uint8_t out_buffer[256];
volatile uint8_t in_buffer[256];

void i2c_slave_init(uint8_t address);
void i2_slave_stop(void);
ISR(TWI_vect);

#endif // I2C_SLAVE_H
