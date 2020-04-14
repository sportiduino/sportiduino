#ifndef SPORTIDENTPROTOCOL_H
#define SPORTIDENTPROTOCOL_H

#include <Arduino.h>

#define SPORTIDENT_MAX_PACKET_SIZE 128

class SportidentProtocol {
public:
    void start(uint8_t code);
    void add(uint8_t dataByte);
    void add(const uint8_t *data, uint8_t size);
    void send();
    uint8_t *read(bool *error, uint8_t *code, uint8_t *dataSize);

private:
    union {
        uint16_t value;
        byte b[2];
    } crc;
    uint8_t serialBuffer[SPORTIDENT_MAX_PACKET_SIZE];
    uint8_t serialDataPos = 3;
};



#endif

