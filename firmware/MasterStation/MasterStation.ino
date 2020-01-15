#include <sportiduino.h>

#define HW_VERS           1
#define FW_MAJOR_VERS     6
#define FW_MINOR_VERS     3

#define FW_VERS (((FW_MAJOR_VERS - 1) << 2) | FW_MINOR_VERS)

// bits7:6 are hardware version, bits5:2 are firmware major version and bits1:0 are firmware minor version
#define VERS    (((HW_VERS - 1) << 6) | FW_VERS)

//-----------------------------------------------------------
// HARDWARE

#define BUZ             3
#define LED             4
#define RC522_RST_PIN   9
#define RC522_SS_PIN    10

// the third parameter should be the frequency of your buzzer if you solded the buzzer without a generator else 0
#define beep(ms,n) beep_w(LED,BUZ,0,ms,n)

//-----------------------------------------------------------
// SIGNALS

#define BEEP_TIME_CARD_OK beep(500,3)
#define BEEP_ERROR beep(100, 3)
#define BEEP_OK beep(500, 1)

//-----------------------------------------------------------
// CONST

#define SERIAL_TIMEOUT          10
#define SERIAL_PACKET_SIZE      32
#define SERIAL_START_BYTE       0xFE
#define SERIAL_DATA_MAX_SIZE    28

#define ERROR_COM               0x01
#define ERROR_CARD_WRITE        0x02
#define ERROR_CARD_READ         0x03
#define ERROR_EEPROM_READ       0x04
#define ERROR_CARD_NOT_FOUND    0x05

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

void serialClearBuffer();
uint8_t serialCheckSum();
void serialSend(uint8_t func, uint8_t data);
void serialFlush(uint8_t func);
void signalError(uint8_t error);
void signalOK(bool beepOK = true);

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

    readPwdSettings();
}

void loop() { 
    if(Serial.available() > 0) {
        serialClearBuffer();
        
        Serial.readBytes(serialBuffer, SERIAL_PACKET_SIZE);
        
        uint8_t dataSize = serialBuffer[2];
        
        if(dataSize > SERIAL_DATA_MAX_SIZE) {
            dataSize = SERIAL_DATA_MAX_SIZE;  
        }
        
        if(serialBuffer[0] != SERIAL_START_BYTE || serialBuffer[dataSize + 3] != serialCheckSum()) {
            signalError(ERROR_COM);
        } else {
            findFunc();
        }
    }
}

void findFunc() {
    switch(serialBuffer[1]) {
        case 0x41:
            funcWriteMasterTime();
            break;
        case 0x42:
            funcWriteMasterNum();
            break;
        case 0x43:
            funcWriteMasterPass();
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
            funcReadLog();
            break;
        case 0x4B:
            funcReadCard();
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
            BEEP_ERROR;
            break;
        case 0x59:
            BEEP_OK;
            break;
    }
}

uint8_t serialCheckSum() {
    uint8_t sum = 0;
    uint8_t dataSize = serialBuffer[2];
    if(dataSize > SERIAL_DATA_MAX_SIZE) {
        dataSize = SERIAL_DATA_MAX_SIZE;
    }
    uint8_t len = dataSize + 3;  // + func byte + length byte + offset from start
    
    for (uint8_t i = 1; i < len; i++) {
        sum += serialBuffer[i];
    }
    
    return sum;
}

void serialClearBuffer() {
    serialDataPos = 3;
    serialPacketCount = 0;
    
    for(uint8_t k = 0; k < SERIAL_PACKET_SIZE; k++) {
        serialBuffer[k] = 0;
    }
}

void serialSend(uint8_t func, uint8_t data) { 
    if(serialDataPos >= SERIAL_PACKET_SIZE - 1) {
        serialDataPos++;  // to indicate that we going to send packet count
        serialFlush(func);
    }

    serialBuffer[serialDataPos] = data;
    serialDataPos++;
}

