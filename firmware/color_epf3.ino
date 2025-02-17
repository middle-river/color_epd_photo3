// 7.3/11.3 inch Color E-Paper Photo Frame.
// 2024-12-01, 2025-01-16  T. Nakagawa

//#define EPD7IN3F
#define EPD13IN3E
#define UPSIDE_DOWN 1

#include <LittleFS.h>
#include <Preferences.h>
#include <cstring>
#include <driver/rtc_io.h>
#include <soc/rtc_cntl_reg.h>
#include "GIF.h"
#include "WiFiConfig.h"
#include "WiFiSync.h"
#include "WiFiTransfer.h"
#if defined(EPD7IN3F)
#include "EPD7IN3F.h"
#elif defined(EPD13IN3E)
#include "EPD13IN3E.h"
#endif

extern "C" int rom_phy_get_vdd33();

#if defined(EPD7IN3F)
constexpr int PIN_SCK = 18;
constexpr int PIN_MOSI = 23;
constexpr int PIN_BUSY = 16;
constexpr int PIN_RST = 17;
constexpr int PIN_DC = 21;
constexpr int PIN_CS = 22;
constexpr int PIN_SW = 0;
constexpr float VOLTAGE_ADJUST = 3.30f / 3.60f;
#elif defined(EPD13IN3E)
constexpr int PIN_SCK = 18;
constexpr int PIN_MOSI = 23;
constexpr int PIN_PWR = 4;
constexpr int PIN_BUSY = 16;
constexpr int PIN_RST = 17;
constexpr int PIN_DC = 21;
constexpr int PIN_CSM = 19;
constexpr int PIN_CSS = 22;
constexpr int PIN_SW = 0;
constexpr float VOLTAGE_ADJUST = 3.30f / 3.81f;
#endif
constexpr uint32_t font[13] = {0x25555520, 0x26222270, 0x25124470, 0x25121520, 0x55571110, 0x74461520, 0x25465520, 0x71122220, 0x25525520, 0x25531520, 0x00000040, 0x11222440, 0x00000000};  // 4x8 font data for digits [0-9./ ].
constexpr float SHUTDOWN_VOLTAGE = 2.30f;
constexpr int PATH_LEN = 32;

Preferences preferences;
#if defined(EPD7IN3F)
EPD epd(PIN_SCK, PIN_MOSI, PIN_BUSY, PIN_RST, PIN_DC, PIN_CS);
#elif defined(EPD13IN3E)
EPD epd(PIN_SCK, PIN_MOSI, PIN_PWR, PIN_BUSY, PIN_RST, PIN_DC, PIN_CSM, PIN_CSS);
#endif
RTC_DATA_ATTR char photo_path[PATH_LEN];
RTC_DATA_ATTR int photo_index;
RTC_DATA_ATTR int low_battery_counter;

float getVoltage() {
  btStart();
  int v = 0;
  for (int i = 0; i < 20; i++) v += rom_phy_get_vdd33();
  btStop();
  v /= 20;
  const float vdd =  (0.0005045f * v + 0.3368f) * VOLTAGE_ADJUST;
  return vdd;
}

String find_path(const char *dir, const String& path_old, int skip, bool reverse, int &total) {
  String list[10];
  int num = 0;
  total = 0;
  File root = LittleFS.open(dir);
  if (root) {
    for (File file = root.openNextFile(); file; file = root.openNextFile()) {
      const String path = file.name();
      file.close();
      if (path.length() > PATH_LEN - 1) continue;
      if (!path.endsWith(".gif")) continue;
      total++;
      int idx = num;
      if (reverse) {
        while (idx > 0 && ((path < path_old && list[idx - 1] >= path_old) || (path < path_old && list[idx - 1] < path_old && path > list[idx - 1]) || (path > path_old && list[idx - 1] > path_old && path > list[idx - 1]))) idx--;
      } else {
        while (idx > 0 && ((path > path_old && list[idx - 1] <= path_old) || (path > path_old && list[idx - 1] > path_old && path < list[idx - 1]) || (path < path_old && list[idx - 1] < path_old && path < list[idx - 1]))) idx--;
      }
      if (idx >= skip) continue;
      if (num < skip) num++;
      for (int i = num - 1; i > idx; i--) list[i] = list[i - 1];
      list[idx] = path;
    }
    root.close();
  }
  return (num == 0) ? "" : list[(skip - 1) % num];
}

