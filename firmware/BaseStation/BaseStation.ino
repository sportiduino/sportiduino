// To compile this project with Arduino IDE change sketchbook to <Project>/firmware

#include <EEPROM.h>
#include <Wire.h>
#include <ds3231.h>
#include <Adafruit_SleepyDog.h>
#include <PinChangeInterrupt.h>
#include <sportiduino.h>

// Remove a comment from a line below to compile in DEBUG mode
//#define DEBUG

#define HW_VERS         2
#define FW_MAJOR_VERS   6
#define FW_MINOR_VERS   3

#define VERS ((HW_VERS - 1) << 6) | ((FW_MAJOR_VERS - 1) << 2) | FW_MINOR_VERS

//-------------------------------------------------------------------
// HARDWARE

#if HW_VERS == 1
    
    #define BUZ           3
    #define LED           4

    #define RC522_RST     9
    #define RC522_SS      10
    #define RC522_IRQ     6

    #define DS3231_VCC    5
    #define DS3231_IRQ    A3
    // PC1 (24) This pin is not used. It is reserved for future
    #define DS3231_32K    A1
    #define DS3231_RST    A0

    #define UART_RX       0
    #define UART_TX       1
    #define SDA           A4
    #define SCL           A5
    
#else

    #define BUZ           3
    #define LED           4

    #define RC522_RST     9
    #define RC522_SS      10
    #define RC522_IRQ     6

    // It is not used anymore. Just the free pin acts as output
    #define DS3231_VCC    8
    #define DS3231_IRQ    A3
    // PD5 (9) This pin is not used. It is reserved for future
    #define DS3231_32K    5
    #define DS3231_RST    2

    #define UART_RX       0
    // It is not used anymore. Just the free pin acts as output
    #define DS3231_VCC    8
    #define UART_TX       1
    #define SDA           A4
    #define SCL           A5
    
#endif

#define UNKNOWN_PIN 0xFF

// the third parameter should be the frequency of your buzzer if you solded the buzzer without a generator else 0
#define beep(ms,n) beep_w(LED,BUZ,0,ms,n)

#define SERIAL_BAUDRATE   9600

//-------------------------------------------------------------------


// 31 days = 2678400 (seconds)
#define CARD_EXPIRE_TIME 2678400L

#define MAX_CARD_NUM_TO_LOG 4000

#define DEFAULT_STATION_NUM       CHECK_STATION_NUM

//-------------------------------------------------------------------
// SIGNALS

#define BEEP_SYSTEM_STARTUP     beep(1000,1)

#define BEEP_EEPROM_ERROR       beep(100,2)
#define BEEP_TIME_ERROR         beep(100,3)
#define BEEP_PASS_ERROR         beep(100,4)

#define BEEP_LOW_BATTERY        beep(100,5)
#define BEEP_BATTERY_OK         beep(500,1)

#define BEEP_CARD_CHECK_ERROR   //beep(200,3)
#define BEEP_CARD_CHECK_OK      beep(500,1)

#define BEEP_CARD_MARK_WRITTEN  beep(500,1)
#define BEEP_CARD_MARK_OK       beep(250,2)
#define BEEP_CARD_MARK_ERROR

#define BEEP_CARD_CLEAR_OK      beep(500,1)
#define BEEP_CARD_CLEAR_ERROR

#define BEEP_MASTER_CARD_PASS_OK            beep(500,2)
#define BEEP_MASTER_CARD_PASS_ERROR

#define BEEP_MASTER_CARD_TIME_OK            beep(500,3)
#define BEEP_MASTER_CARD_TIME_ERROR

#define BEEP_MASTER_CARD_SLEEP_OK           beep(500,4)
#define BEEP_MASTER_CARD_SLEEP_ERROR

#define BEEP_MASTER_CARD_STATION_WRITTEN    beep(500,5)
#define BEEP_MASTER_CARD_STATION_OK         beep(500,1)
#define BEEP_MASTER_CARD_STATION_ERROR      beep(50,6)

#define BEEP_MASTER_CARD_DUMP_OK            beep(500,6)
#define BEEP_MASTER_CARD_DUMP_ERROR

#define BEEP_MASTER_CARD_GET_INFO_OK        beep(250,1)
#define BEEP_MASTER_CARD_GET_INFO_ERROR     beep(250,2)

#define BEEP_SERIAL_OK                      beep(250,1)
#define BEEP_SERIAL_ERROR                   beep(250,2)

//--------------------------------------------------------------------

// Poll time in active mode (milliseconds)
#define MODE_ACTIVE_CARD_CHECK_PERIOD     250
// Poll time in wait mode (milliseconds)
#define MODE_WAIT_CARD_CHECK_PERIOD       1000
// Poll time in sleep mode (milliseconds)
#define MODE_SLEEP_CARD_CHECK_PERIOD      25000

//--------------------------------------------------------------------
// VARIABLES  

// work time in milliseconds
uint32_t workTimer;
uint8_t stationNum;
uint8_t mode;

