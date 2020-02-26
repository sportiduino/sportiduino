#include <sportiduino.h>

#define HW_VERS           1
#define FW_MAJOR_VERS     7
#define FW_MINOR_VERS     99

//-----------------------------------------------------------
// HARDWARE

#define BUZZ_PIN        3
#define LED_PIN         4
#define RC522_RST_PIN   9
#define RC522_SS_PIN    10

#define BUZZER_FREQUENCY 0 // 0 for buzzer with generator

//-----------------------------------------------------------
// CONST

#define SERIAL_START_BYTE       0xFE

enum Error {
    ERROR_SERIAL          = 0x01,
    ERROR_CARD_WRITE      = 0x02,
    ERROR_CARD_READ       = 0x03,
    ERROR_EEPROM_READ     = 0x04,
    ERROR_CARD_NOT_FOUND  = 0x05,
    ERROR_UNKNOWN_CMD     = 0x06,
    ERROR_BAD_DATASIZE    = 0x07,
    ERROR_BAD_SETTINGS    = 0x08
};

enum Resp {
    RESP_FUNC_LOG         = 0x61,
    RESP_FUNC_MARKS       = 0x63,
    RESP_FUNC_RAW_DATA    = 0x65,
    RESP_FUNC_VERSION     = 0x66,
    RESP_FUNC_MODE        = 0x69,
    RESP_FUNC_CARD_TYPE   = 0x70,
    RESP_FUNC_ERROR       = 0x78,
    RESP_FUNC_OK          = 0x79
};

#define EEPROM_ANTENNA_GAIN_ADDR 0x3EE

//-----------------------------------------------------------
// FUNCTIONS

inline void beep(uint16_t ms, uint8_t n) { beep_w(LED_PIN, BUZZ_PIN, BUZZER_FREQUENCY, ms, n); }
inline void beepTimeCardOk() { beep(500, 3); delay(500); beep(1000, 1); }
inline void beepError() { beep(100, 3); }
inline void beepOk() { beep(500, 1); }

// Declatarions for building by Arduino-Makefile
void signalError(uint8_t error);
void handleCmd(uint8_t cmdCode, uint8_t *data, uint8_t dataSize);
void setPwd(uint8_t newPwd[]);
uint8_t getPwd(uint8_t i);

//-----------------------------------------------------------
// VARIABLES
static Rfid rfid;
static SerialProtocol serialProto;
static uint8_t antennaGain = DEFAULT_ANTENNA_GAIN;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZ_PIN, OUTPUT);
    pinMode(RC522_RST_PIN, OUTPUT);
    pinMode(RC522_SS_PIN, OUTPUT);
    
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZ_PIN, LOW);
    digitalWrite(RC522_RST_PIN, LOW);

    antennaGain = majEepromRead(EEPROM_ANTENNA_GAIN_ADDR);
    if(antennaGain > MAX_ANTENNA_GAIN || antennaGain < MIN_ANTENNA_GAIN) {
        antennaGain = DEFAULT_ANTENNA_GAIN;
    }

    rfid.init(RC522_SS_PIN, RC522_RST_PIN, antennaGain);
    serialProto.init(SERIAL_START_BYTE);
}

void loop() { 
    bool error = false;
    uint8_t cmdCode = 0;
    uint8_t dataSize = 0;

    uint8_t *data = serialProto.read(&error, &cmdCode, &dataSize);
    if(error) {
        signalError(ERROR_SERIAL);
        return;
    }
    if(data) {
        handleCmd(cmdCode, data, dataSize);
    }
}


void signalError(uint8_t error) { 
    serialProto.start(RESP_FUNC_ERROR);
    serialProto.add(error);
    serialProto.add((uint8_t)rfid.getCardType());
    serialProto.send();

    beepError();
}

void signalOK(bool beepOK = true) {
    serialProto.start(RESP_FUNC_OK);
    serialProto.add((uint8_t)rfid.getCardType());
    serialProto.send();

    if(beepOK) {
        beepOk();
    }
}

uint8_t writeMasterCard(uint8_t masterCode, byte *data = NULL, uint16_t size = 0) {
    if(!rfid.isCardDetected()) {
        return ERROR_CARD_NOT_FOUND;
    }

    byte head[] = {
        0, masterCode, 255, FW_MAJOR_VERS,
        getPwd(0), getPwd(1), getPwd(2), 0
    };

    if(!rfid.cardWrite(CARD_PAGE_INIT, head, sizeof(head))) {
        return ERROR_CARD_WRITE;
    }

    if(data && size > 0) {
        if(!rfid.cardWrite(CARD_PAGE_INIT + 2, data, size)) {
            return ERROR_CARD_WRITE;
        }
    }

    return 0;
}

