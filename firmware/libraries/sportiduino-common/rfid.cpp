#include <Arduino.h>
#include <SPI.h>
#include "rfid.h"

void Rfid::init(uint8_t ssPin, uint8_t rstPin, uint8_t newAntennaGain) {
    rfidSsPin = ssPin;
    rfidRstPin = rstPin;
    antennaGain = constrain(newAntennaGain, MIN_ANTENNA_GAIN, MAX_ANTENNA_GAIN);
}

void Rfid::clearLastCardUid() {
    memset(&lastCardUid, 0, sizeof(lastCardUid));
}

void Rfid::setAntennaGain(uint8_t newAntennaGain) {
    antennaGain = constrain(newAntennaGain, MIN_ANTENNA_GAIN, MAX_ANTENNA_GAIN);
}

void Rfid::begin(uint8_t newAntennaGain) {
    if(newAntennaGain) {
        antennaGain = newAntennaGain;
    }
    cardType = CardType::UNKNOWN;
    
    memset(&key, 0xFF, sizeof(key));
    
    SPI.begin();
    mfrc522.PCD_Init(rfidSsPin, rfidRstPin);
    mfrc522.PCD_AntennaOff();
    mfrc522.PCD_SetAntennaGain(antennaGain<<4);
    mfrc522.PCD_AntennaOn();
    
    delay(5);
    
    if(!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        clearLastCardUid();
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
        default:
            cardType = CardType::UNKNOWN;
            return;
    }
}

void Rfid::end() {
    if(isCardDetected()) {
        memcpy(&lastCardUid, &mfrc522.uid, sizeof(lastCardUid));
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    SPI.end();
    digitalWrite(rfidRstPin, LOW);
}

bool Rfid::isCardDetected() {
    if(cardType != CardType::UNKNOWN) {
        return true;
    }
    return false;
}

bool Rfid::isNewCardDetected() {
    if(isCardDetected()) {
        for(uint8_t i = 0; i < mfrc522.uid.size; i++) {
            if(lastCardUid.uidByte[i] != mfrc522.uid.uidByte[i]) {
                return true;
            }
        }
    }
    
    return false;
}

bool Rfid::mifareCardPageRead(uint8_t pageAdr, byte *data, byte *size) {
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

bool Rfid::mifareCardPageWrite(uint8_t pageAdr, byte *data, byte size) {
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

bool Rfid::ntagCardPageRead(uint8_t pageAdr, byte *data, byte *size) {
    if(*size < 18) {
        return false;
    }

    auto status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(pageAdr, data, size);
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }

    return true;
}

bool Rfid::ntagCardPageWrite(uint8_t pageAdr, byte *data, byte size) {
    if(pageAdr < 2 || size < 4) {
        return false;
    }

    auto status = (MFRC522::StatusCode)mfrc522.MIFARE_Ultralight_Write(pageAdr, data, size);
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }
    
    return true;
}

uint8_t Rfid::getCardMaxPage() {
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
        default:
            return 0;
    }

    return 0;
}

CardType Rfid::getCardType() {
    return cardType;
}

bool Rfid::cardPageRead(uint8_t pageAdr, byte *data, uint8_t size) {
    uint8_t maxPage = getCardMaxPage();

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
        memcpy(data, pageData, size);
    }
        
    return result;
}

bool Rfid::cardPageWrite(uint8_t pageAdr, const byte *data, uint8_t size) {
    uint8_t maxPage = getCardMaxPage();

    if(pageAdr > maxPage) {
        return false;
    }

    byte pageData[16];
    memset(pageData, 0, sizeof(pageData));
    memcpy(pageData, data, size);

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

bool Rfid::cardWrite(uint8_t startPageAdr, const byte *data, uint16_t size) {
    uint8_t pageAddr = startPageAdr;
    for(uint8_t i = 0; i < size/4; ++i) {
        if(!cardPageWrite(pageAddr++, data + i*4)) {
            return false;
        }
    }

    uint8_t tailSize = size%4;
    if(tailSize > 0) {
        if(!cardPageWrite(pageAddr, data + size - tailSize, tailSize)) {
            return false;
        }
    }
    return true;
}

bool Rfid::cardErase(uint8_t beginPageAddr, uint8_t endPageAddr) {
    for(uint8_t pageAddr = endPageAddr; pageAddr >= beginPageAddr; --pageAddr) {
        if(!cardPageErase(pageAddr)) {
            return false;
        }
    }
    return true;
}

bool Rfid::cardPageErase(uint8_t pageAddr) {
    const byte emptyBlock[] = {0,0,0,0};
    return cardPageWrite(pageAddr, emptyBlock);
}

