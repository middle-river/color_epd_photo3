// E-ink rendering library for 13.3 inch 6-color Spectra 6 e-Paper.
// 2025-01-07  T. Nakagawa

#ifndef EPD_H_
#define EPD_H_

#include <SPI.h>

class EPD {
public:
  static constexpr const int WIDTH = 1200 / 2;
  static constexpr const int HEIGHT = 1600 * 2;

  enum Color {
    BLACK  = 0b000,
    WHITE  = 0b001,
    YELLOW = 0b010,
    RED    = 0b011,
    BLUE   = 0b101,
    GREEN  = 0b110,
  };

  EPD(int sck_pin, int mosi_pin, int pwr_pin, int busy_pin, int rst_pin, int dc_pin, int csm_pin, int css_pin) : sck_pin_(sck_pin), mosi_pin_(mosi_pin), pwr_pin_(pwr_pin), busy_pin_(busy_pin), rst_pin_(rst_pin), dc_pin_(dc_pin), csm_pin_(csm_pin), css_pin_(css_pin) {
  }

  ~EPD() {
  }

  void begin() {
    pinMode(pwr_pin_, OUTPUT);
    pinMode(busy_pin_, INPUT); 
    pinMode(rst_pin_, OUTPUT);
    pinMode(dc_pin_, OUTPUT);
    pinMode(csm_pin_, OUTPUT);
    pinMode(css_pin_, OUTPUT);
    digitalWrite(pwr_pin_, HIGH);
    digitalWrite(rst_pin_, HIGH);
    digitalWrite(csm_pin_, HIGH);
    digitalWrite(css_pin_, HIGH);
    SPI.begin(sck_pin_, -1, mosi_pin_, -1);
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));

    // HW reset.
    digitalWrite(rst_pin_, LOW);
    delay(30);
    digitalWrite(rst_pin_, HIGH);
    delay(30);
    digitalWrite(rst_pin_, LOW);
    delay(30);
    digitalWrite(rst_pin_, HIGH);
    delay(30);
    wait();

    command(true,  0x74, 9, (const uint8_t []){0xc0, 0x1c, 0x1c, 0xcc, 0xcc, 0xcc, 0x15, 0x15, 0x55});  // AN_TM
    command(false, 0xf0, 6, (const uint8_t []){0x49, 0x55, 0x13, 0x5d, 0x05, 0x10});  // CMD66
    command(false, 0x00, 2, (const uint8_t []){0xdf, 0x69});  // PSR
    command(false, 0x50, 1, (const uint8_t []){0xf7});  // CDI
    command(false, 0x60, 2, (const uint8_t []){0x03, 0x03});  // TCON
    command(false, 0x86, 1, (const uint8_t []){0x10});  // AGID
    command(false, 0xe3, 1, (const uint8_t []){0x22});  // PWS
    command(false, 0xe0, 1, (const uint8_t []){0x01});  // CCSET
    command(false, 0x61, 4, (const uint8_t []){0x04, 0xb0, 0x03, 0x20}); // TRES
    command(true,  0x01, 6, (const uint8_t []){0x0f, 0x00, 0x28, 0x2c, 0x28, 0x38});  // PWR
    command(true,  0xb6, 1, (const uint8_t []){0x07});  // EN_BUF
    command(true,  0x06, 2, (const uint8_t []){0xe8, 0x28});  // BTST_P
    command(true,  0xb7, 1, (const uint8_t []){0x01});  // BOOST_VDDP
    command(true , 0x05, 2, (const uint8_t []){0xe8, 0x28});  // BTST_N
    command(true,  0xb0, 1, (const uint8_t []){0x01});  // BUCK_BOOST_VDDN
    command(true,  0xb1, 1, (const uint8_t []){0x02});  // TFT_VCOM_POWER

    counter_ = 0;
    digitalWrite(csm_pin_, LOW);
    SPI.transfer(0x10);
  }

  void write(uint8_t *data, int size) {
    SPI.transfer(data, size);
    counter_ += size;
    if (counter_ == WIDTH * HEIGHT / 2 / 2) {
      digitalWrite(csm_pin_, HIGH);
      digitalWrite(css_pin_, LOW);
      SPI.transfer(0x10);
    }
  }

  void end() {
    digitalWrite(css_pin_, HIGH);

    command(false, 0x04, 0, nullptr);  // Power On (PON).
    wait();
    delay(50);
    command(false, 0x12, 1, (const uint8_t []){0x00});  // Display Refresh (DRF).
    wait();
    command(false, 0x02, 1, (const uint8_t []){0x00});  // Power OFF (POF).

    command(false, 0x07, 1, (const uint8_t []){0xa5});

    SPI.endTransaction();
    SPI.end();
    digitalWrite(pwr_pin_, LOW);
    digitalWrite(rst_pin_, LOW);
    digitalWrite(csm_pin_, LOW);
    digitalWrite(css_pin_, LOW);
    pinMode(pwr_pin_, INPUT);
    pinMode(rst_pin_, INPUT);
    pinMode(csm_pin_, INPUT);
    pinMode(css_pin_, INPUT);
  }

private:
  int sck_pin_;
  int mosi_pin_;
  int pwr_pin_;
  int busy_pin_;
  int rst_pin_;
  int dc_pin_;
  int csm_pin_;
  int css_pin_;
  int counter_;

  void command(bool master_only, uint8_t cmd, int size, const uint8_t *data) {
    digitalWrite(csm_pin_, LOW);
    if (!master_only) digitalWrite(css_pin_, LOW);
    SPI.transfer(cmd);
    for (int i = 0; i < size; i++) SPI.transfer(data[i]);
    digitalWrite(csm_pin_, HIGH);
    digitalWrite(css_pin_, HIGH);
  }

  void wait() {
    for (int i = 0; i < 50000; i++) {
      delay(10);
      if (digitalRead(busy_pin_) == HIGH) break;
    }
  }
};

#endif
