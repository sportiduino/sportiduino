#ifndef SPORTIDUINO_H
#define SPORTIDUINO_H

#include <arduino.h>

#define MIN_ANTENNA_GAIN      2<<4
#define MAX_ANTENNA_GAIN      7<<4

#define DEFAULT_ANTENNA_GAIN  4<<4

#define EEPROM_STATION_NUM_ADDR     0x3EE
#define EEPROM_PASS_ADDR            0x3F1
#define EEPROM_SETTINGS_ADDR        0x3FA
#define EEPROM_ANTENNA_GAIN_ADDR    0x3FD

// 6 hours = 2160000 milliseconds
#define WAIT_PERIOD1              21600000L
// 24 hours = 86400000 milliseconds
#define WAIT_PERIOD2              86400000L

// Settings
// Bit1,Bit0 - Mode
// (0,0) - be active WAIT_PERIOD1 and to get in wait mode after
// (0,1) - be active WAIT_PERIOD2 and to get in wait mode after
// (1,0) - always be in wait mode (check card in 1 second period)
// (1,1) - always be in active mode (check card in 0.25 second period)
// Bit2 - Check start/finish station marks on a participant card (0 - no, 1 - yes)
// Bit3 - Check init time of a participant card (0 - no, 1 - yes)
// Bit4 - Clean settings after getting in sleep mode (0 - no, 1 - yes)
// Bit5 - Fast mark mode (0 - no, 1 - yes)
// Bit7 - Flag, settings is valid (0 - yes, 1 - no)

#define SETTINGS_INVALID                0x80
#define SETTINGS_WAIT_PERIOD1           0x0
#define SETTINGS_WAIT_PERIOD2           0x1
#define SETTINGS_ALWAYS_WAIT            0x2
#define SETTINGS_ALWAYS_ACTIVE          0x3
#define SETTINGS_CHECK_START_FINISH     0x4
#define SETTINGS_CHECK_CARD_TIME        0x8
#define SETTINGS_CLEAR_ON_SLEEP         0x10
#define SETTINGS_FAST_MARK              0x20

// OR MASK of SETTINGS macros, for example, SETTINGS_WAIT_PERIOD1 | SETTINGS_CHECK_START_FINISH
#define DEFAULT_SETTINGS  SETTINGS_WAIT_PERIOD1

#define START_STATION_NUM         240
#define FINISH_STATION_NUM        245
#define CHECK_STATION_NUM         248
#define CLEAR_STATION_NUM         249

#define MASTER_CARD_GET_INFO        249
#define MASTER_CARD_SET_TIME        250
#define MASTER_CARD_SET_NUMBER      251
#define MASTER_CARD_SLEEP           252
#define MASTER_CARD_READ_DUMP       253
#define MASTER_CARD_SET_PASS        254

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
 * Reads password and settings from EEPROM
 * @return false if settings are invalid
 */
bool readPwdSettings();

/**
 * Begins to work with RFID module
 * Turn on RC522, detect a card and return the card type if a new one has presented else return 0 if the same card has presented or 0xFF if no any card
 * cardType is MFRC522::PICC_Type
 * PICC_TYPE_UNKNOWN		,
 * PICC_TYPE_ISO_14443_4	,	// PICC compliant with ISO/IEC 14443-4 
 * PICC_TYPE_ISO_18092		, 	// PICC compliant with ISO/IEC 18092 (NFC)
 * PICC_TYPE_MIFARE_MINI	,	// MIFARE Classic protocol, 320 bytes
 * PICC_TYPE_MIFARE_1K		,	// MIFARE Classic protocol, 1KB
 * PICC_TYPE_MIFARE_4K		,	// MIFARE Classic protocol, 4KB
 * PICC_TYPE_MIFARE_UL		,	// MIFARE Ultralight or Ultralight C
 * PICC_TYPE_MIFARE_PLUS	,	// MIFARE Plus
 * PICC_TYPE_MIFARE_DESFIRE,	// MIFARE DESFire
 * PICC_TYPE_TNP3XXX		,	// Only mentioned in NXP AN 10833 MIFARE Type Identification Procedure
 * PICC_TYPE_NOT_COMPLETE	= 0xff	// SAK indicates UID is not complete.
 * NTAG_213 = 0x12
 * NTAG_215 = 0x3E
 * NTAG_216 = 0x6D
 
 */
void rfidBegin(uint8_t ssPin, uint8_t rstPin);

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
uint8_t rfidGetCardType();

/**
 * Sets new password and save it in EEPROM
 */
void setPwd(uint8_t pwd0, uint8_t pwd1, uint8_t pwd2);

/**
 * Returns password byte
 */
uint8_t getPwd(uint8_t n);

/**
 * Sets new settings and save it in EEPROM
 */
void setSettings(uint8_t value);

/**
 * Returns current settings
 */
uint8_t getSettings();

/**
 * Sets new antenna gain of RC522 and save it in EEPROM
 */
void setAntennaGain(uint8_t gain);

/**
 * Returns current antenna gain of RC522
 */
uint8_t getAntennaGain();

#endif