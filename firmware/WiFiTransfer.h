// File transfer with FTP.
// 2024-01-14  T. Nakagawa

#ifndef WIFITRANSFER_H_
#define WIFITRANSFER_H_

#include <LittleFS.h>
#include <SimpleFTPServer.h>

class WiFiTransfer {
public:
  static void transfer(const char *ssid, const char *pass, const std::function<bool()>& callback) {
    auto ftp_callback = [](FtpOperation ftpOperation, unsigned int freeSpace, unsigned int totalSpace) {
      switch (ftpOperation) {
      case FTP_CONNECT:
        if (Serial) Serial.println("Client connected.");
        break;
      case FTP_DISCONNECT:
        if (Serial) Serial.println("Client disconnected.");
        break;
      default:
        break;
      }
    };
    auto ftp_transfer_callback = [](FtpTransferOperation ftpOperation, const char* name, unsigned int transferredSize) {
      switch (ftpOperation) {
      case FTP_UPLOAD_START:
        if (Serial) Serial.println("File upload started.");
        break;
      case FTP_TRANSFER_STOP:
        if (Serial) Serial.println("File transfer finished.");
        break;
      default:
        break;
      }
    };

    if (Serial) Serial.println("FTP started.");
    WiFi.mode(WIFI_STA);
    if (Serial) Serial.println("Connecting: " + String(ssid));
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    if (Serial) Serial.print("connected: ");
    if (Serial) Serial.println(WiFi.localIP());

    LittleFS.begin(true);
    if (Serial) Serial.println("Total bytes: " + String(LittleFS.totalBytes()));
    if (Serial) Serial.println("Used bytes: " + String(LittleFS.usedBytes()));
    FtpServer ftp;
    ftp.setCallback(ftp_callback);
    ftp.setTransferCallback(ftp_transfer_callback);
    ftp.begin();  // FTP for anonymous connection.
    while (callback()) ftp.handleFTP();
    ftp.end();
    if (Serial) Serial.println("Total bytes: " + String(LittleFS.totalBytes()));
    if (Serial) Serial.println("Used bytes: " + String(LittleFS.usedBytes()));
    LittleFS.end();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    if (Serial) Serial.println("FTP ended.");
  }
};

#endif
