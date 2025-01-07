#include <Arduino.h>
#include <SPI.h>
#include "debug.h"
#include "rfid.h"

void Rfid::init(uint8_t ssPin, uint8_t rstPin, uint8_t newAntennaGain) {
    rfidSsPin = ssPin;
    rfidRstPin = rstPin;
    antennaGain = constrain(newAntennaGain, MIN_ANTENNA_GAIN, MAX_ANTENNA_GAIN);
    memset(&key, 0xFF, sizeof(key)); // Default MIFARE key
}

void Rfid::setAntennaGain(uint8_t newAntennaGain) {
    antennaGain = constrain(newAntennaGain, MIN_ANTENNA_GAIN, MAX_ANTENNA_GAIN);
}

void Rfid::setPassword(uint8_t* password) {
    if(!password) {
        return;
    }
    // Use password[3] as key for MIFARE or Ntag authentication
    key = {
        password[0], password[1], password[2],
        password[0], password[1], password[2]
    };
}

void Rfid::begin(uint8_t newAntennaGain) {
    if(newAntennaGain) {
        antennaGain = newAntennaGain;
    }
    cardType = CardType::UNKNOWN;
    authenticated = false;

    SPI.begin();
    mfrc522.PCD_Init(rfidSsPin, rfidRstPin);
    mfrc522.PCD_AntennaOff();
    mfrc522.PCD_SetAntennaGain(antennaGain<<4);
    mfrc522.PCD_AntennaOn();

    delay(5);

    if(!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
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
            if(ntagCard4PagesRead(3, pageData, &dataSize)) {
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
        DEBUG_PRINTLN("Same UID");
    }
    
    return false;
}

MFRC522::MIFARE_Key defaultMifareKey = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool Rfid::mifareCardPageRead(uint8_t pageAdr, byte *data, byte *size) {
    // data size should be at least 18 bytes!
    
    if(pageAdr < 3 || *size < 18) {
        return false;
    }
    
    byte blockAddr = pageAdr-3 + ((pageAdr-3)/3);
    byte trailerBlock = blockAddr + (3-blockAddr%4);

    auto status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &defaultMifareKey, &(mfrc522.uid));
    
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

    auto status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &defaultMifareKey, &(mfrc522.uid));
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }

    status = mfrc522.MIFARE_Write(blockAddr, data, size);
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }

    return true;
}

bool Rfid::ntagSetPassword(uint8_t *password, uint8_t *pack, bool readAndWrite, uint8_t negAuthAttemptsLim, uint8_t startPage) {
    if(negAuthAttemptsLim > 7) {
        negAuthAttemptsLim = 7;
    }

    uint8_t maxPage = getCardMaxPage();
    if(!ntagCardPageWrite(maxPage + PAGE_PWD_OFFSET, password, 4)) {
        return false;
    }

    uint8_t packPageData[4] = {pack[0], pack[1], 0, 0};
    if(!ntagCardPageWrite(maxPage + PAGE_PACK_OFFSET, packPageData, 4)) {
        return false;
    }

    //uint8_t pageData[18];
    //uint8_t dataSize = sizeof(pageData);
    //if(!ntagCard4PagesRead(maxPage + PAGE_CFG1_OFFSET, pageData, &dataSize)) {
    //    return false;
    //}
    uint8_t accessByteData = readAndWrite ? 0x80 : 0x00;
    accessByteData |= negAuthAttemptsLim & 0x07;
    uint8_t cfg1PageData[4] = {accessByteData, 0, 0, 0};
    if(!ntagCardPageWrite(maxPage + PAGE_CFG1_OFFSET, cfg1PageData, 4)) {
        return false;
    }

    // Set start page to enable the password verification at the end of the procedure
    uint8_t cfg0PageData[4] = {0, 0, 0, startPage};
    if(!ntagCardPageWrite(maxPage + PAGE_CFG0_OFFSET, cfg0PageData, 4)) {
        return false;
    }

    return true;
}

bool Rfid::ntagDisableAuthentication() {
    DEBUG_PRINTLN("ntagDisableAuthentication");
    uint8_t maxPage = getCardMaxPage();
    uint8_t cfg0PageData[4] = {0, 0, 0, 0xff};
    if(!ntagCardPageWrite(maxPage + PAGE_CFG0_OFFSET, cfg0PageData, 4)) {
        return false;
    }
    return true;
}

bool Rfid::ntagAuth(uint8_t *password, uint8_t *pack) {
    if(!password || !pack) {
        DEBUG_PRINTLN(F("ntagAuth password or pack is null"));
        return false;
    }
    if (!isCardDetected()) {
        DEBUG_PRINTLN(F("ntagAuth card is not detected"));
        return false;
    }
#ifdef DEBUG
    DEBUG_PRINTLN(F("Authenticating..."));
    DEBUG_PRINT(F("Password: "));
    for(uint8_t i = 0; i < 4; i++) {
        DEBUG_PRINT_FORMAT(password[i], HEX);
        DEBUG_PRINT(F(" "));
    }
    DEBUG_PRINTLN("");
    DEBUG_PRINT(F("Pack: "));
    for(uint8_t i = 0; i < 2; i++) {
        DEBUG_PRINT_FORMAT(pack[i], HEX);
        DEBUG_PRINT(F(" "));
    }
    DEBUG_PRINTLN("");
#endif
    uint8_t packReturn[2] = {0, 0};
    auto status = (MFRC522::StatusCode)mfrc522.PCD_NTAG21x_Auth(password, packReturn);
    DEBUG_PRINT(F("Pack from card: 0x"));
    DEBUG_PRINTLN_FORMAT(packReturn[0], HEX);
    DEBUG_PRINT(F(" 0x"));
    DEBUG_PRINTLN_FORMAT(packReturn[1], HEX);

    if(status != MFRC522::STATUS_OK) {
        DEBUG_PRINT(F("Auth failed, status: 0x"));
        DEBUG_PRINTLN_FORMAT(status, HEX);
        if (status == MFRC522::STATUS_TIMEOUT) {
            byte atqa_answer[2];
            byte atqa_size = 2;
            mfrc522.PICC_WakeupA(atqa_answer, &atqa_size);

            if (!mfrc522.PICC_ReadCardSerial()) {
                DEBUG_PRINTLN(F("ReadCardSerial failed"));
            }
        }
        return false;
    }

    return true;
}

