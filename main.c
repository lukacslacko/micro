#define __AVR_ATtiny2313__
#define F_CPU 1000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define I2C_PORT PORTD
#define I2C_DDR DDRD
#define I2C_PIN PIND
#define I2C_CLOCK (1<<4)
#define I2C_DATA (1<<5)
#define ERROR_LED (1<<3)
#define OK_LED (1<<2)

void clock_low() {
	I2C_DDR |= I2C_CLOCK;
	I2C_PORT &= ~I2C_CLOCK;
}

void clock_release() {
	I2C_DDR &= ~I2C_CLOCK;
	I2C_PORT |= I2C_CLOCK;
}

void data_low() {
	I2C_DDR |= I2C_DATA;
	I2C_PORT &= ~I2C_DATA;
}

void data_release() {
	I2C_DDR &= ~I2C_DATA;
	I2C_PORT |= I2C_DATA;
}

void i2c_init() {
	data_release();
	clock_release();
	I2C_DDR |= OK_LED | ERROR_LED;
	I2C_PORT |= OK_LED;
	I2C_PORT &= ~ERROR_LED;
}

void i2c_start() {
	data_low();
	clock_low();
}

void i2c_stop() {
	clock_release();
	data_release();
}

void i2c_fail() {
	I2C_PORT &= ~OK_LED;
	I2C_PORT |= ERROR_LED;
	while (1);
}

void i2c_send(uint8_t data) {
	uint8_t mask = 0x80;
	while (mask) {
		if (data & mask) data_release();
		else data_low();
		mask >>= 1;
		clock_release();
		clock_low();
	}
	data_release();
	clock_release();
	if (I2C_PIN & I2C_DATA) i2c_fail();
	clock_low();
}

void i2c_seq(const uint8_t* data, uint8_t cnt) {
	i2c_start();
	while (cnt--) i2c_send(*(data++));
	i2c_stop();
}

const uint8_t oled_init[] = {0x78, 0, 0xae, 0xd5, 0x80, 0xa8, 0x3f, 0xd3, 0, 0x40, 0x8d, 0x14, 0x20, 0, 0xa1, 0xc8, 0xda, 0x12, 0x81, 0xcf, 0xd9, 0xf1, 0xdb, 0x40, 0xa4, 0xa6, 0xaf};

int main() {
	i2c_init();
	i2c_seq(oled_init, sizeof(oled_init));
	i2c_start();
	i2c_send(0x78);
	i2c_send(0x40);
	for (int i = 0; i < 128; ++i) i2c_send(i);
	i2c_stop();
}


