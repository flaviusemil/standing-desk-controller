#include "LoctekProtocol.h"

// Constructor
LoctekProtocol::LoctekProtocol() {}

// Validate the packet structure
bool LoctekProtocol::validatePacket(uint8_t* packet, size_t length) {
    if (packet[0] != 0x9B || packet[length - 1] != 0x9D) {
        return false; // Invalid Start or End byte
    }

    uint16_t crcReceived = (packet[length - 3] << 8) | packet[length - 2];
    uint16_t crcComputed = computeCRC16(packet + 1, length - 4);

    return crcReceived == crcComputed;
}

// Compute CRC16 Modbus
uint16_t LoctekProtocol::computeCRC16(uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

String LoctekProtocol::decodePayload(uint8_t* payload, size_t payloadLength) {
    if (payload == nullptr || payloadLength == 0) {
        Serial.println("Payload is empty or null.");
        return "";
    }

    String result = "";
    for (size_t i = 0; i < payloadLength; i++) {
        result += decodeSegmentByte(payload[i]);
    }

    return result;
}

String LoctekProtocol::decodeSegmentByte(uint8_t segmentByte) {
    uint8_t digit = sevenSegmentToDigit(segmentByte & 0x7F);
    bool hasDot = segmentByte & 0x80;

    return String((char)digit) + (hasDot ? "." : "");
}

uint8_t LoctekProtocol::sevenSegmentToDigit(uint8_t segmentByte) {
    switch (segmentByte) {
        case 0x3F: return '0';
        case 0x06: return '1';
        case 0x5B: return '2';
        case 0x4F: return '3';
        case 0x66: return '4';
        case 0x6D: return '5';
        case 0x7D: return '6';
        case 0x07: return '7';
        case 0x7F: return '8';
        case 0x6F: return '9';
        default: return '?';
    }
}

bool LoctekProtocol::parsePacket(uint8_t* packet, size_t length) {
    if (!validatePacket(packet, length)) {
        return false; // Invalid packet
    }

    uint8_t type = packet[2];
    uint8_t* payload = packet + 3;
    size_t payloadLength = packet[1] - 4; // Length excludes Start, End, Type, and Checksum

    if (type == 0x12) { // Height message type
        String decoded = decodePayload(payload, payloadLength);
        Serial.println("Height: " + decoded);
        return true;
    }

    Serial.println("Unknown message type");
    return false;
}