void drawText(uint8_t *bitmap, int width, int column, int code) {
  for (int i = 0; i < 16; i++) {  // 4x8 dot font is rendered as 8x16 dot font.
    const int shift = 4 * (7 - i / 2);
    uint32_t bit = 0b1000;
    for (int j = 0; j < 4; j++) {
      bitmap[width / 2 * i + 4 * column + j] = ((font[code] >> shift) & bit) ? (EPD::Color::BLACK << 4 | EPD::Color::BLACK) : (EPD::Color::WHITE << 4 | EPD::Color::WHITE);
      bit >>= 1;
    }
  }
}

void display(int delta, float voltage) {
  if (!LittleFS.begin()) {
    LittleFS.format();
    LittleFS.begin();
    Serial.print("Formatted LittleFS: ");
    Serial.println(LittleFS.totalBytes());
  }

  // Find the file to view.
  String path = String(photo_path);
  Serial.println("Previous photo: " + path);
  if (path == "") photo_index = (delta > 0) ? -1 : 0;
  const int skip = (delta > 0) ? delta : -delta;
  int total;
  path = find_path("/", path, skip, (delta < 0), total);
  path.toCharArray(photo_path, PATH_LEN);
  if (total >= 1) photo_index = (photo_index + delta + total * skip) % total;
  Serial.println("Drawing the photo: " + path);

  // Generate the bitmap for text.
  // Photo index.
  uint8_t bitmap[(40 + 8 + 32) / 2 * 16];  // (5+1+4)*8x1*16 pixels.
  drawText(bitmap, 80,  0, ((photo_index + 1) / 10) % 10);
  drawText(bitmap, 80,  1, ((photo_index + 1) /  1) % 10);
  drawText(bitmap, 80,  2, 11);
  drawText(bitmap, 80,  3, (total / 10) % 10);
  drawText(bitmap, 80,  4, (total /  1) % 10);
  // Battery voltage.
  voltage = 0.01f * (int)(voltage * 100.0f + 0.5f);
  drawText(bitmap, 80, 5, 12);
  drawText(bitmap, 80, 6, (int)(voltage) % 10);
  drawText(bitmap, 80, 7, 10);
  drawText(bitmap, 80, 8, (int)(voltage *  10.0f) % 10);
  drawText(bitmap, 80, 9, (int)(voltage * 100.0f) % 10);

  // Callback function for drawing each line.
  int line = 0;
  auto callback = [bitmap, &line](uint8_t *data, int size) {  // Superimpose function.
#if UPSIDE_DOWN
    if (EPD::HEIGHT - 1 - line < 16) {
      for (int i = 0; i < 80 / 2; i++) data[EPD::WIDTH / 2 - 1 - i] = bitmap[80 / 2 * (EPD::HEIGHT - 1 - line) + i];
    }
#else
    if (line < 16) {
      std::memcpy(data, bitmap + 80 / 2 * line, 80 / 2);
    }
#endif
    epd.write(data, size);
    line++;
  };

  // Draw the picture.
  if (path != "") {
    epd.begin();
    File file = LittleFS.open(("/" + path).c_str(), "r");
    const int status = GIF::read(&file, EPD::WIDTH, EPD::HEIGHT, callback);
    file.close();
    epd.end();
    Serial.println("Status=" + String(status));
  }

  LittleFS.end();
}