#define MODE_ACTIVE   0
#define MODE_WAIT     1
#define MODE_SLEEP    2

// It would be better to have MODE_WAIT as default
// If station resets on competition and default 
// mode is SLEEP in this case the participant can't
// do mark fast
#define DEFAULT_MODE MODE_WAIT

// date/time
ts t;
// We need this variable because DS321 doesn't have Year for Alarms
int16_t alarmYear;
// We need this variable because DS321 doesn't have Month for Alarms
uint8_t alarmMonth;
// To support wakeup on hw v1
uint32_t alarmTimestamp = 0;
// This flag is true when it's DS3231 interrupt
uint8_t rtcAlarmFlag;
// UART data buffer
#define SERIAL_DATA_LENGTH  32
byte serialData[SERIAL_DATA_LENGTH];
// Index of last received byte by UART
uint8_t serialRxPos;
// It's true if there are data from UART in sleep mode
uint8_t serialWakeupFlag = 0;

// UART incoming message: 0xEE, 0xEF, <func>, <func data>, CRC8 (XOR of <func> and <func data>), 0xFD, 0xDF
// UART outcoming message: 0xEE, 0xEF, <resp>, <resp_data>, CRC8 (XOR of <resp> and <resp_data>), 0xFD, 0xDF
// UART message length can't be more 32 bytes

#define SERIAL_MSG_START1           0xFE
#define SERIAL_MSG_START2           0xEF

#define SERIAL_MSG_END1             0xFD
#define SERIAL_MSG_END2             0xDF

#define SERIAL_FUNC_READ_INFO       0xF0
#define SERIAL_FUNC_WRITE_SETTINGS  0xF1

#define SERIAL_RESP_STATUS          0x1
#define SERIAL_RESP_INFO            0x2

#define SERIAL_OK                   0x0
#define SERIAL_ERROR_CRC            0x1
#define SERIAL_ERROR_FUNC           0x2
#define SERIAL_ERROR_SIZE           0x3
#define SERIAL_ERROR_PWD            0x4

//--------------------------------------------------------------------
// FUNCTIONS

void(*resetFunc)(void) = 0;
uint8_t getPinMode(uint8_t pin);
void sleep(uint16_t ms);
void setMode(uint8_t md);
void setStationNum(uint8_t num);
void setTime(int16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t mi, uint8_t sec);
void setWakeupTime(int16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t mi, uint8_t sec);
bool checkBattery(bool beepEnabled);
void processRfid();
void processTimeMasterCard(byte *data, byte dataSize);
void processStationMasterCard(byte *data, byte dataSize);
void processSleepMasterCard(byte *data, byte dataSize);
void processDumpMasterCard(byte *data, byte dataSize);
void processPassMasterCard(byte *data, byte dataSize);
void processGetInfoMasterCard(byte *data, byte dataSize);
void processParticipantCard(uint16_t cardNum);
// Finds new page to write a mark to a participant card. It uses the binary search algorithm
void findNewPage(uint8_t *newPage, uint8_t *lastNum);
void writeCardNumToLog(uint16_t num);
void clearMarkLog();
uint16_t getMarkLogEnd();
bool writeMarkToParticipantCard(uint8_t newPage);
void clearParticipantCard();
void checkParticipantCard();
bool doesCardExpire();
// DS3231 interrupt routine
void rtcAlarmIrq();
void wakeupByUartRx();
void serialFuncReadInfo(byte *data, byte dataSize);
void serialFuncWriteSettings(byte *data, byte dataSize);
void serialRespStatus(uint8_t code);
byte serialCrc(byte *data, uint8_t from, uint8_t to);

// Note: DS3231 works by UTC time!

void setup() {
    MCUSR &= ~(1 << WDRF);
    Watchdog.disable();
    Watchdog.reset();

    pinMode(LED,OUTPUT);
    pinMode(BUZ,OUTPUT);
    pinMode(RC522_RST,OUTPUT);
    pinMode(RC522_SS,OUTPUT);
    pinMode(RC522_IRQ,INPUT_PULLUP);
    pinMode(DS3231_IRQ,INPUT_PULLUP);
    pinMode(DS3231_32K,INPUT_PULLUP);
    pinMode(DS3231_VCC, OUTPUT);

#if HW_VERS > 1
    pinMode(DS3231_RST, INPUT); // if set as pull_up it takes additional supply current
#else
    // not connected in v1
    pinMode(DS3231_RST, INPUT_PULLUP);
#endif

    digitalWrite(LED,LOW);
    digitalWrite(BUZ,LOW);
    digitalWrite(RC522_RST,LOW);
    digitalWrite(DS3231_VCC,HIGH);
    delay(5);
    // initialize I2C
    Wire.begin();
    // Config DS3231
    // Reset all interrupts and disable 32 kHz output
    DS3231_set_addr(DS3231_STATUS_ADDR, 0);
    DS3231_init(DS3231_INTCN | DS3231_A1IE);
    alarmYear = 2017;
    alarmMonth = 1;
    memset(&t, 0, sizeof(t));
    // Check current time
    DS3231_get(&t);
    if(t.year < 2017) {
        BEEP_TIME_ERROR;
    }

    // Config DS3231 interrupts
    attachPCINT(digitalPinToPCINT(DS3231_IRQ), rtcAlarmIrq, FALLING);

    // Read settings from EEPROM
    stationNum = majEepromRead(EEPROM_STATION_NUM_ADDR);

    if(stationNum == 0) {
        setStationNum(DEFAULT_STATION_NUM);
    }

    if(!readPwdSettings()) {
        clearMarkLog();
        setStationNum(DEFAULT_STATION_NUM);
    }

    setMode(DEFAULT_MODE);
  
    // Config UART
    Serial.begin(SERIAL_BAUDRATE);
    serialRxPos = 0;

    delay(1000);
    
    BEEP_SYSTEM_STARTUP;

    workTimer = 0;
    Watchdog.enable(8000);
}

