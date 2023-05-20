#define F_CPU 1000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define I2C_PORT PORTD
#define I2C_DDR DDRD
#define I2C_PIN PIND
#define I2C_CLOCK (1<<4)
#define I2C_DATA (1<<5)
#define ERROR_LED (1<<3)
#define OK_LED (1<<2)
#define true 1
#define false 0
#define SHOW_NEXT 1
#define USE_HOLD 1

typedef char bool;
typedef uint8_t u8;
typedef uint16_t u16;
typedef int8_t i8;
typedef int16_t i16;

u8 board[25];
u16 piece;
u16 next;
u16 hold;
i8 posx;
i8 posy;
u16 lines_cleared;
u8 pi;
u8 ni;
u8 hi;
u16 seed;

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

const uint8_t oled_init[] /*PROGMEM*/ = {0x78, 0, 0xae, 0xd5, 0x80, 0xa8, 0x3f, 0xd3, 0, 0x40, 0x8d, 0x14, 0x20, 0, 0xa1, 0xc8, 0xda, 0x12, 0x81, 0xcf, 0xd9, 0xf1, 0xdb, 0x40, 0xa4, 0xa6, 0xaf};

void range(char colmin, char colmax, char rowmin, char rowmax) {
	const uint8_t cmds[] = {0x78, 0, 0x21, colmin, colmax, 0x22, rowmin, rowmax};
	i2c_seq(cmds, sizeof(cmds));
}

void tetris_get_next();
void tetris_init();
void tetris_move_up();
bool tetris_check(u16);
void tetris_left();
void tetris_right();
void tetris_rot1();
void tetris_rot2();
void tetris_fall();
void tetris_hold();
void tetris_end();

const u16 pieces[] PROGMEM = {0x0660,0x8888,0x0710,0x0170,0x06c0,0x0c60,0x4640,0,0x0660,0xf000,0x0226,0x0446,0x4620,0x2640,0x0720,0,0x0660,0x1111,0x08e0,0x0e80,0x0360,0x0630,0x0262,0,0x0660,0x000f,0x6220,0x6440,0x0462,0x0264,0x04e0,0};

u8 rand() {
	seed = 58566*seed+27443;
	return (seed*53458)>>8;
}

i8 rand7() {
	return rand()%7;
}

void tetris_get_next() {
	ni = rand7();
	next = pieces[ni];
}

bool get_board(u8 x, u8 y) {
	u8 t = ((y<<3)+(y<<1)+x);
	return (board[t>>3]&(1<<(t&7)))>>(t&7);
}

void set_board(u8 x, u8 y, bool s) {
	u8 t = ((y<<3)+(y<<1)+x);
	board[t>>3]&=~(1<<(t&7));
	if (s) {
		board[t>>3]|=(1<<(t&7));
	}
}

void tetris_xor() {
	u16 t = piece;
	for ( i8 y = 0; y < 4; y++) {
		for ( i8 x = 0; x < 4; x++) {
			if ( t&0x80 ) {
				set_board(posx+x,posy+y,!get_board(posx+x,posy+y));
			}
			t <<= 1;
		}
	}
}

void tetris_init() {
	for (u8 i = 0; i < 25; i++) { board[i] = 0; }
	hold = 0;
	tetris_get_next();
	piece = next;
	pi = ni;
	tetris_get_next();
	tetris_move_up();
	lines_cleared = 0;
}

void tetris_move_up() {
	posy = 0;
	posx = 3;
	if (!tetris_check(piece)) {
		tetris_end();
	}
}

void tetris_clear_lines() {
	u8 ltc = 0;
	u16 cl = 0;
	for ( i8 l = 0; l < 20; l++ ) {
		for ( i8 i = 0; i < 10; i++ ) {
			cl <<= 1;
			cl += get_board(i,l);
		}
		if ((cl&0x3ff)==0x3ff) {
			ltc++;
			lines_cleared++;
		}
	}
	while (ltc) {
		cl = 0;
		i8 l = 0;
		while (cl!=0x3ff) {
			for ( i8 i = 0; i < 10; i++ ) {
				cl <<= 1;
				cl += get_board(i,l);
				set_board(i,l,(cl>>10)&1);
			}
			l++;
		}
		ltc--;
	}
}

bool tetris_check(u16 p) {
	for ( i8 y = 0; y < 4; y++ ) {
		for ( i8 x = 0; x < 4; x++ ) {
			if ( p&0x8000 ) {
				i8 nx = posx+x;
				i8 ny = posy+y;
				if ( nx < 0 || ny < 0 || nx > 9 || ny > 19 ) {
					return false;
				}
				if ( get_board(nx, ny) ) {
					return false;
				}
			}
			p <<= 1;
		}
	}
	return true;
}

void tetris_left() {
	posx--;
	if (!tetris_check(piece)) {
		posx++;
	}
}

void tetris_right() {
	posx++;
	if (!tetris_check(piece)) {
		posx--;
	}
}

