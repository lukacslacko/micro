#define SDA 4
#define SCK 3
#define ADDR 0x76

#define LED_PIN 1

void fail() {
  pinMode(LED_PIN, OUTPUT);
  while (1) {
    digitalWrite(1, 1);
    delay(100);
    digitalWrite(1, 0);
    delay(100);
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  i2c_start();
  i2c_send(0x10);
  digitalWrite(LED_PIN, 0);
  i2c_stop();
  digitalWrite(LED_PIN, 0);
  init_oled();
  digitalWrite(LED_PIN, 1);
  draw_pattern(0x5555);
  draw_pattern(get_byte(0xD0));
  draw_pattern(0xAAAA);
}

void loop() {
  // put your main code here, to run repeatedly:

}

uint8_t get_byte(uint8_t reg) {
  uint8_t buf[1];
  in(reg, 1, buf);
  return buf[0];
}

uint16_t get_word(uint8_t reg) {
  uint8_t buf[2];
  in(reg, 2, buf);
  return buf[0] | (buf[1] << 8);
}

void in(uint8_t reg, uint8_t len, uint8_t* buf) {
  i2c_start();
  i2c_send(ADDR);
  i2c_send(reg);
  i2c_start();
  while (len) {
    --len;
    buf[len] = i2c_read(len > 0);
  }
}

void init_oled() {
  char seq[] = {
    0xae, // SET_DISP
    0xd5, // SET_DISP_CLK_DIV
    0x80, 
    0xa8, // SET_MUX_RATIO
    0x3f, // height - 1
    0xd3, // SET_DISP_OFFSET
    0, 
    0x40,
    0x8d, 0x14, 0x20, 0, 0xa1, 0xc8, 0xda, 0x12, 0x81, 0xcf, 0xd9, 0xf1, 0xdb, 0x40, 0xa4, 0xa6, 0xaf};
  i2c_start();
  i2c_send(0x78);
  i2c_send(0);
  for (int i = 0; i < sizeof(seq); ++i) i2c_send(seq[i]);
  i2c_stop();
}

void draw_pattern(uint16_t v) {
  i2c_start();
  i2c_send(0x78);
  i2c_send(0x40);
  for (char i = 0; i < 16; ++i) {
    for (char j = 0; j < 7; ++j) {
      if (v&1) i2c_send(0x7f);
      else i2c_send(0x00);
    }
    v >>= 1;
    i2c_send(0);
  }
  i2c_stop();
}


void i2c_start() {
  data_low();
  clock_low();
}

void i2c_stop() {
  clock_high();
  data_high();
}

void i2c_send(uint8_t c) {
  for (char i = 7; i >= 0; --i) {
    if (c&(1<<i)) data_high();
    else data_low();
    clock_high();
    clock_low();
  }
  data_high();
  clock_high();
  if (digitalRead(SDA)) fail();
  clock_low();
}

uint8_t i2c_read(bool ack) {
  data_high();
  uint8_t b = 0;
  for (char i = 7; i >= 0; --i) {
    clock_high();
    if (digitalRead(SDA)) {
      b |= (1<<i);
    }
    clock_low();
  }
  if (ack) data_low();
  else data_high();
  clock_high();
  clock_low();
  return b;
}

void data_high() {
  pinMode(SDA, INPUT_PULLUP);
  pause();
}

void data_low() {
  pinMode(SDA, OUTPUT);
  digitalWrite(SDA, 0);
  pause();
}

void clock_low() {
  pinMode(SCK, OUTPUT);
  digitalWrite(SCK, 0);
  pause();
}

void clock_high() {
  pinMode(SCK, INPUT_PULLUP);
  pause();
  // while (digitalRead(SCK) == 0);
}