void loop() {
    Watchdog.reset();

    // process DS3231 alarm
    if(rtcAlarmFlag) {
        rtcAlarmFlag = 0;
        DS3231_clear_a1f();

        DS3231_get(&t);
        // DS3231 doesn't support year & month in alarm
        // so it's implemented on MCU side
        if(t.year == alarmYear && t.mon == alarmMonth) {
            setMode(MODE_ACTIVE);
        }
    }

    // automatic wake-up at competition start implementation for hw v1

#if HW_VERS == 1
    DS3231_get(&t);
    if(t.unixtime >= alarmTimestamp && (alarmTimestamp - t.unixtime) < 60) {
        setMode(MODE_ACTIVE);
    }
#endif

    // process cards
    processRfid();

    // process mode
    switch(mode) {
        case MODE_ACTIVE:
            sleep(MODE_ACTIVE_CARD_CHECK_PERIOD);

#ifdef DEBUG
            digitalWrite(LED,HIGH);
#endif

            if(getSettings() & SETTINGS_ALWAYS_ACTIVE) {
                  workTimer = 0;
            } else if(workTimer >= WAIT_PERIOD1) {
                setMode(MODE_WAIT);
            }
            break;
        case MODE_WAIT:
            sleep(MODE_WAIT_CARD_CHECK_PERIOD);

#ifdef DEBUG
            digitalWrite(LED,HIGH);
#endif

            if(getSettings() & SETTINGS_ALWAYS_WAIT) {
                workTimer = 0;
            } else if(workTimer >= WAIT_PERIOD1 && (getSettings() & SETTINGS_WAIT_PERIOD1)) {
                setMode(MODE_SLEEP);
            } else if(workTimer >= WAIT_PERIOD2 && (getSettings() & SETTINGS_WAIT_PERIOD2)) {
                setMode(MODE_SLEEP);
            }
            break;
        case MODE_SLEEP:
            sleep(MODE_SLEEP_CARD_CHECK_PERIOD);

            #ifdef DEBUG
            digitalWrite(LED,HIGH);
            #endif
            
            break;
    }

    // process serial
    if(Serial.available() > 0) {
        serialEvent();
    }
}

void serialEvent() {
    while(Serial.available()) {
        serialData[serialRxPos] = Serial.read();
        serialRxPos++;

        if(serialRxPos > 1 &&
           serialData[serialRxPos - 1] == SERIAL_MSG_START2 &&
           serialData[serialRxPos - 2] == SERIAL_MSG_START1) {
            serialData[0] = SERIAL_MSG_START1;
            serialData[1] = SERIAL_MSG_START2;
            serialRxPos = 2;
        }

        if(serialRxPos > 1 && 
           serialData[serialRxPos - 1] == SERIAL_MSG_END2 &&
           serialData[serialRxPos - 2] == SERIAL_MSG_END1) {
            byte func = serialData[2];
            byte crc8 = serialCrc(serialData, 2, serialRxPos - 3);

            if(crc8 == serialData[serialRxPos - 3]) {
                switch(func) {
                    case SERIAL_FUNC_READ_INFO:
                        serialFuncReadInfo(serialData, serialRxPos);
                        break;
                    case SERIAL_FUNC_WRITE_SETTINGS:
                        serialFuncWriteSettings(serialData, serialRxPos);
                        break;
                    default:
                        serialRespStatus(SERIAL_ERROR_FUNC);
                        break;
                }
            } else {
                serialRespStatus(SERIAL_ERROR_CRC);
            }

            serialRxPos = 0;      
        }

        if(serialRxPos >= SERIAL_DATA_LENGTH) {
            serialRxPos = 0;
            serialRespStatus(SERIAL_ERROR_SIZE);
        }
    }
}

