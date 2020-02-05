#ifndef SPORTIDUINO_H
#define SPORTIDUINO_H

#include <Arduino.h>
#include "rfid.h"

#define START_STATION_NUM         240
#define FINISH_STATION_NUM        245
#define CHECK_STATION_NUM         248
#define CLEAR_STATION_NUM         249

#define MASTER_CARD_GET_INFO        249
#define MASTER_CARD_SET_TIME        250
#define MASTER_CARD_SET_NUMBER      251
#define MASTER_CARD_SLEEP           252
#define MASTER_CARD_READ_DUMP       253
#define MASTER_CARD_CONFIG          254

/**
 * Writes data with a majority backup in three cells of EEPROM
 */
void majEepromWrite (uint16_t adr, uint8_t val);

/**
 * Reads data with a majority backup from EEPROM
 */
uint8_t majEepromRead(uint16_t adr);

/**
 * Turn on led and buzzer for given ms n times
 * @param freq the frequency of your buzzer if you have solded the buzzer without a generator else 0
 */
void beep_w(const uint8_t ledPin, const uint8_t buzPin, uint16_t freq, uint16_t ms, uint8_t n);

bool uint32ToByteArray(uint32_t value, byte *byteArray);
uint32_t byteArrayToUint32(byte *byteArray);

#endif
