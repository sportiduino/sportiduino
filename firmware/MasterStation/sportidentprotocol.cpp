#include "sportidentprotocol.h"

void SiTimestamp::fromUnixtime(uint32_t timestamp) {
    if(!timestamp) {
        return;
    }
    uint32_t daysFromEpoch = timestamp/86400;
    uint32_t secsFromMidnight = timestamp%86400;
    uint8_t weekday = (daysFromEpoch + 4)%7;
    ptd = weekday << 1;
    ptd |= secsFromMidnight/43200;
    uint16_t pt = secsFromMidnight%43200;
    pth = pt >> 8;
    ptl = pt & 0xff;
}

uint16_t SportidentProtocol::crc16(uint8_t *data, uint16_t len) {
    if(len < 2) {
        return 0;     // response value is "0" for none or one data byte
    }
    uint8_t *p = data;

    uint16_t crc = *p++;
    crc = (crc << 8) + *p++;

    if(len == 2) {
        return crc;   // response value is CRC for two data bytes
    }
    
    uint16_t val;
    for(uint16_t i = (int)(len >> 1); i > 0; --i) {
        if(i > 1) {
            val = *p++;
            val = (val << 8) + *p++;
        } else {
            if(len & 1) { // odd number of data bytes, complete with "0"         
                val = *p;
                val = (val << 8);
            } else {
                val = 0;
            }
        }
        
        for(uint8_t j = 0; j<16; ++j) {
            if(crc & 0x8000) {
                crc <<= 1;
                if(val & 0x8000) {
                    ++crc;
                }
                crc ^= 0x8005;
            } else {
                crc <<= 1;
                if(val & 0x8000) {
                          ++crc;
                      }
            }
            val <<= 1;
        }
    }
    return crc;
} 

void SportidentProtocol::start(uint8_t code) {

    serialBuffer[0] = STX;
    serialBuffer[1] = code;

    const uint16_t station_code = 0x0001;

    // Legacy protocol
    if(code < 0x80) {
        baseCmd = true;
        serialDataPos = 2;
        add(station_code & 0xff);
    } else {
        baseCmd = false;
        serialDataPos = 3;
        add(station_code >> 8);
        add(station_code & 0xff);
    }
}

void SportidentProtocol::add(uint8_t dataByte) {
    if(serialDataPos < SPORTIDENT_MAX_PACKET_SIZE - 1) {
        if(baseCmd && dataByte < 0x1F) {
            serialBuffer[serialDataPos++] = DLE;
        }
        serialBuffer[serialDataPos++] = dataByte;
    }
}

void SportidentProtocol::add(const uint8_t *data, uint8_t size) {
    for(uint8_t i = 0; i < size; ++i) {
        add(data[i]);
    }
}

void SportidentProtocol::send() {
    if(baseCmd) {
        serialBuffer[serialDataPos++] = ETX;
    } else {
        uint8_t dataSize = serialDataPos - 3; // minus start, resp code, datalen
        
        serialBuffer[2] = dataSize;

        crc.value = SportidentProtocol::crc16(&serialBuffer[1], 2 + dataSize);
        serialBuffer[serialDataPos++] = crc.b[1];
        serialBuffer[serialDataPos++] = crc.b[0];
        serialBuffer[serialDataPos++] = ETX;
    }

    for(uint8_t i = 0; i < serialDataPos; i++) {
        Serial.write(serialBuffer[i]);
    }
}

void SportidentProtocol::error() {
    Serial.write(NAK);
}

uint8_t *SportidentProtocol::read(bool *error, uint8_t *code, uint8_t *dataSize) {
    *error = false;
    if(Serial.available() > 0) {
        uint8_t b = Serial.peek();
        if(b == ACK) {
            Serial.read(); // drop byte
            *code = ACK;
            *dataSize = 0;
            return code;
        } else if(b == STX) {
            Serial.readBytes(serialBuffer, 3);
            if(serialBuffer[1] == STX) {
                serialBuffer[1] = serialBuffer[2];
                Serial.readBytes(&serialBuffer[2], 1);
            }

            uint8_t cmd = serialBuffer[1];

            // Legacy protocol
            if(cmd < 0x80) {
                legacyMode = true;
                uint8_t length = 0;
                for(uint8_t i = 2; i < SPORTIDENT_MAX_PACKET_SIZE; ++i) {
                    Serial.readBytes(&serialBuffer[i], 1);
                    ++length;
                    if(serialBuffer[i] == DLE) {
                        Serial.readBytes(&serialBuffer[i], 1);
                    } else if(serialBuffer[i] == ETX) {
                        --length;
                        break;
                    }
                }
                *code = cmd;
                *dataSize = length;
                return &serialBuffer[2];
            }
            legacyMode = false;

            uint8_t length = serialBuffer[2];

            if(length > SPORTIDENT_MAX_PACKET_SIZE - 6) {
                *error = true;
                return nullptr;
            }
            memset(&serialBuffer[3], 0, length + 3);
            Serial.readBytes(&serialBuffer[3], length + 3);
            crc.b[1] = serialBuffer[length + 3];
            crc.b[0] = serialBuffer[length + 4];

            if(crc.value != SportidentProtocol::crc16(&serialBuffer[1], 2 + length)) {
                *error = true;
                return nullptr;
            }
            *code = cmd;
            *dataSize = length;
            return &serialBuffer[3];
        }
    }
    return nullptr;
}