int getClicks() {
  constexpr int LONG_CLICK_MS = 1500;
  constexpr int DOUBLE_CLICK_MS = 500;

  uint32_t t = millis();
  while (digitalRead(PIN_SW) == LOW) {
    if (millis() - t >= LONG_CLICK_MS) return 0; // Long click.
    delay(10);
  }
  delay(10);
  t = millis();
  while (digitalRead(PIN_SW) == HIGH) {
    if (millis() - t >= DOUBLE_CLICK_MS) return 1; // Single click.
    delay(10);
  }
  delay(10);
  while (digitalRead(PIN_SW) == LOW) delay(10);
  delay(10);
  t = millis();
  while (digitalRead(PIN_SW) == HIGH) {
    if (millis() - t >= DOUBLE_CLICK_MS) return 2; // Double click.
    delay(10);
  }
  return 3;  // Triple click.
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Disable brown-out detection.
  pinMode(PIN_SW, INPUT);
  Serial.begin(115200);
  while (!Serial) ;
  Serial.println("Color E-Paper Photo Frame");

  // Determine the mode.
  enum Mode {NONE, SYNC, CONFIG, TRANSFER, FORWARD0, BACKWARD0, FORWARD1, BACKWARD1} mode = NONE;
  const esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
  if (reason == ESP_SLEEP_WAKEUP_TIMER) {
    mode = FORWARD0;
  } else if (reason == ESP_SLEEP_WAKEUP_EXT0) {
    const int clicks = getClicks();
    if (clicks == 1) {
      mode = FORWARD0;
    } else if (clicks == 0) {
      mode = BACKWARD0;
    } else if (clicks == 2) {
      mode = FORWARD1;
    } else if (clicks == 3) {
      mode = BACKWARD1;
    }
  } else {
    while (digitalRead(PIN_SW) == HIGH && millis() <= 3000) delay(10);
    if (digitalRead(PIN_SW) == LOW) {
      delay(10);
      const int clicks = getClicks();
      if (clicks == 1) {
        mode = SYNC;
      } else if (clicks == 0) {
        mode = CONFIG;
      } else if (clicks == 2) {
        mode = TRANSFER;
      }
    }
  }
  Serial.println("Mode=" + String(mode));

  // Check the battery voltage.
  const float voltage = getVoltage();
  Serial.println("Battery voltage: " + String(voltage));
  if (voltage < SHUTDOWN_VOLTAGE) {
    low_battery_counter++;
    if (low_battery_counter >= 5) {
      Serial.println("Shutting down due to low battery voltage.");
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
      esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
      esp_deep_sleep_start();
    }
  }
  low_battery_counter = 0;

  preferences.begin("config", true);
  const String ssid = preferences.getString("SSID", "");
  const String pass = preferences.getString("PASS", "");
  const String url = preferences.getString("URL", "/");
  preferences.end();
  if (mode == SYNC) {
    WiFiSync::synchronize(ssid.c_str(), pass.c_str(), url.c_str(), "/", "config");
    photo_path[0] = '\0';
  } else if (mode == CONFIG) {
    WiFiConfig::configure("config");
  } else if (mode == TRANSFER) {
    WiFiTransfer::transfer(ssid.c_str(), pass.c_str(), []() -> bool { return (digitalRead(PIN_SW) == HIGH); });
    photo_path[0] = '\0';
    delay(10);
    while (digitalRead(PIN_SW) == LOW) delay(10);
    delay(10);
  } else if (mode == FORWARD0) {
    display(1, voltage);
  } else if (mode == BACKWARD0) {
    display(-1, voltage);
  } else if (mode == FORWARD1) {
    display(5, voltage);
  } else if (mode == BACKWARD1) {
    display(-5, voltage);
  }

  // Deep sleep.
  preferences.begin("config", true);
  unsigned long long sleep = preferences.getString("SLEEP", "86400").toInt();
  preferences.end();
  Serial.println("Sleeping: " + String((unsigned long)sleep) + "sec.");
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_SW, LOW);
  esp_sleep_enable_timer_wakeup(sleep * 1000000);
  esp_deep_sleep_start();
}

void loop() {
}
