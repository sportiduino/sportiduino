#include <sportiduino.h>
#include "sportidentprotocol.h"

#define HW_VERS           1
#define FW_MAJOR_VERS     9
// If FW_MINOR_VERS more than MAX_FW_MINOR_VERS this is beta version HW_VERS.FW_MINOR_VERS.0-beta.X
// where X is (FW_MINOR_VERS - MAX_FW_MINOR_VERS)
#define FW_MINOR_VERS     1


//-----------------------------------------------------------
// HARDWARE

#define BUZZ_PIN        3
#define LED_PIN         4
#define RC522_RST_PIN   9
#define RC522_SS_PIN    10

// Set BUZZER_FREQUENCY by running "make buzzfreq=2500"
#ifndef BUZZER_FREQUENCY
    // or change here
    #define BUZZER_FREQUENCY 4000 // 0 for buzzer with generator
#endif

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
    RESP_FUNC_BACKUP      = 0x61,
    RESP_FUNC_MARKS       = 0x63,
    RESP_FUNC_RAW_DATA    = 0x65,
    RESP_FUNC_VERSION     = 0x66,
    RESP_FUNC_SETTINGS    = 0x67,
    RESP_FUNC_MODE        = 0x69,
    RESP_FUNC_CARD_TYPE   = 0x70,
    RESP_FUNC_ERROR       = 0x78,
    RESP_FUNC_OK          = 0x79
};

#define EEPROM_CONFIG_ADDR 0x3EE

//-----------------------------------------------------------

using SiProto = SportidentProtocol;

struct __attribute__((packed)) Configuration {
    uint8_t antennaGain;
    int8_t timezone; // timezone in 1/4 hours
};

//-----------------------------------------------------------
// FUNCTIONS

inline void beep(uint16_t ms, uint8_t n) { beep_w(LED_PIN, BUZZ_PIN, BUZZER_FREQUENCY, ms, n); }
inline void beepTimeCardOk() { beep_w(LED_PIN, BUZZ_PIN, BUZZER_FREQUENCY, 500, 3, 500); delay(500); beep(1000, 1); }
inline void beepError() { beep(100, 3); }
inline void beepOk() { beep(500, 1); }

// Declatarions for building by Arduino-Makefile
void signalError(uint8_t error);
void handleCmd(uint8_t cmdCode, uint8_t *data, uint8_t dataSize);
void handleSiCmd(uint8_t cmdCode, uint8_t *data, uint8_t dataSize);
void sieDetectCard();
void sieCardRemoved();
void sieCardReadError();
bool sieSendDataBlock(uint8_t blockNumber);
bool sieSendAllDataBlocks(bool shortFormat);
void setPwd(uint8_t newPwd[]);
uint8_t getPwd(uint8_t i);

//-----------------------------------------------------------
// VARIABLES
static Configuration config;
static Rfid rfid;
static SerialProtocol serialProto;
static SiProto siProto;
static bool sieMode = true; // Sportident emulation mode (continuos readout)

void setup() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZ_PIN, OUTPUT);
    pinMode(RC522_RST_PIN, OUTPUT);
    pinMode(RC522_SS_PIN, OUTPUT);
    
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZ_PIN, LOW);
    digitalWrite(RC522_RST_PIN, LOW);

    readConfig(&config, sizeof(Configuration), EEPROM_CONFIG_ADDR);
    if(config.antennaGain > MAX_ANTENNA_GAIN || config.antennaGain < MIN_ANTENNA_GAIN) {
        config.antennaGain = DEFAULT_ANTENNA_GAIN;
        config.timezone = 0;
    }

    rfid.init(RC522_SS_PIN, RC522_RST_PIN, config.antennaGain);
    serialProto.init(SERIAL_START_BYTE, 38400);

    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
}

void loop() { 
    if(sieMode) {
        rfid.begin(config.antennaGain);
        sieDetectCard();
        rfid.end();
        delay(50);
    }
}

void serialEvent() {
    bool error = false;
    uint8_t cmdCode = 0;
    uint8_t dataSize = 0;

    uint8_t *data = serialProto.read(&error, &cmdCode, &dataSize);
    if(error) {
        signalError(ERROR_SERIAL);
        return;
    }
    if(data) {
        sieMode = false;
        handleCmd(cmdCode, data, dataSize);
        return;
    }
    data = siProto.read(&error, &cmdCode, &dataSize);
    if(error) {
        siProto.error();
        return;
    }
    if(data) {
        sieMode = true;
        handleSiCmd(cmdCode, data, dataSize);
        return;
    }
    serialProto.dropByte();
}