void funcWriteMasterTime(uint8_t *serialData, uint8_t dataSize) {
    byte data[] = {
        serialData[1], serialData[0], serialData[2], 0,  // month, year, day, 0
        serialData[3], serialData[4], serialData[5], 0   // hour, minute, second, 0
    };

    uint8_t error = writeMasterCard(MASTER_CARD_SET_TIME, data, sizeof(data));

    if(error) {
        signalError(error);
    } else {
        signalOK(false);
        beepTimeCardOk();
    }
}

void funcWriteMasterNum(uint8_t *serialData, uint8_t dataSize) {
    byte data[] = {serialData[0], 0, 0, 0};     // station num
    
    uint8_t error = writeMasterCard(MASTER_CARD_SET_NUMBER, data, sizeof(data));

    if(error) {
        signalError(error);
    } else {
        signalOK();
    }
}

void funcWriteMasterConfig(uint8_t *serialData, uint8_t dataSize) {
    if(dataSize != 6) {
        signalError(ERROR_BAD_DATASIZE);
        return;
    }

    uint8_t error = writeMasterCard(MASTER_CARD_CONFIG, serialData, dataSize);

    if(error) {
        signalError(error);
    } else {
        signalOK();
    }
}

void funcApplyPassword(uint8_t *serialData, uint8_t dataSize) {
    setPwd(serialData);
    signalOK();
}

void funcWriteSettings(uint8_t *serialData, uint8_t dataSize) {
    if(dataSize != 1) {
        signalError(ERROR_BAD_DATASIZE);
        return;
    }
    uint8_t newAntennaGain = serialData[0];
    if(newAntennaGain < MIN_ANTENNA_GAIN || newAntennaGain > MAX_ANTENNA_GAIN) {
        signalError(ERROR_BAD_SETTINGS);
        return;
    }
    antennaGain = newAntennaGain;
    majEepromWrite(EEPROM_ANTENNA_GAIN_ADDR, antennaGain);
    signalOK();
}

void funcInitPaticipantCard(uint8_t *serialData, uint8_t dataSize) {
    if(!rfid.isCardDetected()) {
        signalError(ERROR_CARD_NOT_FOUND);
        return;
    }

    CardType cardType = rfid.getCardType();

    uint8_t ntagType; // for old BS firmware?
    switch(cardType) {
        case CardType::NTAG213:
            ntagType = 3;
            break;
        case CardType::NTAG215:
            ntagType = 5;
            break;
        case CardType::NTAG216:
            ntagType = 6;
            break;
        default:
            ntagType = 0;
            break;
    }

    byte data[] = {
        serialData[0], serialData[1], ntagType, FW_MAJOR_VERS,          // card num, card type, version
        serialData[2], serialData[3], serialData[4], serialData[5],     // unixtime
        serialData[6], serialData[7], serialData[8], serialData[9],     // page6
        serialData[10], serialData[11], serialData[12], serialData[13]  // page7
    };

    if(!rfid.cardWrite(CARD_PAGE_INIT, data, sizeof(data))) {
        signalError(ERROR_CARD_WRITE);
        return;
    }

    uint8_t maxPage = rfid.getCardMaxPage();
    if(!rfid.cardErase(CARD_PAGE_START, maxPage)) {
        signalError(ERROR_CARD_WRITE);
        return;
    }


    signalOK();
}

void funcWriteInfo(uint8_t *serialData, uint8_t dataSize) {
    if(dataSize != 8) {
        signalError(ERROR_BAD_DATASIZE);
        return;
    }

    if(!rfid.isCardDetected()) {
        signalError(ERROR_CARD_NOT_FOUND);
        return;
    }

    if(!rfid.cardWrite(CARD_PAGE_INFO1, serialData, 8)) {
        signalError(ERROR_CARD_WRITE);
        return;
    }

    signalOK();
}

void funcWriteMasterLog(uint8_t*, uint8_t) {
    uint8_t error = writeMasterCard(MASTER_CARD_READ_DUMP);

    if(error) {
        signalError(error);
    } else {
        signalOK();
    }
}

void funcWriteGetInfoCard(uint8_t*, uint8_t) {
    uint8_t error = writeMasterCard(MASTER_CARD_GET_INFO);

    if(error) {
        signalError(error);
    } else {
        signalOK();
    }
}

