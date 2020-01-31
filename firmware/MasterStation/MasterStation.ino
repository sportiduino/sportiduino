#include <sportiduino.h>

#define HW_VERS           1
#define FW_MAJOR_VERS     7
#define FW_MINOR_VERS     99

//-----------------------------------------------------------
// HARDWARE

#define BUZ             3
#define LED             4
#define RC522_RST_PIN   9
#define RC522_SS_PIN    10

#define BUZZER_FREQUENCY 0 // 0 for buzzer with generator

//-----------------------------------------------------------
// CONST

#define SERIAL_TIMEOUT          10
#define SERIAL_PACKET_SIZE      32
#define SERIAL_START_BYTE       0xFE
#define SERIAL_DATA_MAX_SIZE    28

#define NO_ERRORS               0x00
#define ERROR_SERIAL            0x01
#define ERROR_CARD_WRITE        0x02
#define ERROR_CARD_READ         0x03
#define ERROR_EEPROM_READ       0x04
#define ERROR_CARD_NOT_FOUND    0x05
#define ERROR_UNKNOWN_CMD       0x06

#define RESP_FUNC_LOG           0x61
#define RESP_FUNC_MARKS         0x63
#define RESP_FUNC_RAW_DATA      0x65
#define RESP_FUNC_VERSION       0x66
#define RESP_FUNC_MODE          0x69
#define RESP_FUNC_CARD_TYPE     0x70
#define RESP_FUNC_ERROR         0x78
#define RESP_FUNC_OK            0x79

//-----------------------------------------------------------
// FUNCTIONS

#define beep(ms, n) { beep_w(LED, BUZ, BUZZER_FREQUENCY, ms, n); }
#define beepTimeCardOk() { beep(500, 3); }
#define beepError() { beep(100, 3); }
#define beepOk() { beep(500, 1); }

//-----------------------------------------------------------
// VARIABLES

uint8_t serialBuffer[SERIAL_PACKET_SIZE];
uint8_t serialDataPos = 3;
uint8_t serialPacketCount = 0;

void setup() {
    pinMode(LED,OUTPUT);
    pinMode(BUZ,OUTPUT);
    pinMode(RC522_RST_PIN,OUTPUT);
    pinMode(RC522_SS_PIN,OUTPUT);
    
    digitalWrite(LED,LOW);
    digitalWrite(BUZ,LOW);
    digitalWrite(RC522_RST_PIN,LOW);
    
    Serial.begin(9600);
    Serial.setTimeout(SERIAL_TIMEOUT);
}

void loop() { 
    if(Serial.available() > 0) {
        Serial.readBytes(serialBuffer, SERIAL_PACKET_SIZE);
        
        uint8_t dataSize = serialBuffer[2];
        
        // if at dataSize position we have packet count
        if(dataSize > SERIAL_DATA_MAX_SIZE) {
            dataSize = SERIAL_DATA_MAX_SIZE;  
        }
        
        if(serialBuffer[0] != SERIAL_START_BYTE || serialBuffer[dataSize + 3] != serialCheckSum(serialBuffer, dataSize)) {
            signalError(ERROR_SERIAL);
            return;
        }
        handleCmd(serialBuffer[1]);
    }
}

void rfidFunction(void (*func)()) {
    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);
    func();
    rfidEnd();
}

