#ifndef LoctekProtocol_h
#define LoctekProtocol_h

#include <Arduino.h>

class LoctekProtocol {
public:
  LoctekProtocol();
  bool parsePacket(uint8_t* packet, size_t length);
  uint16_t computeCRC16(uint8_t* data, size_t length);
  String decodePayload(uint8_t* payload, size_t payloadLength);

private:
  bool validatePacket(uint8_t* packet, size_t length);
  String decodeSegmentByte(uint8_t segmentByte);
  uint8_t sevenSegmentToDigit(uint8_t segmentByte);
};

#endif
