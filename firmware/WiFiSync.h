// Directory Synchronization with HTTP.
// 2025-01-11  T. Nakagawa

#ifndef WIFISYNC_H_
#define WIFISYNC_H_

#include <HTTPClient.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClient.h>

class WiFiSync {
public:
  static constexpr int BUF_SIZE = 1024;
  static constexpr int MAX_FILES = 256;

  static void synchronize(const char *ssid, const char *pass, const char *url, const char *dir, const char *config) {
    if (Serial) Serial.println("WiFiSync started.");
    uint8_t *buf = new uint8_t[BUF_SIZE];
    String *old_names = new String[MAX_FILES];
    String *new_names = new String[MAX_FILES];
    int *old_sizes = new int[MAX_FILES];
    int *new_sizes = new int[MAX_FILES];
    int old_num = 0;
    int new_num = 0;
    HTTPClient http;
    bool ok = true;
    LittleFS.begin();

    // Turn on WiFi.
    if (Serial) Serial.println("Connecting WiFi.");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED && ok) {
      delay(10);
      if (millis() >= 10000) ok = false;
    }

    // Read a configuration file.
    if (ok) {
      Preferences pref;
      pref.begin(config, false);
      if (Serial) Serial.println("Fetching the configuration file.");
      http.begin(url + String("config.txt"));
      if (http.GET() == HTTP_CODE_OK) {
        String page = http.getString();
        while (page.length()) {
          const int end = page.indexOf("\n");
          const int pos = page.indexOf("\t");
          if (end < 0 || pos < 0 || pos >= end) break;
          String key = page.substring(0, pos);
          String val = page.substring(pos + 1, end);
          key.trim();
          val.trim();
          if (Serial) Serial.println("key=" + key + ", val=" + val);
          if (key.length()) pref.putString(key.c_str(), val);
          page = page.substring(end + 1);
        }
      }
      http.end();
      pref.end();
    }

    // Make a list of new files.
    if (ok) {
      if (Serial) Serial.println("Fetching the directory list.");
      http.begin(url);
      if (http.GET() != HTTP_CODE_OK) {
        ok = false;
      } else {
        String page = http.getString();
        while (page.length() && new_num < MAX_FILES) {
          const int end = page.indexOf("\n");
          const int pos = page.indexOf(" ");
          if (end < 0 || pos < 0 || pos >= end) break;
          new_sizes[new_num] = page.substring(0, pos).toInt();
          new_names[new_num] = page.substring(pos + 1, end);
          new_num++;
          page = page.substring(end + 1);
        }
      }
      http.end();
      Serial.println("New files: " + String(new_num));
    }

    // Make a list of old files.
    if (ok) {
      if (Serial) Serial.println("Getting the old files.");
      File root = LittleFS.open(dir);
      if (!root || !root.isDirectory()) {
        ok = false;
      } else {
        File file;
        while ((file = root.openNextFile())) {
          if (!file.isDirectory() && old_num < MAX_FILES) {
            old_names[old_num] = file.name();
            old_sizes[old_num] = file.size();
            old_num++;
          }
          file.close();
        }
      }
      root.close();
      Serial.println("Old files: " + String(old_num));
    }

    // Remove unnecessary old files.
    if (ok) {
      if (Serial) Serial.println("Removing old files.");
      for (int old_idx = 0; old_idx < old_num; old_idx++) {
        int new_idx;
        for (new_idx = 0; new_idx < new_num; new_idx++) if (new_names[new_idx] == old_names[old_idx]) break;
        if (new_idx < new_num && new_sizes[new_idx] == old_sizes[old_idx]) {
          if (Serial) Serial.println("Skipping: " + old_names[old_idx]);
          new_names[new_idx] = new_names[new_num - 1];
          new_sizes[new_idx] = new_sizes[new_num - 1];
          new_num--;
          continue;
        }
        if (Serial) Serial.println("Removing: " + old_names[old_idx]);
        LittleFS.remove(dir + old_names[old_idx]);
      }
    }

    // Download missing new files.
    if (ok) {
      if (Serial) Serial.println("Downloading new files.");
      for (int i = 0; i < new_num; i++) {
        if (Serial) Serial.println("Downloading: " + new_names[i]);
        http.begin(url + new_names[i]);
        if (http.GET() != HTTP_CODE_OK) {
          if (Serial) Serial.println("Failed to download.");
          http.end();
          continue;
        }
        File file = LittleFS.open(dir + new_names[i], FILE_WRITE);
        if (!file) {
          if (Serial) Serial.println("Failed to open the file.");
          http.end();
          continue;
        }
        http.writeToStream(&file);
        file.close();
        http.end();
      }
    }

    // Disable WiFi.
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    LittleFS.end();
    delete[] buf;
    delete[] old_names;
    delete[] new_names;
    delete[] old_sizes;
    delete[] new_sizes;
    if (Serial) Serial.println("WiFiSync finished.");
  }
};

#endif
