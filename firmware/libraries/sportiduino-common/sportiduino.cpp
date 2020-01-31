#include <EEPROM.h>
#include <Adafruit_SleepyDog.h>
#include <MFRC522.h>
#include <SPI.h>
#include <string.h>
#include "sportiduino.h"

static MFRC522 mfrc522;
static MFRC522::MIFARE_Key key;
static MFRC522::Uid lastCardUid;
static uint8_t rfidRstPin = 0;
static CardType cardType = CardType::UNKNOWN;

// data buffer size should be greater 18 bytes
bool mifareCardPageRead(uint8_t pageAdr, byte *data, byte *size);
// data buffer size should be greater 16 bytes
bool mifareCardPageWrite(uint8_t pageAdr, byte *data, byte size);
// data buffer size should be greater 4 bytes
bool ntagCardPageRead(uint8_t pageAdr, byte *data, byte *size);
// data buffer size should be greater 4 bytes
bool ntagCardPageWrite(uint8_t pageAdr, byte *data, byte size);

void majEepromWrite(uint16_t adr, uint8_t val) {
    for(uint16_t i = 0; i < 3; i++) {
        EEPROM.write(adr + i, val);
    }
}

uint8_t majEepromRead(uint16_t adr) {
    uint8_t val1 = EEPROM.read(adr);
    uint8_t val2 = EEPROM.read(adr + 1);
    uint8_t val3 = EEPROM.read(adr + 2);
    
    if(val1 == val2 || val1 == val3) {
        return val1;
    } else if(val2 == val3) {
        return val2;
    }
    
    //BEEP_EEPROM_ERROR;
    return 0;
}

void beep_w(const uint8_t ledPin, const uint8_t buzPin, uint16_t freq, uint16_t ms, uint8_t n) {
    for(uint8_t i = 0; i < n; i++) {
        Watchdog.reset();
        
        digitalWrite(ledPin, HIGH);
        if(freq > 0) {
            tone(buzPin, freq, ms);
        } else {
            digitalWrite(buzPin, HIGH);
        }
        
        delay(ms);
        Watchdog.reset();
        
        digitalWrite(ledPin, LOW);
        digitalWrite(buzPin, LOW);
        
        if(i < n - 1) {
            delay(ms);
            Watchdog.reset();
        }
    }
}

void rfidBegin(uint8_t ssPin, uint8_t rstPin, uint8_t antennaGain) {
    cardType = CardType::UNKNOWN;
    rfidRstPin = rstPin;
    antennaGain = constrain(antennaGain, MIN_ANTENNA_GAIN, MAX_ANTENNA_GAIN);
    
    memset(&key, 0xFF, sizeof(key));
    
    SPI.begin();
    mfrc522.PCD_Init(ssPin, rstPin);
    mfrc522.PCD_AntennaOff();
    mfrc522.PCD_SetAntennaGain(antennaGain<<4);
    mfrc522.PCD_AntennaOn();
    
    delay(5);
    
    if(!mfrc522.PICC_IsNewCardPresent()) {
        memset(&lastCardUid, 0, sizeof(lastCardUid));
        return;
    }
    
    if(!mfrc522.PICC_ReadCardSerial()) {
        memset(&lastCardUid, 0, sizeof(lastCardUid));
        return;
    }
    
    auto piccType = MFRC522::PICC_GetType(mfrc522.uid.sak);
    switch(piccType) {
        case MFRC522::PICC_TYPE_MIFARE_MINI:
            cardType = CardType::MIFARE_MINI;
            return;
        case MFRC522::PICC_TYPE_MIFARE_1K:
            cardType = CardType::MIFARE_1K;
            return;
        case MFRC522::PICC_TYPE_MIFARE_4K:
            cardType = CardType::MIFARE_4K;
            return;
        case MFRC522::PICC_TYPE_MIFARE_UL: {
            byte pageData[18];
            byte dataSize = sizeof(pageData);
            if(ntagCardPageRead(3, pageData, &dataSize)) {
                switch(pageData[2]) {
                    case 0x12:
                        cardType = CardType::NTAG213;
                        return;
                    case 0x3e:
                        cardType = CardType::NTAG215;
                        return;
                    case 0x6d:
                        cardType = CardType::NTAG216;
                        return;
                }
            }
        }
    }
}