void serialFlush(uint8_t func) {
    uint8_t dataSize = serialDataPos - 3; // minus start,func,datalen
    
    if(dataSize > SERIAL_DATA_MAX_SIZE) {
        dataSize = serialPacketCount + 0x1E;
        serialDataPos = SERIAL_PACKET_SIZE - 1;
        serialPacketCount++;
    }
    
    serialBuffer[0] = SERIAL_START_BYTE;
    serialBuffer[1] = func;
    serialBuffer[2] = dataSize;
    serialBuffer[serialDataPos] = serialCheckSum();

    for(uint8_t i = 0; i <= serialDataPos; i++) {
        Serial.write(serialBuffer[i]);
    }

    serialDataPos = 3;
}

void signalError(uint8_t error) { 
    serialClearBuffer();

    serialSend(RESP_FUNC_ERROR, error);
    serialSend(RESP_FUNC_ERROR, rfidGetCardType());
    serialFlush(RESP_FUNC_ERROR);

    BEEP_ERROR;
}

void signalOK(bool beepOK) {
    serialClearBuffer();
    
    serialSend(RESP_FUNC_OK, rfidGetCardType());
    serialFlush(RESP_FUNC_OK);

    if(beepOK) {
        BEEP_OK;
    }
}

void funcWriteMasterTime() {
    uint8_t error = ERROR_CARD_NOT_FOUND;

    byte dataBlock1[] = {0, MASTER_CARD_SET_TIME, 255, FW_VERS};
    byte dataBlock2[] = {getPwd(0), getPwd(1), getPwd(2), 0};
    byte dataBlock3[] = {serialBuffer[4], serialBuffer[3], serialBuffer[5], 0};  // month, year, day, 0
    byte dataBlock4[] = {serialBuffer[6], serialBuffer[7], serialBuffer[8], 0};  // hour, minute, second, 0
    
    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);
    
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
        BEEP_TIME_CARD_OK;
    }
}

