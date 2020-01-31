#ifndef SPORTIDUINO_H
#define SPORTIDUINO_H

#include <Arduino.h>

#define MIN_ANTENNA_GAIN      2
#define MAX_ANTENNA_GAIN      7

#define DEFAULT_ANTENNA_GAIN  4

#define START_STATION_NUM         240
#define FINISH_STATION_NUM        245
#define CHECK_STATION_NUM         248
#define CLEAR_STATION_NUM         249

#define MASTER_CARD_GET_INFO        249
#define MASTER_CARD_SET_TIME        250
#define MASTER_CARD_SET_NUMBER      251
#define MASTER_CARD_SLEEP           252
#define MASTER_CARD_READ_DUMP       253
#define MASTER_CARD_SETTINGS        254

#define CARD_PAGE_INIT              4
#define CARD_PAGE_INIT_TIME         5
#define CARD_PAGE_LAST_RECORD_INFO  6
#define CARD_PAGE_INFO1				6
#define CARD_PAGE_INFO2				7
#define CARD_PAGE_START             8

#define CARD_PAGE_PASS				5
#define CARD_PAGE_NEW_PASS			6
#define CARD_PAGE_DATE				6
#define CARD_PAGE_TIME				7
#define CARD_PAGE_STATION_NUM		6
#define CARD_PAGE_DUMP_START		5

enum class CardType : byte {
    UNKNOWN	    = 0,
    ISO_14443_4	= 1,
    ISO_18092   = 2,
    MIFARE_MINI	= 3,
    MIFARE_1K   = 4,
    MIFARE_4K   = 5,
    MIFARE_UL   = 6,
    MIFARE_PLUS	= 7,
    TNP3XXX	    = 8,
    NTAG213     = 9,
    NTAG215     = 10,
    NTAG216     = 11
};

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

/**
 * Begins to work with RFID module
 * Turn on RC522, detect a card and return the card type if a new one has presented else return 0 if the same card has presented or 0xFF if no any card
 */
void rfidBegin(uint8_t ssPin, uint8_t rstPin, uint8_t antennaGain = DEFAULT_ANTENNA_GAIN);

/**
 * Stops to work with RFID module
 */
void rfidEnd();

/**
 * Returns true if any card has been detected by RFID module
 */
bool rfidIsCardDetected();

/**
 * Returns true if the new card has been detected by RFID module
 */
bool rfidIsNewCardDetected();

/**
 * Reads data from a card page. Buffer size should be 4 bytes!
 */
bool rfidCardPageRead(uint8_t pageAdr, byte *data);

/**
 * Writes data to a card page. Buffer size should be 4 bytes!
 */
bool rfidCardPageWrite(uint8_t pageAdr, byte *data);

/**
 * Returns max page address of the presented card
 */
uint8_t rfidGetCardMaxPage();

/**
 * Returns type of the card
 */
CardType rfidGetCardType();

bool uint32ToByteArray(uint32_t value, uint8_t *byteArray);
uint32_t byteArrayToUint32(uint8_t *byteArray);

#endif
