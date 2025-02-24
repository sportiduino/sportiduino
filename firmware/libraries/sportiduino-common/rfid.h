#ifndef SPORTIDUINO_RFID_H
#define SPORTIDUINO_RFID_H

#include <MFRC522.h>

#define MIN_ANTENNA_GAIN      2
#define MAX_ANTENNA_GAIN      7

#define DEFAULT_ANTENNA_GAIN  4

#define CARD_PAGE_INIT              4
#define CARD_PAGE_INIT_TIME         5
#define CARD_PAGE_LAST_RECORD_INFO  6
#define CARD_PAGE_INFO1             6
#define CARD_PAGE_INFO2             7
#define CARD_PAGE_START             8

#define CARD_PAGE_PASS              5
#define CARD_PAGE_DATE              6
#define CARD_PAGE_TIME              7
#define CARD_PAGE_STATION_NUM       6
#define CARD_PAGE_BACKUP_START      6

#define PAGE_CFG0_OFFSET            2
#define PAGE_CFG1_OFFSET            3
#define PAGE_PWD_OFFSET             4
#define PAGE_PACK_OFFSET            5

enum class CardType : byte {
    UNKNOWN	    = 0,
    ISO_14443_4	= 1,
    ISO_18092   = 2,
    MIFARE_MINI	= 3,
    MIFARE_1K   = 4,
    MIFARE_4K   = 5,
    MIFARE_UL   = 6,
    MIFARE_PLUS	= 7,
    TNP3XXX     = 8,
    NTAG213     = 9,
    NTAG215     = 10,
    NTAG216     = 11
};

typedef struct {
    byte pass[4];
    byte pack[2];
} NtagAuthPassword;

class Rfid {
public:
    void init(uint8_t ssPin, uint8_t rstPin, uint8_t newAntennaGain = DEFAULT_ANTENNA_GAIN);

    void clearLastCardUid();

    void setAntennaGain(uint8_t newAntennaGain);
    void setAuthPassword(uint8_t *password);

    /**
     * Begins to work with RFID module
     * Turn on RC522, detect a card and return the card type if a new one has presented else return 0 if the same card has presented or 0xFF if no any card
     */
    void begin(uint8_t newAntennaGain = 0);

    /**
     * Stops to work with RFID module
     */
    void end();

    /**
     * Returns true if any card has been detected by RFID module
     */
    bool isCardDetected();

    bool isNewCardDetected();

    bool cardWrite(uint8_t startPageAdr, const byte *data, uint16_t size);

    bool cardErase(uint8_t beginPageAddr, uint8_t endPageAddr);

    bool cardPageErase(uint8_t pageAddr);

    bool cardErase4Pages(uint8_t pageAddr);

    /**
     * Reads data from a card page. Buffer size should be 4 bytes!
     */
    bool cardPageRead(uint8_t pageAdr, byte *data, uint8_t size = 4);

    /**
     * Writes data to a card page. Buffer size should be 4 bytes!
     */
    bool cardPageWrite(uint8_t pageAdr, const byte *data, uint8_t size = 4);

    bool cardPageWrite(uint8_t pageAdr, uint32_t value);

    /**
     * Returns max page address of the presented card
     */
    uint8_t getCardMaxPage();

    /**
     * Returns type of the card
     */
    CardType getCardType();

    bool cardEnableDisableAuthentication(bool writeProtection, bool readProtection = false);

private:
    // data buffer size should be greater 18 bytes
    bool mifareCardPageRead(uint8_t pageAdr, byte *data, byte *size);
    // data buffer size should be greater 16 bytes
    bool mifareCardPageWrite(uint8_t pageAdr, byte *data, byte size);
    // data buffer size should be greater 18 bytes
    bool ntagCard4PagesRead(uint8_t pageAdr, byte *data, byte *size);
    bool ntagTryAuth();
    bool ntagAuth(NtagAuthPassword *password);
    bool ntagSetPassword(NtagAuthPassword *password, bool readAndWrite, uint8_t negAuthAttemptsLim, uint8_t startPage);
    bool ntagDisableAuthentication();
    // data buffer size should be greater 4 bytes
    bool ntagCardPageWrite(uint8_t pageAdr, byte *data, byte size);

    MFRC522 mfrc522;
    NtagAuthPassword authPwd;
    MFRC522::Uid lastCardUid;
    uint8_t rfidSsPin = 0;
    uint8_t rfidRstPin = 0;
    uint8_t antennaGain = 0;
    CardType cardType = CardType::UNKNOWN;
    bool authenticated = false;
};

#endif // SPORTIDUINO_RFID_H

