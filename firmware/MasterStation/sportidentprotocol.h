#ifndef SPORTIDENTPROTOCOL_H
#define SPORTIDENTPROTOCOL_H

#include <Arduino.h>

#define SPORTIDENT_MAX_PACKET_SIZE 140


typedef union {
    uint16_t value;
    byte b[2];
} Crc;

class SiTimestamp {
public:
    void fromUnixtime(uint32_t timestamp);

    uint8_t ptd = 0xEE;
    uint8_t cn = 0xEE;
    uint8_t pth = 0xEE;
    uint8_t ptl = 0xEE;
};

class SportidentProtocol {
public:
    enum SpecialBytes {
        STX     = 0x02,
        ETX     = 0x03,
        ACK     = 0x06,
        DLE     = 0x10,
        NAK     = 0x15,
        WAKEUP  = 0xff
    };

    enum Commands {
        BCMD_SET_MS       = 0x70,
        BCMD_GET_SYS_VAL  = 0x73,
        CMD_SET_MS        = 0xF0,
        CMD_GET_TIME      = 0xF7,
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
    static uint16_t crc16(uint8_t *data, uint16_t len);

private:
    Crc crc;
    uint8_t serialBuffer[SPORTIDENT_MAX_PACKET_SIZE];
    uint8_t serialDataPos = 3;
    bool legacyMode = false;
};



#endif

