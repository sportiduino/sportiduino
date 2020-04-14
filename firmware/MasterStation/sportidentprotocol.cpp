#include "sportidentprotocol.h"

#define STX 0x02
#define ETX 0x03
#define ACK 0x06
#define NAK 0x15

static uint16_t crc16(uint8_t *data, uint16_t len)
{
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

uint8_t *SportidentProtocol::read(bool *error, uint8_t *code, uint8_t *dataSize) {
    *error = false;
    if(Serial.available() > 0) {
        uint8_t b = Serial.peek();
        if(b == STX) {
            memset(serialBuffer, 0, SPORTIDENT_MAX_PACKET_SIZE);
            Serial.readBytes(serialBuffer, 3);
            uint8_t length = serialBuffer[2];

            if(length > SPORTIDENT_MAX_PACKET_SIZE - 6) {
                *error = true;
                Serial.write(NAK);
                return nullptr;
            }
            Serial.readBytes(&serialBuffer[3], length + 3);
            crc.b[1] = serialBuffer[length + 3];
            crc.b[0] = serialBuffer[length + 4];

            if(crc.value != crc16(&serialBuffer[1], 2 + length)) {
                *error = true;
                Serial.write(NAK);
                return nullptr;
            }
            *code = serialBuffer[1];
            *dataSize = length;
            return &serialBuffer[3];
        }
    }
    return nullptr;
}