void serialFuncReadInfo(byte *data, byte dataSize) {
    if(dataSize < 5) {
        serialRespStatus(SERIAL_ERROR_SIZE);
        return;
    }

    if(data[3] != getPwd(0) ||
       data[4] != getPwd(1) ||
       data[5] != getPwd(2) ) {
        serialRespStatus(SERIAL_ERROR_PWD);
        return;
    }

    uint8_t pos = 0;
    byte *sendData = &serialData[0];
    
    // Check battery
    bool batteryOk = checkBattery(false);
    // Get actual date and time
    DS3231_get(&t);
    // Write message start
    sendData[pos++] = SERIAL_MSG_START1;
    sendData[pos++] = SERIAL_MSG_START2;
    // Write func
    sendData[pos++] = SERIAL_RESP_INFO;
    // Write version
    sendData[pos++] = VERS;
    // Write info about station
    sendData[pos++] = stationNum;
    sendData[pos++] = getSettings();
    sendData[pos++] = batteryOk;
    sendData[pos++] = mode;
    // Write current time
    sendData[pos++] = (t.unixtime & 0xFF000000)>>24;
    sendData[pos++] = (t.unixtime & 0x00FF0000)>>16;
    sendData[pos++] = (t.unixtime & 0x0000FF00)>>8;
    sendData[pos++] = (t.unixtime & 0x000000FF);
    // Write wake-up time
    t.sec = bcdtodec(DS3231_get_addr(0x07));
    t.min = bcdtodec(DS3231_get_addr(0x08));
    t.hour = bcdtodec(DS3231_get_addr(0x09));
    t.mday = bcdtodec(DS3231_get_addr(0x0A));
    t.mon = alarmMonth;
    t.year = alarmYear;
    t.unixtime = get_unixtime(t);
    sendData[pos++] = (t.unixtime & 0xFF000000)>>24;
    sendData[pos++] = (t.unixtime & 0x00FF0000)>>16;
    sendData[pos++] = (t.unixtime & 0x0000FF00)>>8;
    sendData[pos++] = (t.unixtime & 0x000000FF);
    // Write antenna gain
    sendData[pos++] = getAntennaGain();
    // Write message end
    sendData[pos] = serialCrc(sendData, 2, pos);
    pos++;
    sendData[pos++] = SERIAL_MSG_END1;
    sendData[pos++] = SERIAL_MSG_END2;

    Serial.write(sendData, pos);
    
    BEEP_SERIAL_OK;
}

void serialFuncWriteSettings(byte *data, byte dataSize) {
    if(dataSize < 23) {
        serialRespStatus(SERIAL_ERROR_SIZE);
        return;
    }
    
    if(data[3] != getPwd(0) ||
       data[4] != getPwd(1) ||
       data[5] != getPwd(2) ) {
        serialRespStatus(SERIAL_ERROR_PWD);
        return;
    }

    setTime(data[6] + 2000, data[7], data[8], data[9], data[10], data[11]);
    setPwd(data[12], data[13], data[14]);  
    setStationNum(data[15]);
    setSettings(data[16]);
    setWakeupTime(data[17] + 2000, data[18], data[19], data[20], data[21], data[22]);
    setAntennaGain(data[23]);

    mode = MODE_SLEEP;

    serialRespStatus(SERIAL_OK);
}

void serialRespStatus(uint8_t code) {
    uint8_t pos = 0;
    byte *sendData = &serialData[0];
    
    // Write message start
    sendData[pos++] = SERIAL_MSG_START1;
    sendData[pos++] = SERIAL_MSG_START2;
    // Write func
    sendData[pos++] = SERIAL_RESP_STATUS;
    // Write version
    sendData[pos++] = code;
    // Write message end
    sendData[pos] = serialCrc(sendData, 2, pos);
    pos++;
    sendData[pos++] = SERIAL_MSG_END1;
    sendData[pos++] = SERIAL_MSG_END2;

    Serial.write(sendData, pos);

    if(code) {
        BEEP_SERIAL_ERROR;
    } else {
        BEEP_SERIAL_OK;
    }
}

byte serialCrc(byte *data, uint8_t from, uint8_t to) {
    byte crc8 = 0;
    
    for(uint8_t i = from; i < to; i++) {
        crc8 ^= data[i];
    }
        
    return crc8;
}

void wakeupByUartRx() {
    serialWakeupFlag = 1;
}

uint8_t getPinMode(uint8_t pin) {
    uint8_t bit = digitalPinToBitMask(pin);
    uint8_t port = digitalPinToPort(pin);

    // I don't see an option for mega to return this, but whatever...
    if(NOT_A_PIN == port) return UNKNOWN_PIN;

    // Is there a bit we can check?
    if(0 == bit) return UNKNOWN_PIN;

    // Is there only a single bit set?
    if(bit & (bit - 1)) return UNKNOWN_PIN;

    volatile uint8_t *reg, *out;
    reg = portModeRegister(port);
    out = portOutputRegister(port);

    if(*reg & bit) {
        return OUTPUT;
    } else if(*out & bit) {
        return INPUT_PULLUP;
    } else {
        return INPUT;
    }
}

void setStationNum(uint8_t num) {
    if(num == stationNum || num == 0) {
        return;
    }

    stationNum = num;
    majEepromWrite(EEPROM_STATION_NUM_ADDR, stationNum);
}