void signalError(uint8_t error) { 
    serialProto.start(RESP_FUNC_ERROR);
    serialProto.add(error);
    serialProto.add((uint8_t)rfid.getCardType());
    serialProto.send();

    beepError();
}

void signalOK(bool beep = true) {
    serialProto.start(RESP_FUNC_OK);
    serialProto.add((uint8_t)rfid.getCardType());
    serialProto.send();

    if(beep) {
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

void funcWriteMasterPassword(uint8_t *serialData, uint8_t dataSize) {
    if(dataSize != 3) {
        signalError(ERROR_BAD_DATASIZE);
        return;
    }

    uint8_t error = writeMasterCard(MASTER_CARD_PASSWORD, serialData, dataSize);

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

void funcReadSettings(uint8_t *serialData, uint8_t dataSize) {
    serialProto.start(RESP_FUNC_SETTINGS);
    serialProto.add((uint8_t*)&config, sizeof(Configuration));
    serialProto.send();
}

void funcWriteSettings(uint8_t *serialData, uint8_t dataSize) {
    if(dataSize != sizeof(Configuration)) {
        signalError(ERROR_BAD_DATASIZE);
        return;
    }
    Configuration *newConfig = (Configuration*)serialData;
    if(newConfig->antennaGain < MIN_ANTENNA_GAIN || newConfig->antennaGain > MAX_ANTENNA_GAIN
        || newConfig->timezone < -12*4 || newConfig->timezone > 14*4) {
        signalError(ERROR_BAD_SETTINGS);
        return;
    }
    memcpy(&config, newConfig, sizeof(Configuration));
    writeConfig(&config, sizeof(Configuration), EEPROM_CONFIG_ADDR);
    signalOK();
}

void funcInitPaticipantCard(uint8_t *serialData, uint8_t dataSize) {
    if(!rfid.isCardDetected()) {
        signalError(ERROR_CARD_NOT_FOUND);
        return;
    }

    uint8_t maxPage = rfid.getCardMaxPage();
    digitalWrite(LED_PIN, HIGH);
    if(!rfid.cardErase(CARD_PAGE_START, maxPage)) {
        signalError(ERROR_CARD_WRITE);
        digitalWrite(LED_PIN, LOW);
        return;
    }
    digitalWrite(LED_PIN, LOW);

    byte data[] = {
        serialData[0], serialData[1], 0, FW_MAJOR_VERS,                 // card num, 0, fw version
        serialData[2], serialData[3], serialData[4], serialData[5],     // unixtime
        serialData[6], serialData[7], serialData[8], serialData[9],     // page6
        serialData[10], serialData[11], serialData[12], serialData[13]  // page7
    };

    if(!rfid.cardWrite(CARD_PAGE_INIT, data, sizeof(data))) {
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

void funcWriteMasterBackup(uint8_t*, uint8_t) {
    uint8_t error = writeMasterCard(MASTER_CARD_READ_BACKUP);

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

void funcReadBackup(uint8_t*, uint8_t) {
    if(!rfid.isCardDetected()) {
        signalError(ERROR_CARD_NOT_FOUND);
        return;
    }

    byte pageData[] = {0,0,0,0};
    if(!rfid.cardPageRead(CARD_PAGE_INIT, pageData)) {
        signalError(ERROR_CARD_READ);
        return;
    }

    serialProto.start(RESP_FUNC_BACKUP);
    serialProto.add(pageData[0]);   // add station number

    uint8_t maxPage = rfid.getCardMaxPage();
    if(pageData[3] == 1) { // old format with timestamps
        serialProto.add(0xff); // flag: have timestamps
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
    } else if(pageData[3] >= 10) { // new format (FW version 10 or greater)
        serialProto.add(0xff); // flag: have timestamps
        uint16_t lastTimeHigh16bits = 0;
        for(uint8_t page = CARD_PAGE_INFO1; page <= maxPage; ++page) {
            if(!rfid.cardPageRead(page, pageData)) {
                signalError(ERROR_CARD_READ);
                return;
            }

            if(pageData[0] == 0 && pageData[1] == 0) {
                uint16_t timeHigh16bits = byteArrayToUint32(pageData) & 0xffff;
                if(timeHigh16bits > 0 && timeHigh16bits != lastTimeHigh16bits) {
                    lastTimeHigh16bits = timeHigh16bits;
                }
                continue;
            }
            serialProto.add(pageData[0]); // card number first byte
            serialProto.add(pageData[1]); // card number second byte
            serialProto.add(lastTimeHigh16bits >> 8);
            serialProto.add(lastTimeHigh16bits & 0xff);
            serialProto.add(pageData[2]); // timestamps
            serialProto.add(pageData[3]); // timestamps
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
        signalError(ERROR_CARD_READ);
        return;
    }
    if(pageData[2] == 0xff) {
        return;
    }
    serialProto.start(RESP_FUNC_MARKS);
    // Output the card number
    serialProto.add(pageData[0]);
    serialProto.add(pageData[1]);

    if(!rfid.cardPageRead(CARD_PAGE_INIT_TIME, pageData)) {
        signalError(ERROR_CARD_READ);
        return;
    }
    uint8_t timeHighByte = pageData[0];
    uint32_t initTime = pageData[1];
    initTime <<= 8;
    initTime |= pageData[2];
    initTime <<= 8;
    initTime |= pageData[3];

    if(!rfid.cardPageRead(CARD_PAGE_INFO1, pageData)) {
        signalError(ERROR_CARD_READ);
        return;
    }
    // Output page 6
    for(uint8_t i = 0; i < 4; i++) {
        serialProto.add(pageData[i]);
    }

    if(!rfid.cardPageRead(CARD_PAGE_INFO2, pageData)) {
        signalError(ERROR_CARD_READ);
        return;
    }
    // Output page 7
    for(uint8_t i = 0; i < 4; i++) {
        serialProto.add(pageData[i]);
    }

    uint8_t maxPage = rfid.getCardMaxPage();

    for(uint8_t page = CARD_PAGE_START; page <= maxPage; ++page) {
        if(!rfid.cardPageRead(page, pageData)) {
            signalError(ERROR_CARD_READ);
            return;
        }

        if(pageIsEmpty(pageData)) { // no new punches
            break;
        }
        // Output station number
        serialProto.add(pageData[0]);

        uint32_t punchTime = pageData[1];
        punchTime <<= 8;
        punchTime |= pageData[2];
        punchTime <<= 8;
        punchTime |= pageData[3];

        // for example, we have init time 0x00FFFFFF
        // all mark time will be 0x01xxxxxx
        // in this case we have to add 1 to timeHighByte
        if(punchTime < initTime) {
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
    rfid.begin(config.antennaGain);
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
        case 0x43:
            callRfidFunction(funcWriteMasterPassword, data, dataSize);
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
            callRfidFunction(funcWriteMasterBackup, data, dataSize);
            break;
        case 0x48:
            callRfidFunction(funcReadBackup, data, dataSize);
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
        case 0x4D:
            funcReadSettings(data, dataSize);
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

const uint8_t fakeStationConfig[] = {
    0x00, 0x00, 0x00, 0x01, // serial number
    0xF7, // SRR-dongle configuration?
    0x36, 0x32, 0x33, // firmware (623)
    0x0A, 0x01, 0x19, // buid date
    0x91, 0x97, // model ID (BSM7-RS232, BSM7-USB)
    0x80, // memory size in kB
    0x20, 0x0D,
    0x4B, 0x08, 0x4E, 0xFA, 0x28, 
    0x0A, 0x01, 0x19, // battery date
    0x00,
    0x6D, 0xDD, // battery capacity in Ah (as multiples of 14062.5)
    0x00,
    0x00, 0x00, // backup ptr 1
    0x18, 0x04, 0xFF,
    0x03, 0x80, // backup ptr 2
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x4D, 0x70, 0xFF, 0xFF, 0xFF, 0x00, 0xC3, 
    0xFF, // read all SI6 card 8 blocks
    0x00, // SRR-dongle frequency band: 0x00="red", 0x01="blue"
    0x00, 0x00, 0x0A, 0x00, 0x00,
    0x00, 0x00, 0xFF,
    0x00, // memory overflow if != 0x00
    0xEF, 0xFF, 0x00, 0x24, 0xFE, 0xC0, 0xFF, 0xFF, 0x19, 0x99,
    0x05, 0x1E, 0x7F, 0xF8, 0x85, 0x0C, 0x01, 0x01, 0xA6, 0xE0,
    0x6F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x30, 0x30, 0x30, 0x35, 0x7D, 0x20,
    0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x30, // program
    0x05, // readout mode
    0x01, // station code
    0x35, // feedback
    0x05, // extended protocol with handshake
    0x10, 0x08, 0x01, // wakeup date
    0x00, 0x00, 0x00, // wakeup time
    0x00, 0x1C, 0x20, // sleep time
    0x00, 0x78
};

void handleSiCmd(uint8_t cmdCode, uint8_t *data, uint8_t dataSize) {
    switch(cmdCode) {
        case SiProto::BCMD_GET_SYS_VAL:
            {
                siProto.start(cmdCode);
                siProto.add(0);
                siProto.add(fakeStationConfig, 14);
                siProto.send();
            }
            break;
        case SiProto::BCMD_SET_MS:
        case SiProto::CMD_SET_MS:
            {
                siProto.start(cmdCode);
                siProto.add(0x4d);
                siProto.send();
            }
            break;
        case SiProto::CMD_GET_SYS_VAL:
            {
                uint8_t offset = data[0];
                if(offset > sizeof(fakeStationConfig)) {
                    return;
                }
                uint8_t len = data[1];
                uint8_t maxDataLen = sizeof(fakeStationConfig) - offset;
                if(len > 0x7F) {
                    len = maxDataLen;
                }
                siProto.start(cmdCode);
                siProto.add(offset);
                siProto.add(&fakeStationConfig[offset], len);
                siProto.send();
            }
            break;
        case SiProto::CMD_GET_TIME:
            {
                const uint8_t emptyData[] = {
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                };
                siProto.start(cmdCode);
                siProto.add(emptyData, sizeof(emptyData));
                siProto.send();
            }
            break;
        case SiProto::BCMD_READ_SI6:
        case SiProto::CMD_READ_SI6:
            {
                uint8_t blockNumber = data[0];
                rfid.begin(config.antennaGain);
                bool autosend = false; // not implemented
                if(blockNumber == 0x00 && autosend) {
                    if(!sieSendAllDataBlocks(true)) {
                        sieCardReadError();
                    }
                } else if(blockNumber == 0x08) {
                    if(!sieSendAllDataBlocks(false)) {
                        sieCardReadError();
                    }
                } else {
                    if(!sieSendDataBlock(blockNumber)) {
                        sieCardReadError();
                    }
                }
                rfid.end();
            }
            break;
        case SiProto::ACK:
            {
                sieCardRemoved();
                beepOk();
            }
            break;
        default:
            siProto.error();
            break;
    }
}

uint8_t *readCardNumber() {
    if(!rfid.isNewCardDetected()) {
        return nullptr;
    }

    byte pageData[] = {0,0,0,0};

    if(!rfid.cardPageRead(CARD_PAGE_INIT, pageData)) {
        return nullptr;
    }

    if(pageData[2] == MASTER_CARD_SIGN) {
        return nullptr;
    }

    static uint8_t cardNum[2];
    cardNum[0] = pageData[0];
    cardNum[1] = pageData[1];
    return cardNum;
}

uint32_t readInitTime() {
    byte pageData[4];
    if(!rfid.cardPageRead(CARD_PAGE_INIT_TIME, pageData)) {
        return 0;
    }
    return byteArrayToUint32(pageData);
}

uint32_t getPunchTime(const byte *pageData, uint32_t initTime) {
    uint32_t punchTime = (byteArrayToUint32(pageData)&0x00FFFFFF) | (initTime&0xFF000000);
    if(punchTime < initTime) {
        punchTime += (uint32_t)1 << 24;
    }
    return punchTime;
}

bool readStart(uint32_t initTime, uint32_t *startTime, uint8_t *pageStartPunch) {
    *startTime = 0;
    *pageStartPunch = CARD_PAGE_START;
    const uint8_t beginPage = CARD_PAGE_START;
    const uint8_t endPage = beginPage + 10; // read only first 10 punches
    byte pageData[4];

    for(uint8_t page = beginPage; page < endPage; ++page) {
        if(!rfid.cardPageRead(page, pageData)) {
            return false;
        }

        uint8_t cp = pageData[0];
        if(cp == START_STATION_NUM) {
            *startTime = getPunchTime(pageData, initTime);
            *pageStartPunch = page;
            return true;
        }
    }
    // no start punch found
    return true;
}

bool readFinish(uint32_t initTime, uint32_t *finishTime, uint8_t *pageFinishPunch) {
    *finishTime = 0;
    *pageFinishPunch = 0;
    uint8_t newPage = 0;
    uint8_t lastNum;
    if(!findNewPage(&rfid, &newPage, &lastNum)) {
        return false;
    }
    *pageFinishPunch = newPage;
    uint8_t endPage = newPage;
    uint8_t beginPage = max(CARD_PAGE_START, endPage - 10);
    byte pageData[4];

    for(uint8_t page = endPage - 1; page >= beginPage; --page) {
        if(!rfid.cardPageRead(page, pageData)) {
            return false;
        }

        uint8_t cp = pageData[0];
        if(cp == FINISH_STATION_NUM) {
            *finishTime = getPunchTime(pageData, initTime);
            *pageFinishPunch = page;
            return true;
        }
    }
    // no finish punch found
    return true;
}

uint8_t *currentCardNumber = nullptr;
uint32_t currentCardInitTime = 0;
uint8_t currentCpCount = 0;

void sieDetectCard() {
    uint8_t *cardNum= readCardNumber();
    if(!cardNum) {
        return;
    }
    uint32_t initTime = readInitTime();
    if(!initTime) {
        return;
    }
    currentCardNumber = cardNum;
    currentCardInitTime = initTime;

    if(siProto.isLegacyMode()) {
        siProto.start(SiProto::BCMD_SI6_DETECTED);
        siProto.add(0x55);
        siProto.add(0xAA);
        siProto.add(0x00);
        siProto.add(0x00);
        siProto.add(currentCardNumber[0]);
        siProto.add(currentCardNumber[1]);
        siProto.send();
        return;
    }
    siProto.start(SiProto::CMD_SI6_DETECTED);
    siProto.add(0);
    siProto.add(0);
    siProto.add(currentCardNumber[0]);
    siProto.add(currentCardNumber[1]);
    siProto.send();
}

void sieCardRemoved() {
    if(!currentCardNumber) {
        return;
    }
    if(siProto.isLegacyMode()) {
        siProto.start(SiProto::BCMD_SI5_DETECTED);
        siProto.add(0x4F);
    } else {
        siProto.start(SiProto::CMD_SI_REMOVED);
        siProto.add(0);
        siProto.add(0);
        siProto.add(currentCardNumber[0]);
        siProto.add(currentCardNumber[1]);
    }
    siProto.send();
}

void sieCardReadError() {
    sieCardRemoved();
    beepError();
}

bool sieSendDataBlock(uint8_t blockNumber) {
    if(!currentCardNumber || !rfid.isCardDetected()) {
        //siProto.error();
        return false;
    }

    if(siProto.isLegacyMode()) {
        siProto.start(SiProto::BCMD_READ_SI6);
    } else {
        siProto.start(SiProto::CMD_READ_SI6);
    }
    siProto.add(blockNumber);
    static uint8_t blockOffset = 0;
    if(blockNumber == 0) {
        // TODO: read number of last CP
        uint8_t lastCpNum = 0;
        currentCpCount = 0;
 
        SiTimestamp clear;
        SiTimestamp check;
        SiTimestamp start;
        SiTimestamp finish;
        SiTimestamp lastCp;

        clear.fromUnixtime(currentCardInitTime, config.timezone);
        clear.cn = 0;

        uint8_t finishPunchOrEmptyPage = 0;
        uint32_t finishTime = 0;
        if(!readFinish(currentCardInitTime, &finishTime, &finishPunchOrEmptyPage)) {
            return false;
        }
        if(finishTime) {
            finish.fromUnixtime(finishTime, config.timezone);
            finish.cn = 0;
        }
        if(finishPunchOrEmptyPage) {
            currentCpCount = finishPunchOrEmptyPage - CARD_PAGE_START;
        }

        uint8_t pageStartPunch = 0;
        uint32_t startTime = 0;
        if(!readStart(currentCardInitTime, &startTime, &pageStartPunch)) {
            return false;
        }
        if(startTime) {
            start.fromUnixtime(startTime, config.timezone);
            start.cn = 0;
            blockOffset = pageStartPunch - CARD_PAGE_START + 1;
            currentCpCount -= blockOffset;
        } else {
            blockOffset = 0;
        }

        uint8_t cti[] = {
            0x55, // card type (CTI)
            0xAA, // punches pointer (PP)
            0x00, 0x00, currentCardNumber[0], currentCardNumber[1]
        };
        Crc crc;
        crc.value = SiProto::crc16(cti, sizeof(cti));

        uint8_t data[40] = {
            0x01, 0x01, 0x01, 0x01, // structure of data
            0xED, 0xED, 0xED, 0xED, // SI6 ID
            cti[0], cti[1], cti[2], cti[3], cti[4], cti[5],
            crc.b[1], crc.b[0],
            0, lastCpNum,
            currentCpCount, static_cast<uint8_t>(currentCpCount + 1),
            finish.ptd, finish.cn, finish.pth, finish.ptl,
            start.ptd, start.cn, start.pth, start.ptl,
            check.ptd, check.cn, check.pth, check.ptl,
            clear.ptd, clear.cn, clear.pth, clear.ptl,
            lastCp.ptd, lastCp.cn, lastCp.pth, lastCp.ptl
        };
        siProto.add(data, sizeof(data));
        // Start number
        for(uint8_t i = 0; i < 4; ++i) {
            siProto.add(0xFF);
        }
        memset(data, ' ', sizeof(data));
        // Class
        siProto.add(data, 4);
        // Surname
        siProto.add(data, 20);
        // Name
        siProto.add(data, 20);
        // Country
        siProto.add(data, 4);
        // Club
        siProto.add(data, 36);
    } else if(blockNumber == 1) {
        uint8_t data[36];
        memset(data, ' ', sizeof(data));
        // User ID
        siProto.add(data, 16);
        // Mobile phone number
        siProto.add(data, 16);
        // E-mail
        siProto.add(data, 36);
        // Street
        siProto.add(data, 20);
        // City
        siProto.add(data, 16);
        // ZIP code
        siProto.add(data, 8);
        // Sex
        siProto.add(data, 4);
        // Day of birth
        siProto.add(data, 8);
		// Date of production
        for(uint8_t i = 0; i < 4; ++i) {
            siProto.add(0xff);
        }
    } else {
        uint8_t maxPage = rfid.getCardMaxPage();

        const uint8_t blockOrder[] = {
            0, 1, 4, 5, 6, 7, 2, 3
        };

        uint8_t blockAddress = CARD_PAGE_START + (blockOrder[blockNumber] - 2)*32 + blockOffset;
        byte pageData[4];

        for(uint8_t page = blockAddress; page < blockAddress + 32; ++page) {
            SiTimestamp siTimestamp;
            if(page <= maxPage) {
                if(!rfid.cardPageRead(page, pageData)) {
                    return false;
                }

                uint8_t cp = pageData[0];
                if(cp == START_STATION_NUM || cp == FINISH_STATION_NUM) {
                    siTimestamp.cn = 0;
                } else if(cp != 0) {
                    siTimestamp.cn = cp;
                    uint32_t punchTime = getPunchTime(pageData, currentCardInitTime);
                    siTimestamp.fromUnixtime(punchTime, config.timezone);
                }
            }
            siProto.add(siTimestamp.ptd);
            siProto.add(siTimestamp.cn);
            siProto.add(siTimestamp.pth);
            siProto.add(siTimestamp.ptl);
        }
    }
    siProto.send();
    return true;
}

bool sieSendAllDataBlocks(bool shortFormat) {
    for(uint8_t blockNumber = 0; blockNumber < 8; ++blockNumber) {
        if(!sieSendDataBlock(blockNumber)) {
            return false;
        }
        if(blockNumber == 0 && (currentCpCount <= 64 || shortFormat)) {
            blockNumber = 5;
        }
    }
    return true;
}

uint8_t pwd[] = {0, 0, 0};

void setPwd(uint8_t newPwd[]) {
    memcpy(pwd, newPwd, 3);
}

uint8_t getPwd(uint8_t i) {
    return pwd[i];
}

