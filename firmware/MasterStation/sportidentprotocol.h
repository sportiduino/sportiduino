#ifndef SPORTIDENTPROTOCOL_H
#define SPORTIDENTPROTOCOL_H

#include <Arduino.h>

#define SPORTIDENT_MAX_PACKET_SIZE 128

#define STX 0x02
#define ETX 0x03
#define ACK 0x06
#define NAK 0x15

class SportidentProtocol {
public:
    enum Commands {
        BCMD_SET_MS       = 0x70,
        BCMD_GET_SYS_VAL  = 0x73,
        CMD_SET_MS        = 0xf0,
        CMD_GET_SYS_VAL   = 0x83,
        CMD_READ_SI6      = 0xE1, // read out SI-card 6 data block
        CMD_SI6_DETECTED  = 0xE6,
        CMD_SI_REMOVED    = 0xE7
    };

    enum Offsets {
        O_MODE          = 0x71,
        O_PROTO         = 0x74
    };

    void start(uint8_t code);
    void add(uint8_t dataByte);
    void add(const uint8_t *data, uint8_t size);
    void send();
    void error();
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