void setTime(int16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t mi, uint8_t sec) {
    memset(&t, 0, sizeof(t));

    t.mon = mon;
    t.year = year;
    t.mday = day;
    t.hour = hour;
    t.min = mi;
    t.sec = sec;

    DS3231_set(t);  
}

void setWakeupTime(int16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t mi, uint8_t sec) {
    uint8_t flags[5] = {0,0,0,0,0};
    memset(&t, 0, sizeof(t));
    alarmMonth = t.mon = mon;
    alarmYear = t.year = year;
    t.mday = day;
    t.hour = hour;
    t.min = mi;
    t.sec = sec;
    alarmTimestamp = get_unixtime(t);

    DS3231_clear_a1f();
    DS3231_set_a1(t.sec, t.min, t.hour, t.mday, flags);
}

void sleep(uint16_t ms) {
    // We can't sleep if there is data received by UART or DS3231 interrupt
    if(rtcAlarmFlag || Serial.available() > 0 || serialWakeupFlag) {
        return;
    }
    
    uint16_t period;
    
    // Resolve issue #61
    digitalWrite(RC522_RST,LOW);
    digitalWrite(LED,LOW);
    digitalWrite(BUZ,LOW);
    digitalWrite(DS3231_VCC,LOW);
    Wire.end();
    Serial.end();

    pinMode(SDA, INPUT);  // it is pulled up by hardware
    pinMode(SCL, INPUT);  // it is pulled up by hardware

    for(byte pin = 0; pin < A5; pin++) {
        if(pin == SDA ||
           pin == SCL ||
           pin == RC522_RST ||
           pin == LED ||
           pin == BUZ ||
           pin == DS3231_VCC ||
           pin == DS3231_RST ) {
            continue;
        }

        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
    // Turn off ADC
    ADCSRA = 0;
    // Attach PCINT to wake-up CPU when data will arrive by UART in sleep mode
    attachPCINT(digitalPinToPCINT(UART_RX), wakeupByUartRx, CHANGE);
    // Reset watchdog
    Watchdog.reset();
    period = Watchdog.sleep(ms);
    workTimer += period;
    // Use recursion if sleep time below need time
    if(ms > period) {
        sleep(ms - period);
    }
    // no need this interrupt anymore
    detachPCINT(digitalPinToPCINT(UART_RX));
    // Resolve issue #61
    digitalWrite(DS3231_VCC, HIGH);
    serialWakeupFlag = 0;
    serialRxPos = 0;
    Serial.begin(SERIAL_BAUDRATE);
    Wire.begin();
}

void setMode(uint8_t md) {
    mode = md;
    workTimer = 0;

    // Check mode with settings
    if(getSettings() & SETTINGS_ALWAYS_WAIT) {
        mode = MODE_WAIT;
    } else if(getSettings() & SETTINGS_ALWAYS_ACTIVE) {
        mode = MODE_ACTIVE;
    }
}

void writeCardNumToLog(uint16_t num) {
    if(num > MAX_CARD_NUM_TO_LOG) {
        // BEEP_EEPROM_ERROR;
        return;
    }
    
    uint16_t byteAdr = num/8;
    uint16_t bitAdr = num%8;
    uint8_t eepromByte = EEPROM.read(byteAdr);
    bitSet(eepromByte, bitAdr);
    EEPROM.write(byteAdr, eepromByte);
}

void clearMarkLog() {
    uint16_t endAdr = getMarkLogEnd();
    
    for (uint16_t a = 0; a <= endAdr; a++) {
        Watchdog.reset();
        EEPROM.write(a,0);
        delay(2);
    }
}

uint16_t getMarkLogEnd() {
    uint16_t endAdr = MAX_CARD_NUM_TO_LOG/ 8;
    
    if(endAdr > 1000) {
        endAdr = 1000;
    }

    return endAdr;
}

bool checkBattery(bool beepEnabled) {
    const uint32_t refConst = 1125300L; //voltage constanta
    uint32_t value = 0;
    uint32_t adcl, adch;
    bool result = false;

    Watchdog.reset();
    // Turn on ADC
    ADCSRA |=  bit(ADEN); 
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

    Watchdog.reset();
    // Turn on led to increase current
    digitalWrite(LED, HIGH);
    delay(5000);
    // Measure battery voltage
    for(uint8_t i = 0; i < 10; i++) {
        // Start to measure
        ADCSRA |= _BV(ADSC);
        while(bit_is_set(ADCSRA, ADSC));
        // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
        adcl = ADCL;
        adch = ADCH;

        adcl &= 0xFF;
        adch &= 0xFF;
        value += ((adch << 8) | adcl);
    }

    value = (refConst*10)/value;

    // Turn off ADC
    ADCSRA = 0;

    digitalWrite(LED, LOW);
    delay(250);
    
    Watchdog.reset();

    if(value < 3100) {
        result = false;
    } else {
        result = true;
    }
        
    if(beepEnabled) {
        if(result) {
            BEEP_BATTERY_OK;
        } else {
            BEEP_LOW_BATTERY;
        }
    }

    return result;
}

void processRfid() {
    bool result;
    byte pageData[4];
    byte masterCardData[16];

    rfidBegin(RC522_SS, RC522_RST);
    
    if(rfidIsNewCardDetected()) {
        memset(pageData, 0, sizeof(pageData));
        memset(masterCardData, 0, sizeof(masterCardData));
        
        if(rfidCardPageRead(CARD_PAGE_INIT, pageData)) {
            // Check the card role
            if(pageData[2] == 0xFF) {
                // This is a master card

                // Don't change mode if it's the get info card
                if(pageData[1] != MASTER_CARD_GET_INFO) {
                    setMode(MODE_ACTIVE);
                }
                    
                // Copy data
                memcpy(masterCardData, pageData, 4);
                // Read master card data
                result = true;
                    
                result &= rfidCardPageRead(CARD_PAGE_INIT_TIME, pageData);
                if(result) {
                    memcpy(masterCardData + 4, pageData, 4);
                }

                result &= rfidCardPageRead(CARD_PAGE_INFO1, pageData);
                if(result) {
                    memcpy(masterCardData + 8, pageData, 4);
                }

                result &= rfidCardPageRead(CARD_PAGE_INFO2, pageData);
                if(result) {
                    memcpy(masterCardData + 12, pageData, 4);
                }

                if(result) {
                    // Process master card
                    // Check password
                    if( (getPwd(0) == masterCardData[4]) &&
                            (getPwd(1) == masterCardData[5]) &&
                            (getPwd(2) == masterCardData[6]) ) {
                        switch(masterCardData[1]) {
                            case MASTER_CARD_SET_TIME:
                                processTimeMasterCard(masterCardData, sizeof(masterCardData));
                                break;
                            case MASTER_CARD_SET_NUMBER:
                                processStationMasterCard(masterCardData, sizeof(masterCardData));
                                break;
                            case MASTER_CARD_SLEEP:
                                processSleepMasterCard(masterCardData, sizeof(masterCardData));
                                break;
                            case MASTER_CARD_READ_DUMP:
                                processDumpMasterCard(masterCardData, sizeof(masterCardData));
                                break;
                            case MASTER_CARD_SET_PASS:
                                processPassMasterCard(masterCardData, sizeof(masterCardData));
                                break;
                            case MASTER_CARD_GET_INFO:
                                processGetInfoMasterCard(masterCardData, sizeof(masterCardData));
                        }
                    } else {
                        BEEP_PASS_ERROR;
                    }
                }
            } else {
                setMode(MODE_ACTIVE);
                // Process a participant card
                switch(stationNum) {
                    case CLEAR_STATION_NUM:
                        clearParticipantCard();
                        break;
                    case CHECK_STATION_NUM:
                        checkParticipantCard();
                        break;
                    default:
                        uint16_t cardNum = pageData[0];
                        cardNum <<= 8;
                        cardNum |= pageData[1];
                        processParticipantCard(cardNum);
                        break;
                }
            }
        } 
    } // End of rfidIsNewCardDetected

    rfidEnd();
}

void processTimeMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        BEEP_MASTER_CARD_TIME_ERROR;
        return;
    }

    // Note: time is UTC
    setTime(data[9] + 2000, data[8], data[10], data[12], data[13], data[14]);

    memset(&t, 0, sizeof(t));
    DS3231_get(&t);

    if(t.year < 2017) {
        BEEP_TIME_ERROR;
    } else {
        BEEP_MASTER_CARD_TIME_OK;
    }
}

void processStationMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        BEEP_MASTER_CARD_STATION_ERROR;
        return;
    }

    uint8_t newNum = data[8];

    if(newNum > 0) {
        if(stationNum != newNum) {
            setStationNum(newNum);
            BEEP_MASTER_CARD_STATION_WRITTEN;
        } else {
            BEEP_MASTER_CARD_STATION_OK;
        }
    } else {
        BEEP_MASTER_CARD_STATION_ERROR;
    }
}

void processSleepMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        BEEP_MASTER_CARD_SLEEP_ERROR;
        return;
    }

    // don't use setMode because it checks settings
    // in this case we can't sleep if always work is set
    mode = MODE_SLEEP;
    
    if(getSettings() & SETTINGS_CLEAR_ON_SLEEP) {
        setSettings(DEFAULT_SETTINGS);
    }

    // Config alarm
    setWakeupTime(data[9] + 2000, data[8], data[10], data[12], data[13], data[14]);
    
    clearMarkLog();

    BEEP_MASTER_CARD_SLEEP_OK;
}

void processDumpMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        BEEP_MASTER_CARD_DUMP_ERROR;
        return;
    }
    
    byte pageData[4] = {0,0,0,0};
    uint16_t eepromAdr = 0;
    uint8_t maxPage = rfidGetCardMaxPage();
    uint16_t eepromEnd = getMarkLogEnd();
    bool result = true;

    // Write station num
    pageData[0] = stationNum;
    result &= rfidCardPageWrite(CARD_PAGE_INIT, pageData);

    for(uint8_t page = CARD_PAGE_INIT_TIME; page <= maxPage; page++) {
        Watchdog.reset();
        delay(50);
        
        digitalWrite(LED,HIGH);
        
        for(uint8_t m = 0; m < 4; m++) {
            pageData[m] = EEPROM.read(eepromAdr);
            eepromAdr++;

            if(eepromAdr > eepromEnd) {
                break;
            }
        }

        result &= rfidCardPageWrite(page, pageData);
        
        digitalWrite(LED, LOW);

        if(eepromAdr > eepromEnd) {
            break;
        }
    }

    if(result) {
        BEEP_MASTER_CARD_DUMP_OK;
    } else {
        BEEP_MASTER_CARD_DUMP_ERROR;
    }
    
    return;
}

void processPassMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        BEEP_MASTER_CARD_PASS_ERROR;
        return;
    }

    setAntennaGain(data[7]);
    setPwd(data[8], data[9], data[10]);
    setSettings(data[11]);

    BEEP_MASTER_CARD_PASS_OK;
}

void processGetInfoMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        BEEP_MASTER_CARD_GET_INFO_ERROR;
        return;
    }  

    byte pageData[4] = {0,0,0,0};
    
    // Check battery
    bool batteryOk = checkBattery(false);
    // Get actual time and date
    DS3231_get(&t);
    // Write version & gain
    pageData[0] = VERS;
    pageData[3] = getAntennaGain();
    bool result = rfidCardPageWrite(CARD_PAGE_START, pageData);
    // Write info about station
    memset(pageData, 0, sizeof(pageData));
    pageData[0] = stationNum;
    pageData[1] = getSettings();
    pageData[2] = batteryOk;
    pageData[3] = mode;
    result &= rfidCardPageWrite(CARD_PAGE_START + 1, pageData);
    // Write current time and date
    memset(pageData, 0, sizeof(pageData));
    pageData[0] = (t.unixtime & 0xFF000000)>>24;
    pageData[1] = (t.unixtime & 0x00FF0000)>>16;
    pageData[2] = (t.unixtime & 0x0000FF00)>>8;
    pageData[3] = (t.unixtime & 0x000000FF);
    result &= rfidCardPageWrite(CARD_PAGE_START + 2, pageData);
    // Write wake-up time
    memset(&t, 0, sizeof(t));
    t.sec = bcdtodec(DS3231_get_addr(0x07));
    t.min = bcdtodec(DS3231_get_addr(0x08));
    t.hour = bcdtodec(DS3231_get_addr(0x09));
    t.mday = bcdtodec(DS3231_get_addr(0x0A));
    t.mon = alarmMonth;
    t.year = alarmYear;
    t.unixtime = get_unixtime(t);
    pageData[0] = (t.unixtime & 0xFF000000)>>24;
    pageData[1] = (t.unixtime & 0x00FF0000)>>16;
    pageData[2] = (t.unixtime & 0x0000FF00)>>8;
    pageData[3] = (t.unixtime & 0x000000FF);
    result &= rfidCardPageWrite(CARD_PAGE_START + 3, pageData);
    
    Watchdog.reset();
    delay(1000);

    if(result) {
        BEEP_MASTER_CARD_GET_INFO_OK;
    } else {
        BEEP_MASTER_CARD_GET_INFO_ERROR;
    }
}

void processParticipantCard(uint16_t cardNum) {
    uint8_t lastNum = 0;
    uint8_t newPage = 0;
    uint8_t maxPage = rfidGetCardMaxPage();
    byte pageData[4] = {0,0,0,0};
    bool checkOk = false;

    if(cardNum) {
        // Find the empty page to write new mark
        if(getSettings() & SETTINGS_FAST_MARK) {
            if(rfidCardPageRead(CARD_PAGE_LAST_RECORD_INFO, pageData)) {
                lastNum = pageData[0];
                newPage = pageData[1] + 1;

                if(newPage < CARD_PAGE_START || newPage > maxPage) {
                    newPage = CARD_PAGE_START;
                }
            }
        } else {
            findNewPage(&newPage, &lastNum);
        }
    
        if(newPage >= CARD_PAGE_START && newPage <= maxPage) {
            if(lastNum != stationNum) {
                // Check Start/Finish marks on a card
                checkOk = true;
                if(getSettings() & SETTINGS_CHECK_START_FINISH) {
                    if(newPage == CARD_PAGE_START && stationNum != START_STATION_NUM) {
                        checkOk = false;
                    } else if(stationNum == START_STATION_NUM && newPage != CARD_PAGE_START) {
                        checkOk = false;
                    } else if(lastNum == FINISH_STATION_NUM) {
                        checkOk = false;
                    }
                    else if(stationNum == FINISH_STATION_NUM && newPage == CARD_PAGE_START) {
                        checkOk = false;
                    }
                }

                if(getSettings() & SETTINGS_CHECK_CARD_TIME) {
                    checkOk = !doesCardExpire();
                }

                // Записываем отметку
                if(checkOk) {
                    if(writeMarkToParticipantCard(newPage)) {
                        writeCardNumToLog(cardNum);
                            
                        BEEP_CARD_MARK_WRITTEN;
                    }
                }
            }
            else {
                checkOk = true;
                BEEP_CARD_MARK_OK;
            }
        }
    }

    if(!checkOk) {
        BEEP_CARD_MARK_ERROR;
    }
}