bool Rfid::ntagAuthWithMifareKey(MFRC522::MIFARE_Key *key) {
    if(!key) {
        DEBUG_PRINTLN(F("ntagAuthWithMifareKey key is null"));
        return false;
    }
    if(key->keyByte[0] != 0xFF || key->keyByte[1] != 0xFF || key->keyByte[2] != 0xFF || key->keyByte[3] != 0xFF) {
        // Using first 4 bytes of MIFARE key as NTAG password and 2 last bytes as pack
        if(!ntagAuth(&key->keyByte[0], &key->keyByte[4])) {
            DEBUG_PRINTLN(F("ntagAuthWithMifareKey failed, ignoring"));
        }
    }
    authenticated = true;
    return true;
}

bool Rfid::ntagCard4PagesRead(uint8_t pageAdr, byte *data, byte *size) {
    if(*size < 18) {
        return false;
    }

    if(pageAdr >= CARD_PAGE_INIT && !authenticated && !ntagAuthWithMifareKey(&key)) {
        return false;
    }

    auto status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(pageAdr, data, size);
    
    if(status != MFRC522::STATUS_OK) {
        return false;
    }

    return true;
}

bool Rfid::ntagCardPageWrite(uint8_t pageAdr, byte *data, byte size) {
    DEBUG_PRINT(F("ntagCardPageWrite pageAdr: "));
    DEBUG_PRINTLN(pageAdr);
    DEBUG_PRINT(F(" size: "));
    DEBUG_PRINTLN(size);
    if(pageAdr < 2 || size < 4) {
        return false;
    }

    if(!authenticated && !ntagAuthWithMifareKey(&key)) {
        return false;
    }

    auto status = (MFRC522::StatusCode)mfrc522.MIFARE_Ultralight_Write(pageAdr, data, size);
    
    if(status != MFRC522::STATUS_OK) {
        DEBUG_PRINT(F("ntagCardPageWrite failed, status: 0x"));
        DEBUG_PRINTLN_FORMAT(status, HEX);
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
            result = ntagCard4PagesRead(pageAdr, pageData, &dataSize);
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
    DEBUG_PRINT(F("Erasing page "));
    DEBUG_PRINTLN(pageAddr);
    byte pageData[4];
    if(!cardPageRead(pageAddr, pageData)) {
        return false;
    }
    for(uint8_t i = 0; i < 4; ++i) {
        if(pageData[i] != 0) {
            const byte emptyBlock[] = {0,0,0,0};
            return cardPageWrite(pageAddr, emptyBlock);
        }
    }
    return true;
}

bool Rfid::cardErase4Pages(uint8_t pageAddr) {
    if (!isCardDetected()) {
        return false;
    }
    switch(cardType) {
        case CardType::MIFARE_MINI:
        case CardType::MIFARE_1K:
        case CardType::MIFARE_4K: {
            for(uint8_t i = 0; i < 4; ++i) {
                if(!cardPageErase(pageAddr + 3 - i)) {
                    return false;
                }
            }
            return true;
        }
        case CardType::NTAG213:
        case CardType::NTAG215:
        case CardType::NTAG216: {
            DEBUG_PRINT(F("Checking pages at "));
            DEBUG_PRINTLN(pageAddr);
            byte pageData[18];
            byte dataSize = sizeof(pageData);
            if(!ntagCard4PagesRead(pageAddr, pageData, &dataSize)) {
                return false;
            }
            for(uint8_t i = 0; i < 4; ++i) {
                for(uint8_t j = 0; j < 4; ++j) {
                    uint8_t offset = (3 - i);
                    if(pageData[offset*4 + j] != 0) {
                        byte emptyBlock[] = {0,0,0,0};
                        if(!ntagCardPageWrite(pageAddr + offset, emptyBlock, sizeof(emptyBlock))) {
                            return false;
                        }
                        break;
                    }
                }
            }
            return true;
        }
        default:
            return false;
    }
}

bool Rfid::cardEnableDisableAuthentication(bool writeProtection, bool readProtection) {
    if (!isCardDetected()) {
        return false;
    }
    switch(cardType) {
        case CardType::MIFARE_MINI:
        case CardType::MIFARE_1K:
        case CardType::MIFARE_4K: {
            // TODO: not implemented yet 
            return true;
        }
        case CardType::NTAG213:
        case CardType::NTAG215:
        case CardType::NTAG216: {
            if(!writeProtection) {
                return ntagDisableAuthentication();
            }
            // Enable authentication from page 4 and with unlimited negative password verification attempts
            return ntagSetPassword(&key.keyByte[0], &key.keyByte[4], readProtection, 0, CARD_PAGE_INIT);
        }
        default:
            return true;
    }
}