void funcWriteMasterSleep(uint8_t *serialData, uint8_t dataSize) {
    // wakeup time
    byte data[] = {
        serialData[1], serialData[0], serialData[2], 0,
        serialData[3], serialData[4], serialData[5], 0
    };

    uint8_t error = writeMasterCard(MASTER_CARD_SLEEP, data, sizeof(data));

    if(error) {
        signalError(error);
    } else {
        signalOK();
    }
}

void funcReadLog(uint8_t*, uint8_t) {
    if(!rfid.isCardDetected()) {
        signalError(ERROR_CARD_NOT_FOUND);
        return;
    }

    byte pageData[] = {0,0,0,0};
    if(!rfid.cardPageRead(CARD_PAGE_INIT, pageData)) {
        signalError(ERROR_CARD_READ);
        return;
    }

    serialProto.start(RESP_FUNC_LOG);
    serialProto.add(pageData[0]);   // add station number

    uint8_t maxPage = rfid.getCardMaxPage();
    if(pageData[3] > 0) { // have timestamps
        serialProto.add(0); // flag: have timestamps
        uint16_t timeHigh12bits = 0;
        uint32_t initTime = 0;
        for(uint8_t page = CARD_PAGE_INFO1; page <= maxPage; ++page) {
            if(!rfid.cardPageRead(page, pageData)) {
                signalError(ERROR_CARD_READ);
                return;
            }

            if(timeHigh12bits == 0) {
                timeHigh12bits = pageData[0] << 8;
                timeHigh12bits |= pageData[1] & 0xf0;
                initTime = ((uint32_t)pageData[1] & 0x0f) << 16;
                initTime |= (uint32_t)pageData[2] << 8;
                initTime |= pageData[3];
                continue;
            }

            uint16_t cardNum = pageData[0] << 8;
            cardNum |= pageData[1] & 0xff;
            cardNum >>= 4;

            if(cardNum == 0) {
                continue;
            }
            serialProto.add(pageData[0] >> 4); // card number first byte
            serialProto.add(pageData[0] << 4 | pageData[1] >> 4); // card number second byte
            uint32_t punchTime = ((uint32_t)pageData[1] & 0x0f) << 16;
            punchTime |= (uint32_t)pageData[2] << 8;
            punchTime |= pageData[3];
            uint16_t currentTimeHigh12bits = timeHigh12bits;
            if(punchTime < initTime) {
                currentTimeHigh12bits += 0x10;
            }
            serialProto.add(currentTimeHigh12bits >> 8);
            serialProto.add((currentTimeHigh12bits&0xf0) | (pageData[1]&0x0f));
            serialProto.add(pageData[2]);
            serialProto.add(pageData[3]);
        }
    } else {
        for(uint8_t page = CARD_PAGE_DUMP_START; page <= maxPage; ++page) {
            if(!rfid.cardPageRead(page, pageData)) {
                signalError(ERROR_CARD_READ);
                return;
            }
                
            for(uint8_t i = 0; i < 4; i++) {
                for(uint8_t y = 0; y < 8; y++) {
                    if(pageData[i] & (1 << y)) {
                        uint16_t num = (page - CARD_PAGE_DUMP_START)*32 + i*8 + y;
                        uint8_t first = (num&0xFF00)>>8;
                        uint8_t second = num&0x00FF; 
                        serialProto.add(first);
                        serialProto.add(second);
                    }
                }
            }
        }
    }

    serialProto.send();
    beepOk();
}

void funcReadCard(uint8_t*, uint8_t) {
    // Don't signal error to prevent discontiniuos beep in the poll mode
    if(!rfid.isCardDetected()) {
        return;
    }

    byte pageData[] = {0,0,0,0};
    if(!rfid.cardPageRead(CARD_PAGE_INIT, pageData)) {
        return;
    }
    serialProto.start(RESP_FUNC_MARKS);
    // Output the card number
    serialProto.add(pageData[0]);
    serialProto.add(pageData[1]);

    if(!rfid.cardPageRead(CARD_PAGE_INIT_TIME, pageData)) {
        return;
    }
    uint8_t timeHighByte = pageData[0];
    uint32_t initTime = pageData[1];
    initTime <<= 8;
    initTime |= pageData[2];
    initTime <<= 8;
    initTime |= pageData[3];

    if(!rfid.cardPageRead(CARD_PAGE_INFO1, pageData)) {
        return;
    }
    // Output page 6
    for(uint8_t i = 0; i < 4; i++) {
        serialProto.add(pageData[i]);
    }

    if(!rfid.cardPageRead(CARD_PAGE_INFO2, pageData)) {
        return;
    }
    // Output page 7
    for(uint8_t i = 0; i < 4; i++) {
        serialProto.add(pageData[i]);
    }

    uint8_t maxPage = rfid.getCardMaxPage();

    for(uint8_t page = CARD_PAGE_START; page <= maxPage; ++page) {
        if(!rfid.cardPageRead(page, pageData)) {
            return;
        }

        if(pageData[0] == 0) { // no new punches
            return;
        }
        // Output station number
        serialProto.add(pageData[0]);

        uint32_t markTime = pageData[1];
        markTime <<= 8;
        markTime |= pageData[2];
        markTime <<= 8;
        markTime |= pageData[3];

        // for example, we have init time 0x00FFFFFF
        // all mark time will be 0x01xxxxxx
        // in this case we have to add 1 to timeHighByte
        if(markTime < initTime) {
            serialProto.add(timeHighByte + 1);
        } else {
            serialProto.add(timeHighByte);
        }
        // Output time
        serialProto.add(pageData[1]);
        serialProto.add(pageData[2]);
        serialProto.add(pageData[3]);
    }

    serialProto.send();
}