void findNewPage(uint8_t *newPage, uint8_t *lastNum) {
    uint8_t startPage = CARD_PAGE_START;
    uint8_t endPage = rfidGetCardMaxPage();
    uint8_t page = startPage;
    byte pageData[4] = {0,0,0,0};
    byte num = 0;

    *newPage = 0;
    *lastNum = 0;

    while(startPage < endPage) {   
        page = (startPage + endPage)/2;

        if(!rfidCardPageRead(page, pageData)) {
            return;
        }

        num = pageData[0];
          
        if(num == 0) {
            endPage = page;
        }
        else {
            startPage = (startPage != page)? page : page + 1;
        }
    }

    if(num > 0) {
        page++;
    }

    *newPage = page;
    *lastNum = num;
}

bool writeMarkToParticipantCard(uint8_t newPage) {
    byte pageData[4] = {0,0,0,0};
    bool result = false;
    
    DS3231_get(&t);

    pageData[0] = stationNum;
    pageData[1] = (t.unixtime & 0x00FF0000)>>16;
    pageData[2] = (t.unixtime & 0x0000FF00)>>8;
    pageData[3] = (t.unixtime & 0x000000FF);
            
    result = rfidCardPageWrite(newPage, pageData);

    if((getSettings() & SETTINGS_FAST_MARK) && result) {
        pageData[0] = stationNum;
        pageData[1] = newPage;
        pageData[2] = 0;
        pageData[3] = 0;
        result &= rfidCardPageWrite(CARD_PAGE_LAST_RECORD_INFO, pageData);
    }

    return result;
}

void clearParticipantCard() {
    byte pageData[4] = {0,0,0,0};
    uint8_t maxPage = rfidGetCardMaxPage();
    bool result = true;

    for(uint8_t page = CARD_PAGE_INIT_TIME; page <= maxPage; page++) {
        Watchdog.reset();
        delay(50);
        
        digitalWrite(LED,HIGH);
        
        result &= rfidCardPageWrite(page, pageData);
        
        digitalWrite(LED,LOW);
    }

    if(result) {
        DS3231_get(&t);
        
        pageData[0] = (t.unixtime&0xFF000000)>>24;
        pageData[1] = (t.unixtime&0x00FF0000)>>16;
        pageData[2] = (t.unixtime&0x0000FF00)>>8;
        pageData[3] = (t.unixtime&0x000000FF);

        result &= rfidCardPageWrite(CARD_PAGE_INIT_TIME, pageData);
    }

    if(result) {
        BEEP_CARD_CLEAR_OK;
    } else {
        BEEP_CARD_CLEAR_ERROR;
    }
}

void checkParticipantCard() {
    byte pageData[4] = {0,0,0,0};
    uint16_t cardNum = 0;
    uint8_t newPage = 0;
    uint8_t lastNum = 0;
    bool result = false;
    
    if(rfidCardPageRead(CARD_PAGE_INIT, pageData)) {
        // Check card number
        cardNum = (((uint16_t)pageData[0])<<8) + pageData[1];
        if(cardNum > 0 && pageData[2] != 0xFF) {
            // It shouldn't be marks on a card
            findNewPage(&newPage, &lastNum);
            if(newPage == CARD_PAGE_START && lastNum == 0) {
                result = true;
                // Check card init time
                if(getSettings() & SETTINGS_CHECK_CARD_TIME) {
                    result = !doesCardExpire();
                }
            }
        }
    }

    if(result) {
        BEEP_CARD_CHECK_OK;
    } else {
        BEEP_CARD_CHECK_ERROR;
    }
}

bool doesCardExpire() {
    byte pageData[4] = {0,0,0,0};
    uint32_t cardTime = 0;
    bool result = true;
    
    if(rfidCardPageRead(CARD_PAGE_INIT_TIME, pageData)) {
        DS3231_get(&t);

        cardTime = (((uint32_t)pageData[0]) & 0xFF000000)<<24;
        cardTime |= (((uint32_t)pageData[1]) & 0x00FF0000)<<16;
        cardTime |= (((uint32_t)pageData[2]) & 0x0000FF00)<<8;
        cardTime |= (((uint32_t)pageData[3]) & 0x000000FF);

        if(t.unixtime - cardTime >= CARD_EXPIRE_TIME) {
            result = true;
        } else {
            result = false;
        }
    }

    return result;
}

void rtcAlarmIrq() {
    // We can't process interrupt here
    // because it hangs CPU by I2C operations
    // So set flag and process interrupt in main routine
    rtcAlarmFlag = 1;
}
