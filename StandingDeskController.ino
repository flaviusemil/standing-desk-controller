#include "LoctekProtocol.h"

LoctekProtocol parser;

#define displayPin20 23

enum PacketType {
    BUTTON_PRESS = 0x02,
    RECEIVE_ENABLED_FLAG = 0x11,
    HEIGTH = 0x12,
    MAYBE_RECEIVE_DISABLED_FLAG = 0x13,
    UNKNOWN_COMMAND3 = 0x15
};

const char* getPacketTypeName(enum PacketType type) {
    switch (type) {
        case BUTTON_PRESS:
            return "BUTTON_PRESS";
        case HEIGTH:
            return "HEIGTH";
        case RECEIVE_ENABLED_FLAG:
            return "RECEIVE_ENABLED_FLAG";
        case MAYBE_RECEIVE_DISABLED_FLAG:
            return "MAYBE_RECEIVE_DISABLED_FLAG";
        case UNKNOWN_COMMAND3:
            return "UNKNOWN_COMMAND3";
        default:
            return "UNKNOWN";
    }
}

struct Packet {
    uint8_t startByte = 0x9B;       // Start byte (e.g., 0x9B)
    uint8_t length;          // Length of the packet
    enum PacketType type;            // Message type (e.g., 0x12 for height)
    byte payload[3];     // Payload (adjust size to match max payload length)
    byte checksum[2];       // CRC16 checksum
    uint8_t endByte = 0x9D;         // End byte (e.g., 0x9D)
} packet;

const byte wakeup[] = { 0x9b, 0x06, 0x02, 0x00, 0x00, 0x6c, 0xa1, 0x9d };
const byte command_up[] = { 0x9b, 0x06, 0x02, 0x01, 0x00, 0xfc, 0xa0, 0x9d };
const byte command_down[] = { 0x9b, 0x06, 0x02, 0x02, 0x00, 0x0c, 0xa0, 0x9d };
const byte command_m[] = { 0x9b, 0x06, 0x02, 0x20, 0x00, 0xac, 0xb8, 0x9d };
const byte command_preset_1[] = { 0x9b, 0x06, 0x02, 0x04,
                                  0x00, 0xac, 0xa3, 0x9d };
const byte command_preset_2[] = { 0x9b, 0x06, 0x02, 0x08,
                                  0x00, 0xac, 0xa6, 0x9d };
const byte command_preset_3[] = { 0x9b, 0x06, 0x02, 0x10,
                                  0x00, 0xac, 0xac, 0x9d };
const byte command_preset_4[] = { 0x9b, 0x06, 0x02, 0x00,
                                  0x01, 0xac, 0x60, 0x9d };

void setup() {
  Serial.begin(115200, SERIAL_8N1);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("Booting");

  pinMode(displayPin20, OUTPUT);
  digitalWrite(displayPin20, LOW);

  Serial.println("Ready");

  // demo();
}

void demo() {
  sit();
  delay(10000);

  stand();
  delay(10000);

  wake();
}

void turn_on() {
  Serial.println("sending turn on command");
  digitalWrite(displayPin20, HIGH);
  delay(650);
  digitalWrite(displayPin20, LOW);
}

void wake() {
  turn_on();

  // This will allow us to receive the current height
  Serial.println("sending wakeup command");
  Serial2.flush();
  Serial2.write(wakeup, sizeof(wakeup));
}

void sit() {
  turn_on();

  Serial.println("sending sit preset (preset_1)");
  Serial2.flush();
  Serial2.write(command_preset_1, sizeof(command_preset_1));
}

void stand() {
  turn_on();

  Serial.println("sending stand preset (preset_2)");
  Serial2.flush();
  Serial2.write(command_preset_2, sizeof(command_preset_2));
}

void loop() {
  static bool isReading = false;
  static byte buffer[9];
  static int bufferIndex = 0;

  while (Serial2.available()) {
    byte incomingByte = Serial2.read();

    if (incomingByte == packet.startByte) {
      isReading = true;
      bufferIndex = 0;
      buffer[bufferIndex++] = incomingByte;
    } else if (isReading) {
      if (bufferIndex < sizeof(buffer)) {
        buffer[bufferIndex++] = incomingByte;
      } else {
        isReading = false;
        bufferIndex = 0;
        Serial.println("Buffer overflow, resetting...");
        continue;
      }

      if (incomingByte == packet.endByte) {
        isReading = false;
        processPackage(buffer, bufferIndex);
      }
    }
  }
}

const bool shouldLogPacketType(enum PacketType type) {
  switch(type) {
    case BUTTON_PRESS:
      return true;
    case HEIGTH:
      return false;
    case RECEIVE_ENABLED_FLAG:
      return false;
    case MAYBE_RECEIVE_DISABLED_FLAG:
    case UNKNOWN_COMMAND3:
    default:
      return true;
  }
}

const bool isPayloadEmpty(const byte *packet) {
  size_t payloadSize = packet[1] - 4;
  for (int i = 0; i < payloadSize; i++) {
    if (packet[i + 3]) {
      return false;
    }
  }
  return true;
}

void processPackage(const byte *data, int length) {
  if (parser.parsePacket((uint8_t*)data, length)) {
    packet.length = data[1];
    packet.type = (enum PacketType)data[2];
    size_t payloadSize = packet.length - 4; // Len is made out of Type (1 Byte), Payload (0-3 Bytes), Checksum (2 Bytes) and End Byte (1 Byte) - if you remove the 4 bytes you get the payload len
    for (int i = 0; i < payloadSize; i++) {
      packet.payload[i] = data[3 + i]; // offset + index of payload
    }
    packet.checksum[0] = data[length - 3];
    packet.checksum[1] = data[length - 2];

    if (shouldLogPacketType(packet.type)) {
      if (packet.type == HEIGTH && isPayloadEmpty(data)) {
        return;
      }

      Serial.print("Received packet type ");
      Serial.print(getPacketTypeName(packet.type));
      Serial.print(": value: ");
      Serial.println(parser.decodePayload(packet.payload, payloadSize));

      for (int i = 0; i < length; i++) {
        Serial.print(data[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}
