#include <EEPROM.h>
#include <Adafruit_SleepyDog.h>
#include <MFRC522.h>
#include <SPI.h>
#include <string.h>
#include "sportiduino.h"

MFRC522 mfrc522;
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
MFRC522::Uid lastCardUid;
uint8_t rfidRstPin = 0;
uint8_t pass[] = {0,0,0};
uint8_t settings = DEFAULT_SETTINGS;
uint8_t antennaGain = DEFAULT_ANTENNA_GAIN;
uint8_t cardType = 0xFF;

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

bool readPwdSettings() {
    bool result = true;
    
    settings = majEepromRead(EEPROM_SETTINGS_ADDR);
    
    for(uint8_t i = 0; i < 3; i++) {
        pass[i] = majEepromRead(EEPROM_PASS_ADDR + i*3);
    }
    
    antennaGain = majEepromRead(EEPROM_ANTENNA_GAIN_ADDR);

    if(antennaGain > MAX_ANTENNA_GAIN || antennaGain < MIN_ANTENNA_GAIN) {
        setAntennaGain(DEFAULT_ANTENNA_GAIN);
    }
    
    if(settings & SETTINGS_INVALID) {
        setSettings(DEFAULT_SETTINGS);
        setPwd(0,0,0);
        setAntennaGain(DEFAULT_ANTENNA_GAIN);
        result = false;
    }

    return result;
}

void rfidBegin(uint8_t ssPin, uint8_t rstPin) {
    cardType = 0xFF;
    rfidRstPin = rstPin;
    
    memset(&key, 0xFF, sizeof(key));
    
    SPI.begin();
    mfrc522.PCD_Init(ssPin, rstPin);
    mfrc522.PCD_AntennaOff();
    mfrc522.PCD_SetAntennaGain(antennaGain);
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
    
    cardType = MFRC522::PICC_GetType(mfrc522.uid.sak);
    if(cardType == MFRC522::PICC_TYPE_MIFARE_UL) {
        byte pageData[18];
        byte dataSize = sizeof(pageData);
        if(ntagCardPageRead(3, pageData, &dataSize)) {
            cardType = pageData[2];
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
    digitalWrite(rfidRstPin,LOW);
}

bool rfidIsCardDetected() {
    switch(cardType) {
        case MFRC522::PICC_TYPE_MIFARE_MINI:
        case MFRC522::PICC_TYPE_MIFARE_1K:
        case MFRC522::PICC_TYPE_MIFARE_4K:
        case MFRC522::PICC_TYPE_MIFARE_UL:
        case 0x6D: // NTAG_216
        case 0x3E: // NTAG_215
        case 0x12: // NTAG_213
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

    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    
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

    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    
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

    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(pageAdr, data, size);
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }

    return true;
}

bool ntagCardPageWrite(uint8_t pageAdr, byte *data, byte size) {
    if(pageAdr < 2 || size < 4) {
        return false;
    }

    status = (MFRC522::StatusCode)mfrc522.MIFARE_Ultralight_Write(pageAdr, data, size);
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }
    
    return true;
}

uint8_t rfidGetCardMaxPage() {
    switch(cardType) {
        case MFRC522::PICC_TYPE_MIFARE_MINI:
            return 17;
        case MFRC522::PICC_TYPE_MIFARE_1K:
            return 50;
        case MFRC522::PICC_TYPE_MIFARE_4K:
            return 98;
        case MFRC522::PICC_TYPE_MIFARE_UL:
            return 39;
        case 0x6D: // NTAG_216
            return 225;
        case 0x3E: // NTAG_215
            return 129;
        case 0x12: // NTAG_213
            return 39;
    }
    
    return 0;
}

uint8_t rfidGetCardType() {
    return cardType;
}

bool rfidCardPageRead(uint8_t pageAdr, byte *data) {
    byte pageData[18];
    byte dataSize = sizeof(pageData);
    bool result = false;
    uint8_t maxPage = rfidGetCardMaxPage();
    
    if(pageAdr > maxPage) {
        return false;
    }
    
    switch(cardType) {
        case MFRC522::PICC_TYPE_MIFARE_MINI:
        case MFRC522::PICC_TYPE_MIFARE_1K:
        case MFRC522::PICC_TYPE_MIFARE_4K:
            result = mifareCardPageRead(pageAdr, pageData, &dataSize);
            break;
        case MFRC522::PICC_TYPE_MIFARE_UL:
        case 0x12:
        case 0x3E:
        case 0x6D:
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
    byte pageData[16];
    bool result = false;
    uint8_t maxPage = rfidGetCardMaxPage();
    
    if(pageAdr > maxPage) {
        return false;
    }

    memset(pageData, 0, sizeof(pageData));
    memcpy(pageData, data, 4);
    
    switch(cardType) {
        case MFRC522::PICC_TYPE_MIFARE_MINI:
        case MFRC522::PICC_TYPE_MIFARE_1K:
        case MFRC522::PICC_TYPE_MIFARE_4K:
            result = mifareCardPageWrite(pageAdr, pageData, sizeof(pageData));
            break;
        case MFRC522::PICC_TYPE_MIFARE_UL:
        case 0x12:
        case 0x3E:
        case 0x6D:
        default:
            result = ntagCardPageWrite(pageAdr, pageData, sizeof(pageData));
            break;
    }
        
    return result;
}

void setPwd(uint8_t pwd1, uint8_t pwd2, uint8_t pwd3) {
    if(pass[0] == pwd1 && pass[1] == pwd2 && pass[2] == pwd3) {
        return;
    }
        
    pass[0] = pwd1;
    pass[1] = pwd2;
    pass[2] = pwd3;
    
    for(uint8_t i = 0; i < 3; i++) {
        majEepromWrite(EEPROM_PASS_ADDR + i*3, pass[i]);
    }
}

uint8_t getPwd(uint8_t n) {
    if(n > 2) {
        return 0xFF;
    }
    
    return pass[n];
}

void setSettings(uint8_t value) {
    if(settings == value) {
        return;
    }

    settings = value;
    majEepromWrite(EEPROM_SETTINGS_ADDR, settings);
}

uint8_t getSettings() {
    return settings;
}

void setAntennaGain(uint8_t gain) {
    if(antennaGain == gain || gain > MAX_ANTENNA_GAIN || gain < MIN_ANTENNA_GAIN) {
        return;
    }

    antennaGain = gain;
    majEepromWrite(EEPROM_ANTENNA_GAIN_ADDR, antennaGain);
}

uint8_t getAntennaGain() {
    return antennaGain;
}