void handleCmd(uint8_t cmdCode) {
    switch(cmdCode) {
        case 0x41:
            funcWriteMasterTime();
            break;
        case 0x42:
            funcWriteMasterNum();
            break;
        case 0x43:
            funcWriteMasterSettings();
            break;
        case 0x44:
            funcWriteInit();
            break;
        case 0x45:
            funcWriteInfo();
            break;
        case 0x46:
            funcGetVersion();
            break;
        case 0x47:
            funcWriteMasterLog();
            break;
        case 0x48:
            rfidFunction(funcReadLog);
            break;
        case 0x4B:
            rfidFunction(funcReadCard);
            break;
        case 0x4C:
            funcReadRawCard();
            break;
        case 0x4E:
            funcWriteMasterSleep();
            break;
        case 0x4F:
            funcApplyPassword();
            break;
        case 0x50:
            funcWriteGetInfoCard();
            break;
        case 0x51:
            funcReadCardType();
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

uint8_t serialCheckSum(uint8_t *buffer, uint8_t dataSize) {
    // if at dataSize position we have packet count
    if(dataSize > SERIAL_DATA_MAX_SIZE) {
        dataSize = SERIAL_DATA_MAX_SIZE;
    }

    uint8_t len = dataSize + 2;  // + cmd/resp byte + length byte
    uint8_t sum = 0;
    for (uint8_t i = 1; i <= len; ++i) {
        sum += buffer[i];
    }
    
    return sum;
}

void serialStart(uint8_t resp) {
    serialDataPos = 3;
    serialPacketCount = 0;
    memset(serialBuffer, 0, SERIAL_PACKET_SIZE);

    serialBuffer[0] = SERIAL_START_BYTE;
    serialBuffer[1] = resp;
}

void serialAdd(uint8_t dataByte) {
    if(serialDataPos >= SERIAL_PACKET_SIZE - 1) {
        serialDataPos++;  // to indicate that we going to send packet count
        serialSend();
    }

    serialBuffer[serialDataPos] = dataByte;
    serialDataPos++;
}

void serialSend() {
    uint8_t dataSize = serialDataPos - 3; // minus start, resp code, datalen
    
    if(dataSize > SERIAL_DATA_MAX_SIZE) {
        dataSize = serialPacketCount + 0x1E;
        serialDataPos = SERIAL_PACKET_SIZE - 1;
        serialPacketCount++;
    }

    serialBuffer[2] = dataSize;
    serialBuffer[serialDataPos] = serialCheckSum(serialBuffer, dataSize);

    for(uint8_t i = 0; i <= serialDataPos; i++) {
        Serial.write(serialBuffer[i]);
    }

    serialDataPos = 3;
}

void signalError(uint8_t error) { 
    serialStart(RESP_FUNC_ERROR);
    serialAdd(error);
    serialAdd(static_cast<uint8_t>(rfidGetCardType()));
    serialSend();

    beepError();
}

void signalOK(bool beepOK = true) {
    serialStart(RESP_FUNC_OK);
    serialAdd(static_cast<uint8_t>(rfidGetCardType()));
    serialSend();

    if(beepOK) {
        beepOk();
    }
}

void funcWriteMasterTime() {
    byte dataBlock1[] = {0, MASTER_CARD_SET_TIME, 255, FW_MAJOR_VERS};
    byte dataBlock2[] = {getPwd(0), getPwd(1), getPwd(2), 0};
    byte dataBlock3[] = {serialBuffer[4], serialBuffer[3], serialBuffer[5], 0};  // month, year, day, 0
    byte dataBlock4[] = {serialBuffer[6], serialBuffer[7], serialBuffer[8], 0};  // hour, minute, second, 0
    
    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);
    
    uint8_t error = ERROR_CARD_NOT_FOUND;
    if(rfidIsCardDetected()) {
        error = ERROR_CARD_WRITE;
        
        if(rfidCardPageWrite(CARD_PAGE_INIT, dataBlock1)) {    
            if(rfidCardPageWrite(CARD_PAGE_PASS, dataBlock2)) {
                if(rfidCardPageWrite(CARD_PAGE_DATE, dataBlock3)) {
                    if(rfidCardPageWrite(CARD_PAGE_TIME, dataBlock4)) {
                        error = 0;
                    }
                }
            }
        }
    }

    rfidEnd();

    if(error) {
        signalError(error);
    } else {
        signalOK(false);
        beepTimeCardOk();
    }
}

void funcWriteMasterNum() {
    byte dataBlock1[] = {0, MASTER_CARD_SET_NUMBER, 255, FW_MAJOR_VERS};
    byte dataBlock2[] = {getPwd(0), getPwd(1), getPwd(2), 0};
    byte dataBlock3[] = {serialBuffer[3], 0, 0, 0};     // station num
    
    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

    uint8_t error = ERROR_CARD_NOT_FOUND;
    if(rfidIsCardDetected()) {
        error = ERROR_CARD_WRITE;
        
        if(rfidCardPageWrite(CARD_PAGE_INIT, dataBlock1)) {
            if(rfidCardPageWrite(CARD_PAGE_PASS, dataBlock2)) {
                if(rfidCardPageWrite(CARD_PAGE_STATION_NUM, dataBlock3)) {
                    error = 0;
                }
            }
        }
    }

    rfidEnd();

    if(error) {
        signalError(error);
    } else {
        signalOK();
    }
}

void funcWriteMasterSettings() {
    uint8_t newSettings = serialBuffer[9];
    uint8_t oldPass[] = {serialBuffer[6], serialBuffer[7], serialBuffer[8]};
    uint8_t newPass[] = {serialBuffer[3], serialBuffer[4], serialBuffer[5]};

    uint8_t gain = serialBuffer[10];
    if(gain > MAX_ANTENNA_GAIN || gain < MIN_ANTENNA_GAIN) {
        gain = DEFAULT_ANTENNA_GAIN;
    }

    byte dataBlock1[] = {0, MASTER_CARD_SETTINGS, 255, FW_MAJOR_VERS};
    byte dataBlock2[] = {oldPass[0], oldPass[1], oldPass[2], 0};
    byte dataBlock3[] = {newPass[0], newPass[1], newPass[2], 0};

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

    uint8_t error = ERROR_CARD_NOT_FOUND;
    if(rfidIsCardDetected()) {
        error = ERROR_CARD_WRITE;
        
        if(rfidCardPageWrite(CARD_PAGE_INIT, dataBlock1)) {
            if(rfidCardPageWrite(CARD_PAGE_PASS, dataBlock2)) {
                if(rfidCardPageWrite(CARD_PAGE_NEW_PASS, dataBlock3)) {
                    error = 0;
                }
            }
        }
    }

    rfidEnd();

    if(error) {
        signalError(error);
    } else {
        signalOK();
    }
}

void funcApplyPassword() {
    setPwd(&serialBuffer[3]);
    
    signalOK();
}

void funcWriteInit() {
    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);
    
    CardType cardType = rfidGetCardType();

    uint8_t ntagType = 0; // for old BS firmware?
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
    }

    byte emptyBlock[] = {0,0,0,0};
    byte dataBlock1[] = {serialBuffer[3], serialBuffer[4], ntagType, FW_MAJOR_VERS};              // card num, card type, version
    byte dataBlock2[] = {serialBuffer[5], serialBuffer[6], serialBuffer[7], serialBuffer[8]};           // unixtime
    byte dataBlock3[] = {serialBuffer[9], serialBuffer[10], serialBuffer[11], serialBuffer[12]};        // page6
    byte dataBlock4[] = {serialBuffer[13], serialBuffer[14], serialBuffer[15], serialBuffer[16]};       // page7

    uint8_t error = ERROR_CARD_NOT_FOUND;
    if(rfidIsCardDetected()) {
        error = ERROR_CARD_WRITE;
        
        if(rfidCardPageWrite(CARD_PAGE_INIT, dataBlock1)) {
            if(rfidCardPageWrite(CARD_PAGE_INIT_TIME, dataBlock2)) {
                if(rfidCardPageWrite(CARD_PAGE_INFO1, dataBlock3)) {
                    if(rfidCardPageWrite(CARD_PAGE_INFO2, dataBlock4)) {
                        error = 0;
                        uint8_t maxPage = rfidGetCardMaxPage();
                        for(uint8_t page = CARD_PAGE_START; page <= maxPage; ++page) {
                            if(!rfidCardPageWrite(page, emptyBlock)) {
                                error = ERROR_CARD_WRITE;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    rfidEnd();

    if(error) {
        signalError(error);
    } else {
        signalOK();
    }
}

void funcWriteInfo() {
    byte dataBlock1[] = {serialBuffer[3], serialBuffer[4], serialBuffer[5], serialBuffer[6]};
    byte dataBlock2[] = {serialBuffer[7], serialBuffer[8], serialBuffer[9], serialBuffer[10]};

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

    uint8_t error = ERROR_CARD_NOT_FOUND;
    if(rfidIsCardDetected()) {
        error = ERROR_CARD_WRITE;
        
        if(rfidCardPageWrite(CARD_PAGE_INFO1, dataBlock1)) {
            if(rfidCardPageWrite(CARD_PAGE_INFO2, dataBlock2)) {
                error = 0;
            }
        }
    }
    
    rfidEnd();

    if(error) {
        signalError(error);
    } else {
        signalOK();
    }
}

void funcWriteMasterLog() {
    byte dataBlock1[] = {0, MASTER_CARD_READ_DUMP, 255, FW_MAJOR_VERS};
    byte dataBlock2[] = {getPwd(0), getPwd(1), getPwd(2), 0};

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

    uint8_t error = ERROR_CARD_NOT_FOUND;
    if(rfidIsCardDetected()) {
        error = ERROR_CARD_WRITE;
        
        if(rfidCardPageWrite(CARD_PAGE_INIT, dataBlock1)) {
            if(rfidCardPageWrite(CARD_PAGE_PASS, dataBlock2)) {
                error = 0;
            }
        }
    }

    rfidEnd();

    if(error) {
        signalError(error);
    } else {
        signalOK();
    }
}

void funcWriteGetInfoCard() {
    byte dataBlock1[] = {0, MASTER_CARD_GET_INFO, 255, FW_MAJOR_VERS};
    byte dataBlock2[] = {getPwd(0), getPwd(1), getPwd(2), 0};

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

    uint8_t error = ERROR_CARD_NOT_FOUND;
    if(rfidIsCardDetected()) {
        error = ERROR_CARD_WRITE;
        
        if(rfidCardPageWrite(CARD_PAGE_INIT, dataBlock1)) {
            if(rfidCardPageWrite(CARD_PAGE_PASS, dataBlock2)) {
                error = 0;
            }
        }
    }

    rfidEnd();

    if(error) {
        signalError(error);
    } else {
        signalOK();
    }
}

void funcWriteMasterSleep() {
    byte dataBlock1[] = {0, MASTER_CARD_SLEEP, 255, FW_MAJOR_VERS};
    byte dataBlock2[] = {getPwd(0), getPwd(1), getPwd(2), 0};
    // wakeup time
    byte dataBlock3[] = {serialBuffer[4], serialBuffer[3], serialBuffer[5], 0};
    byte dataBlock4[] = {serialBuffer[6], serialBuffer[7], serialBuffer[8], 0};

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

    uint8_t error = ERROR_CARD_NOT_FOUND;
    if(rfidIsCardDetected()) {
        error = ERROR_CARD_WRITE;
        
        if(rfidCardPageWrite(CARD_PAGE_INIT, dataBlock1)) {
            if(rfidCardPageWrite(CARD_PAGE_PASS, dataBlock2)) {
                if(rfidCardPageWrite(CARD_PAGE_DATE, dataBlock3)) {
                    if(rfidCardPageWrite(CARD_PAGE_TIME, dataBlock4)) {
                        error = 0;
                    }
                }
            }
        }
    }

    rfidEnd();

    if(error) {
        signalError(error);
    } else {
        signalOK();
    }
}

void funcReadLog() {
    if(!rfidIsCardDetected()) {
        signalError(ERROR_CARD_NOT_FOUND);
        return;
    }

    byte pageData[] = {0,0,0,0};
    if(!rfidCardPageRead(CARD_PAGE_INIT, pageData)) {
        signalError(ERROR_CARD_READ);
        return;
    }

    serialStart(RESP_FUNC_LOG);
    serialAdd(pageData[0]);   // add station number

    uint8_t maxPage = rfidGetCardMaxPage();
    if(pageData[3] > 0) { // have timestamps
        serialAdd(0); // flag: have timestamps
        uint16_t timeHigh12bits = 0;
        uint32_t initTime = 0;
        for(uint8_t page = CARD_PAGE_INFO1; page <= maxPage; ++page) {
            if(!rfidCardPageRead(page, pageData)) {
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
            serialAdd(pageData[0] >> 4); // card number first byte
            serialAdd(pageData[0] << 4 | pageData[1] >> 4); // card number second byte
            uint32_t punchTime = ((uint32_t)pageData[1] & 0x0f) << 16;
            punchTime |= (uint32_t)pageData[2] << 8;
            punchTime |= pageData[3];
            uint16_t currentTimeHigh12bits = timeHigh12bits;
            if(punchTime < initTime) {
                currentTimeHigh12bits += 0x10;
            }
            serialAdd(currentTimeHigh12bits >> 8);
            serialAdd(currentTimeHigh12bits&0xf0 | pageData[1]&0x0f);
            serialAdd(pageData[2]);
            serialAdd(pageData[3]);
        }
    } else {
        for(uint8_t page = CARD_PAGE_DUMP_START; page <= maxPage; ++page) {
            if(!rfidCardPageRead(page, pageData)) {
                signalError(ERROR_CARD_READ);
                return;
            }
                
            for(uint8_t i = 0; i < 4; i++) {
                for(uint8_t y = 0; y < 8; y++) {
                    if(pageData[i] & (1 << y)) {
                        uint16_t num = (page - CARD_PAGE_DUMP_START)*32 + i*8 + y;
                        uint8_t first = (num&0xFF00)>>8;
                        uint8_t second = num&0x00FF; 
                        serialAdd(first);
                        serialAdd(second);
                    }
                }
            }
        }
    }

    serialSend();
    beepOk();
}

void funcReadCard() {
    // Don't signal error to prevent discontiniuos beep in the poll mode
    if(!rfidIsCardDetected()) {
        return;
    }

    byte pageData[] = {0,0,0,0};
    if(!rfidCardPageRead(CARD_PAGE_INIT, pageData)) {
        return;
    }
    serialStart(RESP_FUNC_MARKS);
    // Output the card number
    serialAdd(pageData[0]);
    serialAdd(pageData[1]);

    if(!rfidCardPageRead(CARD_PAGE_INIT_TIME, pageData)) {
        return;
    }
    uint8_t timeHighByte = pageData[0];
    uint32_t initTime = pageData[1];
    initTime <<= 8;
    initTime |= pageData[2];
    initTime <<= 8;
    initTime |= pageData[3];

    if(!rfidCardPageRead(CARD_PAGE_INFO1, pageData)) {
        return;
    }
    // Output page 6
    for(uint8_t i = 0; i < 4; i++) {
        serialAdd(pageData[i]);
    }

    if(!rfidCardPageRead(CARD_PAGE_INFO2, pageData)) {
        return;
    }
    // Output page 7
    for(uint8_t i = 0; i < 4; i++) {
        serialAdd(pageData[i]);
    }

    uint8_t maxPage = rfidGetCardMaxPage();

    for(uint8_t page = CARD_PAGE_START; page <= maxPage; ++page) {
        if(!rfidCardPageRead(page, pageData)) {
            return;
        }

        if(pageData[0] == 0) { // no new punches
            return;
        }
        // Output station number
        serialAdd(pageData[0]);

        uint32_t markTime = pageData[1];
        markTime <<= 8;
        markTime |= pageData[2];
        markTime <<= 8;
        markTime |= pageData[3];

        // for example, we have init time 0x00FFFFFF
        // all mark time will be 0x01xxxxxx
        // in this case we have to add 1 to timeHighByte
        if(markTime < initTime) {
            serialAdd(timeHighByte + 1);
        } else {
            serialAdd(timeHighByte);
        }
        // Output time
        serialAdd(pageData[1]);
        serialAdd(pageData[2]);
        serialAdd(pageData[3]);
    }

    serialSend();
}

void funcReadRawCard() {
    uint8_t error = ERROR_CARD_NOT_FOUND;
    byte pageData[] = {0,0,0,0};

    serialStart(RESP_FUNC_RAW_DATA);

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

    if(rfidIsCardDetected()) {
        error = ERROR_CARD_READ;
        uint8_t maxPage = rfidGetCardMaxPage();

        for(uint8_t page = CARD_PAGE_INIT; page <= maxPage; page++) {
            if(!rfidCardPageRead(page, pageData)) {
                error = ERROR_CARD_READ;
                break;
            }
            error = 0;
            serialAdd(page);
            for(uint8_t i = 0; i < 4; i++) {
                serialAdd(pageData[i]);
            }
        }   
    }

    rfidEnd();

    if(error) {
        signalError(error);
    } else {
        serialSend();
        beepOk();
    }
}

void funcReadCardType() {
    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);
    rfidEnd();

    serialStart(RESP_FUNC_CARD_TYPE);
    serialAdd(static_cast<uint8_t>(rfidGetCardType()));
    serialSend();
}

void funcGetVersion() {
    serialStart(RESP_FUNC_VERSION);

    serialAdd(HW_VERS);
    serialAdd(FW_MAJOR_VERS);
    serialAdd(FW_MINOR_VERS);

    serialSend();
}

uint8_t pwd[] = {0, 0, 0};

void setPwd(uint8_t newPwd[]) {
    memcpy(pwd, newPwd, 3);
}

uint8_t getPwd(uint8_t i) {
    return pwd[i];
}

