#include <Arduino.h>
#include <SPI.h>
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

void Rfid::setPassword(uint8_t* password, bool _ntagWriteProtection, bool _ntagReadProtection) {
    if(!password) {
        return;
    }
    // Use password[3] as key for MIFARE or Ntag authentication
    key = {
        password[0], password[1], password[2],
        password[0], password[1], password[2]
    };
    ntagWriteProtection = _ntagWriteProtection;
    ntagReadProtection = _ntagReadProtection && _ntagWriteProtection;
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
#ifdef DEBUG
        Serial.println("Same UID");
#endif
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
    //if(!ntagCardPageRead(maxPage + PAGE_CFG1_OFFSET, pageData, &dataSize)) {
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
#ifdef DEBUG
    Serial.println("ntagDisableAuthentication");
#endif
    uint8_t maxPage = getCardMaxPage();
    uint8_t cfg0PageData[4] = {0, 0, 0, 0xff};
    if(!ntagCardPageWrite(maxPage + PAGE_CFG0_OFFSET, cfg0PageData, 4)) {
        return false;
    }
    return true;
}

bool Rfid::ntagAuth(uint8_t *password, uint8_t *pack) {
    if(!password || !pack) {
#ifdef DEBUG
        Serial.println(F("ntagAuth password or pack is null"));
#endif
        return false;
    }
    if (!isCardDetected()) {
#ifdef DEBUG
        Serial.println(F("ntagAuth card is not detected"));
#endif
        return false;
    }
#ifdef DEBUG
    Serial.println(F("Authenticating..."));
    Serial.print(F("Password: "));
    for(uint8_t i = 0; i < 4; i++) {
        Serial.print(password[i], HEX);
        Serial.print(F(" "));
    }
    Serial.println();
    Serial.print(F("Pack: "));
    for(uint8_t i = 0; i < 2; i++) {
        Serial.print(pack[i], HEX);
        Serial.print(F(" "));
    }
    Serial.println();
#endif
    uint8_t packReturn[2] = {0, 0};
    auto status = (MFRC522::StatusCode)mfrc522.PCD_NTAG21x_Auth(password, packReturn);
#ifdef DEBUG
    Serial.print(F("Pack from card: 0x"));
    Serial.print(packReturn[0], HEX);
    Serial.print(F(" 0x"));
    Serial.println(packReturn[1], HEX);
#endif

    if(status != MFRC522::STATUS_OK) {
#ifdef DEBUG
        Serial.print(F("Auth failed, status: 0x"));
        Serial.println(status, HEX);
#endif
        if (status == MFRC522::STATUS_TIMEOUT) {
            byte atqa_answer[2];
            byte atqa_size = 2;
            mfrc522.PICC_WakeupA(atqa_answer, &atqa_size);

            if (!mfrc522.PICC_ReadCardSerial()) {
#ifdef DEBUG
                Serial.println(F("ReadCardSerial failed"));
#endif
            }
        }

//#ifdef DEBUG
//        Serial.println(F("Trying default password"));
//#endif
//        uint8_t defaultPassword[4] = {0xFF, 0xFF, 0xFF, 0xFF};
//        uint8_t defaultPack[2] = {0, 0};
//        auto status = (MFRC522::StatusCode)mfrc522.PCD_NTAG21x_Auth(defaultPassword, defaultPack);
//        if(status != MFRC522::STATUS_OK) {
//            status = (MFRC522::StatusCode)mfrc522.PCD_NTAG21x_Auth(password, pack);
//#ifdef DEBUG
//            Serial.print(F("Auth failed, status: 0x"));
//            Serial.println(status, HEX);
//#endif
//            return false;
//        }
        return false;
    }

    return true;
}

bool Rfid::ntagAuthWithMifareKey(MFRC522::MIFARE_Key *key) {
    if(!key) {
#ifdef DEBUG
        Serial.println(F("ntagAuthWithMifareKey key is null"));
#endif
        return false;
    }
    if(key->keyByte[0] != 0xFF || key->keyByte[1] != 0xFF || key->keyByte[2] != 0xFF || key->keyByte[3] != 0xFF) {
        // Using first 4 bytes of MIFARE key as NTAG password and 2 last bytes as pack
        if(!ntagAuth(&key->keyByte[0], &key->keyByte[4])) {
#ifdef DEBUG
            Serial.println(F("ntagAuthWithMifareKey failed, ignoring"));
#endif
        }
    }
    authenticated = true;
    return true;
}

bool Rfid::ntagCardPageRead(uint8_t pageAdr, byte *data, byte *size) {
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
#ifdef DEBUG
    Serial.print(F("ntagCardPageWrite pageAdr: 0x"));
    Serial.print(pageAdr, HEX);
    Serial.print(F(" size: "));
    Serial.println(size);
#endif
    if(pageAdr < 2 || size < 4) {
        return false;
    }

    if(!authenticated && !ntagAuthWithMifareKey(&key)) {
        return false;
    }

    auto status = (MFRC522::StatusCode)mfrc522.MIFARE_Ultralight_Write(pageAdr, data, size);
    
    if(status != MFRC522::STATUS_OK) {
#ifdef DEBUG
        Serial.print(F("ntagCardPageWrite failed, status: 0x"));
        Serial.println(status, HEX);
#endif
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

bool Rfid::cardEnableDisableAuthentication() {
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
            if(!ntagWriteProtection) {
                return ntagDisableAuthentication();
            }
            // Enable authentication from page 4 and with unlimited negative password verification attempts
            return ntagSetPassword(&key.keyByte[0], &key.keyByte[4], ntagReadProtection, 0, CARD_PAGE_INIT);
        }
        default:
            return true;
    }
}