void funcReadRawCard(uint8_t*, uint8_t) {
    uint8_t error = ERROR_CARD_NOT_FOUND;
    byte pageData[] = {0,0,0,0};

    serialProto.start(RESP_FUNC_RAW_DATA);

    if(rfid.isCardDetected()) {
        error = ERROR_CARD_READ;
        uint8_t maxPage = rfid.getCardMaxPage();

        for(uint8_t page = CARD_PAGE_INIT; page <= maxPage; page++) {
            if(!rfid.cardPageRead(page, pageData)) {
                error = ERROR_CARD_READ;
                break;
            }
            error = 0;
            serialProto.add(page);
            for(uint8_t i = 0; i < 4; i++) {
                serialProto.add(pageData[i]);
            }
        }   
    }

    if(error) {
        signalError(error);
    } else {
        serialProto.send();
        beepOk();
    }
}

void funcReadCardType(uint8_t*, uint8_t) {
    serialProto.start(RESP_FUNC_CARD_TYPE);
    serialProto.add((uint8_t)rfid.getCardType());
    serialProto.send();
}

void funcGetVersion(uint8_t*, uint8_t) {
    serialProto.start(RESP_FUNC_VERSION);

    serialProto.add(HW_VERS);
    serialProto.add(FW_MAJOR_VERS);
    serialProto.add(FW_MINOR_VERS);

    serialProto.send();
}

void callRfidFunction(void (*func)(uint8_t*, uint8_t), uint8_t *data, uint8_t dataSize) {
    rfid.begin(antennaGain);
    func(data, dataSize);
    rfid.end();
}

void handleCmd(uint8_t cmdCode, uint8_t *data, uint8_t dataSize) {
    switch(cmdCode) {
        case 0x41:
            callRfidFunction(funcWriteMasterTime, data, dataSize);
            break;
        case 0x42:
            callRfidFunction(funcWriteMasterNum, data, dataSize);
            break;
        case 0x5A:
            callRfidFunction(funcWriteMasterConfig, data, dataSize);
            break;
        case 0x44:
            callRfidFunction(funcInitPaticipantCard, data, dataSize);
            break;
        case 0x45:
            callRfidFunction(funcWriteInfo, data, dataSize);
            break;
        case 0x46:
            funcGetVersion(data, dataSize);
            break;
        case 0x47:
            callRfidFunction(funcWriteMasterLog, data, dataSize);
            break;
        case 0x48:
            callRfidFunction(funcReadLog, data, dataSize);
            break;
        case 0x4A:
            funcWriteSettings(data, dataSize);
            break;
        case 0x4B:
            callRfidFunction(funcReadCard, data, dataSize);
            break;
        case 0x4C:
            callRfidFunction(funcReadRawCard, data, dataSize);
            break;
        case 0x4E:
            callRfidFunction(funcWriteMasterSleep, data, dataSize);
            break;
        case 0x4F:
            funcApplyPassword(data, dataSize);
            break;
        case 0x50:
            callRfidFunction(funcWriteGetInfoCard, data, dataSize);
            break;
        case 0x51:
            callRfidFunction(funcReadCardType, data, dataSize);
            break;
        case 0x58:
            beepError();
            break;
        case 0x59:
            beepOk();
            break;
        default:
            signalError(ERROR_UNKNOWN_CMD);
            break;
    }
}

uint8_t pwd[] = {0, 0, 0};

void setPwd(uint8_t newPwd[]) {
    memcpy(pwd, newPwd, 3);
}

uint8_t getPwd(uint8_t i) {
    return pwd[i];
}

