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
	i2c_stop();
	i2c_start();
	while (cnt--) i2c_send(*(data++));
	i2c_stop();
	i2c_start();
	i2c_send(0x78);
	i2c_send(0x40);
}

const uint8_t oled_init[] = {0x78, 0, 0xae, 0xd5, 0x80, 0xa8, 0x3f, 0xd3, 0, 0x40, 0x8d, 0x14, 0x20, 0, 0xa1, 0xc8, 0xda, 0x12, 0x81, 0xcf, 0xd9, 0xf1, 0xdb, 0x40, 0xa4, 0xa6, 0xaf};

const uint8_t abc[] = {
	0x7c, 0x0a, 0x09, 0x0a, 0x7c, // A
	0x7f, 0x49, 0x49, 0x49, 0x36, // B
	0x3e, 0x41, 0x41, 0x41, 0x22, // C
};

void print_char(char c) {
	const char i = c - 'A';
	const uint8_t* p = abc + i + (i<<2);
	i2c_send(p[0]);
	i2c_send(p[1]);
	i2c_send(p[2]);
	i2c_send(p[3]);
	i2c_send(p[4]);
	i2c_send(0);
}

void print(const char* s) {
	while (*s) print_char(*(s++));
}

void moveto(char col, char row) {
	const uint8_t cmds[] = {0x78, 0, 0x21, col, 0x7f, 0x22, row, 7};
	i2c_seq(cmds, sizeof(cmds));
}

uint8_t lokey, hikey;

void readkeys() {
	PORTB = 0xf;
	DDRB = 0x10;
	lokey = PINB & 0xf;
	DDRB = 0x20;
	lokey |= (PINB & 0xf) << 4;
	DDRB = 0x40;
	hikey = PINB & 0xf;
	DDRB = 0x80;
	hikey |= (PINB & 0xf) << 4;
}

void draw(uint8_t b) {
	uint8_t f = b ? 0x5a : 0x42;
	i2c_send(0);
	i2c_send(0x7e);
	i2c_send(0x42);
	i2c_send(f);
	i2c_send(f);
	i2c_send(0x42);
	i2c_send(0x7e);
	i2c_send(0);
}

int main() {
	i2c_init();
	i2c_seq(oled_init, sizeof(oled_init));
	for (int i = 0; i < 128*8; ++i) i2c_send(0);
	print("BABACACA");
	moveto(64,2);
	print("ABC");
	moveto(0,3);
	while(1) {
		readkeys();
		for (uint8_t i = 0; i < 8; ++i) {
			uint8_t x = 8*(i>>1);
			uint8_t y = i&1;
			moveto(x, y+3);
			draw(lokey & (1<<i));
			moveto(x, y+5);
			draw(hikey & (1<<i));
		}
	}
}