void rfidEnd() {
    if(rfidIsCardDetected()) {
        memcpy(&lastCardUid, &mfrc522.uid, sizeof(lastCardUid));
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    SPI.end();
    digitalWrite(rfidRstPin, LOW);
}

bool rfidIsCardDetected() {
    if(cardType != CardType::UNKNOWN) {
        return true;
    }
    return false;
}

bool rfidIsNewCardDetected() {
    if(rfidIsCardDetected()) {
        for(uint8_t i = 0; i < mfrc522.uid.size; i++) {
            if(lastCardUid.uidByte[i] != mfrc522.uid.uidByte[i]) {
                return true;
            }
        }
    }
    
    return false;
}

bool mifareCardPageRead(uint8_t pageAdr, byte *data, byte *size) {
    // data size should be at least 18 bytes!
    
    if(pageAdr < 3 || *size < 18) {
        return false;
    }
    
    byte blockAddr = pageAdr-3 + ((pageAdr-3)/3);
    byte trailerBlock = blockAddr + (3-blockAddr%4);

    auto status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }
    
    status = mfrc522.MIFARE_Read(blockAddr, data, size);
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }
 
    return true; 
}

bool mifareCardPageWrite(uint8_t pageAdr, byte *data, byte size) {
    // data size should be 16 bytes!
    
    if(pageAdr < 3 || size < 16) {
        return false;
    }
    
    byte blockAddr = pageAdr-3 + ((pageAdr-3)/3);
    byte trailerBlock = blockAddr + (3-blockAddr%4);

    auto status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }

    status = mfrc522.MIFARE_Write(blockAddr, data, size);
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }

    return true;
}

bool ntagCardPageRead(uint8_t pageAdr, byte *data, byte *size) {
    if(*size < 18) {
        return false;
    }

    auto status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(pageAdr, data, size);
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }

    return true;
}

bool ntagCardPageWrite(uint8_t pageAdr, byte *data, byte size) {
    if(pageAdr < 2 || size < 4) {
        return false;
    }

    auto status = (MFRC522::StatusCode)mfrc522.MIFARE_Ultralight_Write(pageAdr, data, size);
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }
    
    return true;
}

uint8_t rfidGetCardMaxPage() {
    switch(cardType) {
        case CardType::MIFARE_MINI:
            return 17;
        case CardType::MIFARE_1K:
            return 50;
        case CardType::MIFARE_4K:
            return 98;
        case CardType::MIFARE_UL:
            return 39;
        case CardType::NTAG216:
            return 225;
        case CardType::NTAG215:
            return 129;
        case CardType::NTAG213:
            return 39;
    }
    
    return 0;
}

CardType rfidGetCardType() {
    return cardType;
}

bool rfidCardPageRead(uint8_t pageAdr, byte *data) {
    uint8_t maxPage = rfidGetCardMaxPage();

    if(pageAdr > maxPage) {
        return false;
    }

    bool result = false;
    byte pageData[18];
    byte dataSize = sizeof(pageData);
    switch(cardType) {
        case CardType::MIFARE_MINI:
        case CardType::MIFARE_1K:
        case CardType::MIFARE_4K:
            result = mifareCardPageRead(pageAdr, pageData, &dataSize);
            break;
        case CardType::MIFARE_UL:
        case CardType::NTAG213:
        case CardType::NTAG215:
        case CardType::NTAG216:
        default:
            result = ntagCardPageRead(pageAdr, pageData, &dataSize);
            break;
    }
    
    if(result) {
        memcpy(data, pageData, 4);
    }
        
    return result;
}

bool rfidCardPageWrite(uint8_t pageAdr, byte *data) {
    uint8_t maxPage = rfidGetCardMaxPage();

    if(pageAdr > maxPage) {
        return false;
    }

    byte pageData[16];
    memset(pageData, 0, sizeof(pageData));
    memcpy(pageData, data, 4);

    switch(cardType) {
        case CardType::MIFARE_MINI:
        case CardType::MIFARE_1K:
        case CardType::MIFARE_4K:
            return mifareCardPageWrite(pageAdr, pageData, sizeof(pageData));
        case CardType::MIFARE_UL:
        case CardType::NTAG213:
        case CardType::NTAG215:
        case CardType::NTAG216:
        default:
            return ntagCardPageWrite(pageAdr, pageData, sizeof(pageData));
    }
}

bool uint32ToByteArray(uint32_t value, uint8_t *byteArray) {
    for(uint8_t i = 0; i < 4; ++i) {
        byteArray[3 - i] = value & 0xff;
        value >>= 8;
    }
    return true;
}

uint32_t byteArrayToUint32(uint8_t *byteArray) {
    uint32_t value = 0;
    for(uint8_t i = 0; i < 4; ++i) {
        value |= byteArray[i];
        value <<= 8;
    }

    return value;
}

