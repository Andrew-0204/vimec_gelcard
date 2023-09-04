#ifndef GM65_SCANNER_H
#define GM65_SCANNER_H

#include "Arduino.h"
#include "Stream.h"

class GM65_scanner {
  private:
    Stream* mySerial;

    bool writeCommand(const char* command, size_t len);
    int readResponse(char* buffer, size_t bufferSize);
    void waitForResponse(int timeout);
    bool isResponseOK(const char* expectedResponse, int timeout);

  public:
    GM65_scanner(Stream* serial);

    void init();
    void enableSettingCode();
    void disableSettingCode();
    int getMode(byte addr1, byte addr2);
    int* getResponse();
    void clearBuffer();
    void setSilentMode(uint8_t silentMode);
    void setLEDMode(uint8_t LEDMode);
    void setWorkingMode(uint8_t workingMode);
    void setLightMode(uint8_t lightMode);
    void setAimMode(uint8_t aimMode);
    void scanOnce();
    void setSleepMode(uint8_t sleepMode);
    String getInfo();
};

#endif