void funcWriteMasterNum() {
    uint8_t error = ERROR_CARD_NOT_FOUND;

    byte dataBlock1[] = {0, MASTER_CARD_SET_NUMBER, 255, FW_VERS};
    byte dataBlock2[] = {getPwd(0), getPwd(1), getPwd(2), 0};
    byte dataBlock3[] = {serialBuffer[3], 0, 0, 0};     // station num
    
    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

    if(rfidIsCardDetected()) {
        error = ERROR_CARD_WRITE;
        
        if(rfidCardPageWrite(CARD_PAGE_INIT, dataBlock1)) {
            if(rfidCardPageWrite(CARD_PAGE_PASS, dataBlock2)) {
                if(rfidCardPageWrite(CARD_PAGE_STATION_NUM, dataBlock3))
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

void funcWriteMasterPass() {
    uint8_t error = ERROR_CARD_NOT_FOUND;
    
    uint8_t gain = serialBuffer[10];
    uint8_t newSettings = serialBuffer[9];
    uint8_t oldPass[] = {serialBuffer[6], serialBuffer[7], serialBuffer[8]};
    uint8_t newPass[] = {serialBuffer[3], serialBuffer[4], serialBuffer[5]};
    
    if(gain > MAX_ANTENNA_GAIN || gain < MIN_ANTENNA_GAIN)
        gain = DEFAULT_ANTENNA_GAIN;

    byte dataBlock1[] = {0, MASTER_CARD_SET_PASS, 255, FW_VERS};
    byte dataBlock2[] = {oldPass[0], oldPass[1], oldPass[2], gain};
    byte dataBlock3[] = {newPass[0], newPass[1], newPass[2], newSettings};

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

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
        setSettings(newSettings);
        setAntennaGain(gain);
        setPwd(newPass[0], newPass[1], newPass[2]);

        signalOK();
    }
}

void funcApplyPassword() {
    setPwd(serialBuffer[3], serialBuffer[4], serialBuffer[5]);
    
    signalOK();
}

void funcWriteInit() {
    uint8_t error = ERROR_CARD_NOT_FOUND;
    
    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);
    
    uint8_t ntagType = rfidGetCardType();

    if(ntagType == 0x6D) {
        ntagType = 6;
    } else if(ntagType == 0x3E) {
        ntagType = 5;
    } else if(ntagType == 0x12) {
        ntagType = 3;
    } else {
        ntagType = 0;
    }

    byte emptyBlock[] = {0,0,0,0};
    byte dataBlock1[] = {serialBuffer[3], serialBuffer[4], ntagType, FW_VERS};              // card num, card type, version
    byte dataBlock2[] = {serialBuffer[5], serialBuffer[6], serialBuffer[7], serialBuffer[8]};           // unixtime
    byte dataBlock3[] = {serialBuffer[9], serialBuffer[10], serialBuffer[11], serialBuffer[12]};        // page6
    byte dataBlock4[] = {serialBuffer[13], serialBuffer[14], serialBuffer[15], serialBuffer[16]};       // page7

    if(rfidIsCardDetected()) {
        error = ERROR_CARD_WRITE;
        
        if(rfidCardPageWrite(CARD_PAGE_INIT, dataBlock1)) {
            if(rfidCardPageWrite(CARD_PAGE_INIT_TIME, dataBlock2)) {
                if(rfidCardPageWrite(CARD_PAGE_INFO1, dataBlock3)) {
                    if(rfidCardPageWrite(CARD_PAGE_INFO2, dataBlock4)) {
                        uint8_t maxPage = rfidGetCardMaxPage();
                        for(uint8_t page = CARD_PAGE_START; page <= maxPage; page++) {
                            if(!rfidCardPageWrite(page, emptyBlock)) {
                                break;
                            }
                        }

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

void funcWriteInfo() {
    uint8_t error = ERROR_CARD_NOT_FOUND;
    
    byte dataBlock1[] = {serialBuffer[3], serialBuffer[4], serialBuffer[5], serialBuffer[6]};
    byte dataBlock2[] = {serialBuffer[7], serialBuffer[8], serialBuffer[9], serialBuffer[10]};

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

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
    uint8_t error = ERROR_CARD_NOT_FOUND;

    byte dataBlock1[] = {0, MASTER_CARD_READ_DUMP, 255, FW_VERS};
    byte dataBlock2[] = {getPwd(0), getPwd(1), getPwd(2), 0};

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);
    
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
    uint8_t error = ERROR_CARD_NOT_FOUND;
    
    byte dataBlock1[] = {0, MASTER_CARD_GET_INFO, 255, FW_VERS};
    byte dataBlock2[] = {getPwd(0), getPwd(1), getPwd(2), 0};

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

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
    uint8_t error = ERROR_CARD_NOT_FOUND;
    
    byte dataBlock1[] = {0, MASTER_CARD_SLEEP, 255, FW_VERS};
    byte dataBlock2[] = {getPwd(0), getPwd(1), getPwd(2), 0};
    // wakeup time
    byte dataBlock3[] = {serialBuffer[4], serialBuffer[3], serialBuffer[5], 0};
    byte dataBlock4[] = {serialBuffer[6], serialBuffer[7], serialBuffer[8], 0};

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

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
    uint8_t error = ERROR_CARD_NOT_FOUND;
    byte pageData[] = {0,0,0,0};
    uint8_t func = RESP_FUNC_LOG;
    
    serialClearBuffer();

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

    if(rfidIsCardDetected()) {
        error = ERROR_CARD_READ;
        
        if(rfidCardPageRead(CARD_PAGE_INIT, pageData)) {
            serialSend(func, pageData[0]);   // add station number
            
            uint8_t maxPage = rfidGetCardMaxPage();
            
            for(uint8_t page = CARD_PAGE_DUMP_START; page <= maxPage; page++) {
                if(rfidCardPageRead(page, pageData)) {
                    error = 0;
                    
                    for(uint8_t i = 0; i < 4; i++) {
                        for(uint8_t y = 0; y < 8; y++) {
                            if(pageData[i] & (1 << y)) {
                                uint16_t num = (page - CARD_PAGE_DUMP_START)*32 + i*8 + y;
                                uint8_t first = (num&0xFF00)>>8;
                                uint8_t second = num&0x00FF; 
                                serialSend(func, first);
                                serialSend(func, second);      
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
        serialFlush(func);
        BEEP_OK;
    }
}

void funcReadCard() {
    uint8_t error = ERROR_CARD_NOT_FOUND;
    byte pageData[] = {0,0,0,0};
    uint8_t func = RESP_FUNC_MARKS;
    uint8_t timeHighByte = 0;
    uint32_t initTime = 0;
    uint32_t markTime = 0;

    serialClearBuffer();

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

    if(rfidIsCardDetected()) {
        error = ERROR_CARD_READ;
        
        if(rfidCardPageRead(CARD_PAGE_INIT, pageData)) {
            // Output the card number
            serialSend(func, pageData[0]);
            serialSend(func, pageData[1]);
            if(rfidCardPageRead(CARD_PAGE_INIT_TIME, pageData)) {
                timeHighByte = pageData[0];

                initTime = pageData[1];
                initTime <<= 8;
                initTime |= pageData[2];
                initTime <<= 8;
                initTime |= pageData[3];

                if(rfidCardPageRead(CARD_PAGE_INFO1, pageData)) {
                    // Output page 6
                    for(uint8_t i = 0; i < 4; i++)
                        serialSend(func, pageData[i]);

                    if(rfidCardPageRead(CARD_PAGE_INFO2, pageData)) {
                        // Output page 7
                        for(uint8_t i = 0; i < 4; i++)
                            serialSend(func, pageData[i]);

                        uint8_t maxPage = rfidGetCardMaxPage();

                        for(uint8_t page = CARD_PAGE_START; page <= maxPage; page++) {
                            if(rfidCardPageRead(page, pageData)) {
                                error = 0;
                                
                                if(pageData[0] > 0) {
                                    // Output station number                  
                                    serialSend(func, pageData[0]);

                                    markTime = pageData[1];
                                    markTime <<= 8;
                                    markTime |= pageData[2];
                                    markTime <<= 8;
                                    markTime |= pageData[3];

                                    // for example, we have init time 0x00FFFFFF
                                    // all mark time will be 0x01xxxxxx
                                    // in this case we have to add 1 to timeHighByte
                                    if(markTime < initTime) {
                                        serialSend(func, timeHighByte + 1);
                                    } else {
                                        serialSend(func, timeHighByte);
                                    }
                                    // Output time
                                    serialSend(func, pageData[1]);
                                    serialSend(func, pageData[2]);
                                    serialSend(func, pageData[3]);
                                } else {
                                    // fix issue #56
                                    // no new marks will be
                                    // stop to read card and send data as soon as possible
                                    // else we will get serial timeout error on the host side
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    rfidEnd();

    // fix issue #56
    // Commented to prevent discontiniuos beep in the poll mode
    //if(error)
    //{
    //  signalError(error);
    //}
    //else
    //{
    //  serialFlush(func);
    //  BEEP_OK;
    //}

    if(!error) {
        serialFlush(func);
    }
}

void funcReadRawCard() {
    uint8_t error = ERROR_CARD_NOT_FOUND;
    byte pageData[] = {0,0,0,0};
    uint8_t func = RESP_FUNC_RAW_DATA;
    
    serialClearBuffer();

    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);

    if(rfidIsCardDetected()) {
        error = ERROR_CARD_READ;
        uint8_t maxPage = rfidGetCardMaxPage();
    
        for(uint8_t page = CARD_PAGE_INIT; page <= maxPage; page++) {
            if(rfidCardPageRead(page, pageData)) {
                error = 0;
                serialSend(func, page);
                for(uint8_t i = 0; i < 4; i++) {
                    serialSend(func, pageData[i]);
                }
            }
        }   
    }

    rfidEnd();

    if(error) {
        signalError(error);
    } else {
        serialFlush(func);
        BEEP_OK;
    }
}

void funcReadCardType() {
    uint8_t func = RESP_FUNC_CARD_TYPE;
    rfidBegin(RC522_SS_PIN, RC522_RST_PIN);
    rfidEnd();

    serialSend(func, rfidGetCardType());
    serialFlush(func);
}

void funcGetVersion() {
    uint8_t func = RESP_FUNC_VERSION;
    
    serialClearBuffer();

    serialSend(func, VERS);
    serialSend(func, getPwd(0));
    serialSend(func, getPwd(1));
    serialSend(func, getPwd(2));
    serialSend(func, getSettings());
    serialSend(func, getAntennaGain());

    serialFlush(func);
}