void tetris_rot1() {
	/*u16 new = ((piece<<12)&32768)|((piece<<7)&16384)|((piece<<2)&8192)|((piece>>3)&4096)|((piece<<9)&2048)|((piece<<4)&1024)|((piece>>1)&512)|((piece>>5)&256)|((piece<<6)&128)|((piece<<1)&64)|((piece>>4)&32)|((piece>>9)&16)|((piece<<3)&8)|((piece>>2)&4)|((piece>>7)&2)|((piece>>12)&1);
	if (tetris_check(new)) {
		piece = new;
	}*/
	pi += 8;
	if (tetris_check(pieces[pi&31])) {
		piece = pieces[pi&32];
	} else {
		pi -= 8;
	}
}

void tetris_rot2() {
	/*u16 new = ((piece<<3)&32768)|((piece<<6)&16384)|((piece<<9)&8192)|((piece<<12)&4092)|((piece>>2)&2048)|((piece<<1)&1024)|((piece<<4)&512)|((piece<<7)&256)|((piece>>7)&128)|((piece>>4)&64)|((piece>>1)&32)|((piece<<2)&16)|((piece>>12)&8)|((piece>>9)&4)|((piece>>6)&2)|((piece>>3)&1);
	if (tetris_check(new)) {
		piece = new;
	}*/
	pi -= 8;
	if (tetris_check(pieces[pi&31])) {
		piece = pieces[pi&32];
	} else {
		pi += 8;
	}
}

void tetris_fall() {
	posy++;
	if (!tetris_check(piece)) {
		tetris_xor();
		piece = next;
		pi = ni;
		tetris_get_next();
		tetris_move_up();
		tetris_clear_lines();
	}
}

void tetris_instant_drop() {
	do {tetris_fall();} while (posy);
}

void tetris_hold() {
#if USE_HOLD
	if (!hold) {
		hold = piece;
		hi = pi;
		piece = next;
		pi = ni;
		tetris_get_next();
		tetris_move_up();
		return;
	}
	u16 t = piece;
	u8 ti = pi;
	piece = hold;
	pi = hi;
	hold = t;
	hi = ti;
	tetris_move_up();
#endif
}

void tetris_draw_board() {
	tetris_xor();
	range(36,116,3,7);
	for ( i8 x = 9; x > 0; x -= 2 ) {
		for ( i8 l = 0; l < 20; l++ ) {
			u8 p = (get_board(x,l)?0xf:0)|(get_board(x-1,l)?0xf0:0);
			for ( i8 i = 0; i < 4; i++ ) {
				i2c_send(p);
			}
		}
	}
	tetris_xor();
}

void tetris_draw_next() {
#if SHOW_NEXT
	u16 t = pieces[(ni+8)&31];
	range(8,23,1,2);
	for ( i8 i = 0; i < 2; i++ ) { for ( i8 j = 0; j < 4; j++ ) {
		u8 p = (t&0x8000?0xf:0)|(t&0x800?0xf0:0);
		for ( i8 k = 0; k < 4; k++ ) { i2c_send(p); }
		t <<= 1;
	} t <<= 4; }
	/*u16 t = next;
	range(42,74,2,5);
	for ( i8 i = 0; i < 16; i++ ) {
		for ( i8 j = 0; j < 8; j ++ ) {
			i2c_send(t&0x80?0xff:0x00);
		}
		t <<= 1;
	}*/
#endif
}

void tetris_draw_hold() {
	u16 t = pieces[(hi+8)&31];
	range(8,23,5,6);
	for ( i8 i = 0; i < 2; i++ ) { for ( i8 j = 0; j < 4; j++ ) {
		u8 p = (t&0x8000?0xf:0)|(t&0x800?0xf0:0);
		for ( i8 k = 0; k < 4; k++ ) { i2c_send(p); }
		t <<= 1;
	} t <<= 4; }
	/*u16 t = hold;
	range(82,114,2,5);
	for ( i8 i = 0; i < 16; i++ ) {
		for ( i8 j = 0; j < 8; j ++ ) {
			i2c_send(t&0x80?0xff:0x00);
		}
		t <<= 1;
	}*/
}

void tetris_draw() {
	tetris_draw_board();
	tetris_draw_next();
	tetris_draw_hold();
}

void tetris_end() {
	while (true) {}
}

void set_timer(char kilocycles) {
	TCCR0A = 0x02; // Count up to kilocycles.
	TCCR0B = 0x05; // Timer clock every 1024 cycles.
	OCR0A = kilocycles;
}

int main() {
	i2c_init();
	i2c_seq(oled_init, sizeof(oled_init));
	//for (int i = 0; i < 128*8; ++i) i2c_send(0);
	seed = 4;
	tetris_init();
/*	tetris_draw();
	tetris_left();
	tetris_right();
	tetris_rot1();
	tetris_rot2();
	tetris_fall();
	tetris_hold();
	tetris_instant_drop();
	*/
	set_timer(100);
	I2C_DDR = OK_LED | ERROR_LED;
	I2C_PORT = OK_LED;
	while (true) {
		if (TIFR & 1) {
			TIFR |= 1;
			I2C_PORT ^= ERROR_LED;
			I2C_PORT ^= OK_LED;
		}
	}
}
