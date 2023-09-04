#include "GM65_scanner.h"

GM65_scanner::GM65_scanner(Stream* serial) {
  mySerial = serial;
}

bool GM65_scanner::writeCommand(const char* command, size_t len) {
  mySerial->write(command, len);
  mySerial->flush();
  return mySerial->availableForWrite() == 0;
}

int GM65_scanner::readResponse(char* buffer, size_t bufferSize) {
  int bytesRead = 0;
  while (mySerial->available() && bytesRead < bufferSize) {
    buffer[bytesRead] = mySerial->read();
    bytesRead++;
  }
  return bytesRead;
}

void GM65_scanner::waitForResponse(int timeout) {
  unsigned long startTime = millis();
  while (millis() - startTime < timeout) {
    if (mySerial->available()) {
      break;
    }
  }
}

bool GM65_scanner::isResponseOK(const char* expectedResponse, int timeout) {
  char buffer[64];
  int bytesRead = readResponse(buffer, sizeof(buffer));
  if (bytesRead > 0) {
    buffer[bytesRead] = '\0';
    if (strcmp(buffer, expectedResponse) == 0) {
      return true;
    }
  }
  return false;
}

void GM65_scanner::init() {
  writeCommand("\x7E\x00\x08\x01\x00\xD9\x55\xAB\xCD", 9); // Set default
  waitForResponse(10000);
  writeCommand("\x7E\x00\x08\x01\x00\x0D\x00\xAB\xCD", 9); // Set serial output
  waitForResponse(1000);
}

void GM65_scanner::enableSettingCode() {
  writeCommand("\x7E\x00\x08\x01\x00\x03\x01\xAB\xCD", 9);
  waitForResponse(1000);
}

void GM65_scanner::disableSettingCode() {
  writeCommand("\x7E\x00\x08\x01\x00\x03\x03\xAB\xCD", 9);
  waitForResponse(1000);
}

int GM65_scanner::getMode(byte addr1, byte addr2) {
  byte read_reg[9] = {0x7E, 0x00, 0x07, 0x01, 0x00, 0x00, addr1, addr2, 0xAB};
  writeCommand(reinterpret_cast<const char*>(read_reg), 9);
  waitForResponse(1000);
  int* p = getResponse();
  return *(p + 4);
}

int* GM65_scanner::getResponse() {
  static int buf[20];
  int count = 0;
  if (mySerial->available() > 0) {
    while (mySerial->available()) {
      buf[count] = mySerial->read();
      count++;
    }
    return buf;
  }
  return nullptr;
}

void GM65_scanner::clearBuffer() {
  if (mySerial->available()) {
    while (mySerial->available() > 0) {
      char temp = mySerial->read();
    }
  }
}

void GM65_scanner::setSilentMode(uint8_t silentMode) {
  int currentMode = getMode(0x00, 0x00);
  int temp = ~(1ul << 6) & currentMode;
  byte modeData = temp + (silentMode << 6);
  char modeCommand[9] = {0x7E, 0x00, 0x08, 0x01, 0x00, 0x00, modeData, 0xAB, 0xCD};
  writeCommand(modeCommand, 9);
}

void GM65_scanner::setLEDMode(uint8_t LEDMode) {
  int currentMode = getMode(0x00, 0x00);
  int temp = ~(1ul << 7) & currentMode;
  byte modeData = temp + (LEDMode << 7);
  char modeCommand[9] = {0x7E, 0x00, 0x08, 0x01, 0x00, 0x00, modeData, 0xAB, 0xCD};
  writeCommand(modeCommand, 9);
}

void GM65_scanner::setWorkingMode(uint8_t workingMode) {
  int currentMode = getMode(0x00, 0x00);
  int temp = ~(0b11ul) & currentMode;
  byte modeData = temp + workingMode;
  char modeCommand[9] = {0x7E, 0x00, 0x08, 0x01, 0x00, 0x00, modeData, 0xAB, 0xCD};
  writeCommand(modeCommand, 9);
}

void GM65_scanner::setLightMode(uint8_t lightMode) {
  int currentMode = getMode(0x00, 0x00);
  int temp = ~(0b11ul << 2) & currentMode;
  byte modeData = temp + (lightMode << 2);
  char modeCommand[9] = {0x7E, 0x00, 0x08, 0x01, 0x00, 0x00, modeData, 0xAB, 0xCD};
  writeCommand(modeCommand, 9);
}

void GM65_scanner::setAimMode(uint8_t aimMode) {
  int currentMode = getMode(0x00, 0x00);
  int temp = ~(0b11ul << 4) & currentMode;
  byte modeData = temp + (aimMode << 4);
  char modeCommand[9] = {0x7E, 0x00, 0x08, 0x01, 0x00, 0x00, modeData, 0xAB, 0xCD};
  writeCommand(modeCommand, 9);
}

void GM65_scanner::scanOnce() {
  writeCommand("\x7E\x00\x08\x01\x00\x02\x01\xAB\xCD", 9);
}

void GM65_scanner::setSleepMode(uint8_t sleepMode) {
  int currentMode = getMode(0x00, 0x07);
  int temp = ~(0b1ul << 7) & currentMode;
  byte modeData = temp + (sleepMode << 7);
  char modeCommand[9] = {0x7E, 0x00, 0x08, 0x01, 0x00, 0x07, modeData, 0xAB, 0xCD};
  writeCommand(modeCommand, 9);
}

String GM65_scanner::getInfo() {
  String s = "";
  if (mySerial->available() > 0) {
    while (mySerial->available()) {
      s += char(mySerial->read());
    }
  }
  return s;
}
