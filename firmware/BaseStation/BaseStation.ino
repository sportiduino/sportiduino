// To compile this project with Arduino IDE change sketchbook to <Project>/firmware

#include <EEPROM.h>
#include <Wire.h>
#include <ds3231.h>
#include <Adafruit_SleepyDog.h>
#include <PinChangeInterrupt.h>
#include <sportiduino.h>

// Remove a comment from a line below to compile in DEBUG mode
//#define DEBUG

#define HW_VERS         3
#define FW_MAJOR_VERS   7
#define FW_MINOR_VERS   99

//-------------------------------------------------------------------
// HARDWARE

#define BUZZER_FREQUENCY 0 // 0 for buzzer with generator
    
#define BUZ           3
#define LED           4

#define RC522_RST     9
#define RC522_SS      10

#define UART_RX       0
#define UART_TX       1

#define SDA           A4
#define SCL           A5

#if HW_VERS == 1
    #define DS3231_VCC     5
    #define DS3231_IRQ     A3
    #define DS3231_32K     A1 // not used, reserved for future
    #define DS3231_RST     A0

    #define RC522_IRQ      6
#elif HW_VERS == 2
    #define DS3231_VCC     8 // not used
    #define DS3231_IRQ     A3
    #define DS3231_32K     5 // not used, reserved for future
    #define DS3231_RST     2

    #define RC522_IRQ      6

    #define ADC_IN         A0
    #define ADC_ENABLE     A1
#else
    #define DS3231_VCC     A3
    #define DS3231_IRQ     A2
    #define DS3231_32K     5 // not used, reserved for future
    #define DS3231_RST     2

    #define RC522_IRQ      8

    #define ADC_IN         A0
    #define ADC_ENABLE     A1

    #define I2C_EEPROM_VCC 6
    #define REED_SWITCH    7
#endif

//-------------------------------------------------------------------

// Configuration
// Bits 0-2 - Active mode duration:
// Bit 3 - Check start/finish station marks on a participant card (0 - no, 1 - yes)
// Bit 4 - Check init time of a participant card (0 - no, 1 - yes)
// Bit 5 - Reserved
// Bit 6 - Fast mark mode (0 - no, 1 - yes)
// Bit 7 - Reserved
// Bits 8-10 - Antenna gain
// Bits 11-15 - Reserved

typedef struct __attribute__((packed)) {
    uint8_t stationNumber;
// activeModeDuration
//    (xxx) - 2^(bit2:bit0) hours in active mode (1 - 32 hours)
//    (110) - always be in active mode (check card in 0.25 second period)
//    (111) - always be in wait mode (check card in 1 second period)
    uint8_t activeModeDuration: 3;
    uint8_t checkStartFinish: 1;
    uint8_t checkCardInitTime: 1;
    uint8_t _reserved0: 1;
    uint8_t fastMark: 1;
    uint8_t _reserved1: 1;
    uint8_t antennaGain: 3;
    uint8_t _reserved2: 5;
    uint8_t password[3];
} Configuration;



#ifdef REED_SWITCH
    #define USE_REED_SWITCH
#endif

#ifdef I2C_EEPROM_VCC
    #define USE_I2C_EEPROM
    #define I2C_EEPROM_ADDRESS  0x50
#endif

#define UNKNOWN_PIN 0xFF

// 31 days = 2678400 (seconds)
#define CARD_EXPIRE_TIME 2678400L

#define EEPROM_CONFIG_ADDR  0x3EE

#define MAX_CARD_NUM_TO_LOG 4000

// Poll time in active mode (milliseconds)
#define MODE_ACTIVE_CARD_CHECK_PERIOD     250
// Poll time in wait mode (milliseconds)
#define MODE_WAIT_CARD_CHECK_PERIOD       1000
// Poll time in sleep mode (milliseconds)
#define MODE_SLEEP_CARD_CHECK_PERIOD      25000

#define MODE_ACTIVE   0
#define MODE_WAIT     1
#define MODE_SLEEP    2

// It would be better to have MODE_WAIT as default
// If station resets on competition and default 
// mode is SLEEP in this case the participant can't
// do mark fast
#define DEFAULT_MODE                    MODE_WAIT
#define DEFAULT_STATION_NUM             CHECK_STATION_NUM
#define DEFAULT_ACTIVE_MODE_DURATION    1 // 2 hours

#define SETTINGS_ALWAYS_ACTIVE          0x06
#define SETTINGS_ALWAYS_WAIT            0x07

#define SERIAL_MSG_START            0xFA

#define SERIAL_FUNC_READ_INFO       0xF0
#define SERIAL_FUNC_WRITE_SETTINGS  0xF1

#define SERIAL_RESP_STATUS          0x01
#define SERIAL_RESP_INFO            0x02

#define SERIAL_OK                   0x00
#define SERIAL_ERROR_CRC            0x01
#define SERIAL_ERROR_UNKNOWN_FUNC   0x02
#define SERIAL_ERROR_SIZE           0x03
#define SERIAL_ERROR_PWD            0x04

//--------------------------------------------------------------------
// VARIABLES  

// work time in milliseconds
uint32_t workTimer = 0;
Configuration config;
uint8_t mode = DEFAULT_MODE;
Rfid rfid;
SerialProtocol serialProto;
// date/time
ts t;
// We need this variable because DS321 doesn't have Year for Alarms
int16_t alarmYear = 2017;
// We need this variable because DS321 doesn't have Month for Alarms
uint8_t alarmMonth = 1;
// To support wakeup on hw v1
uint32_t alarmTimestamp = 0;
// This flag is true when it's DS3231 interrupt
uint8_t rtcAlarmFlag = 0;
uint8_t reedSwitchFlag = 0;
// It's true if there are data from UART in sleep mode
uint8_t serialWakeupFlag = 0;

//--------------------------------------------------------------------
// FUNCTIONS

// the third parameter should be the frequency of your buzzer if you solded the buzzer without a generator else 0
inline void beep(uint16_t ms, uint8_t n) { beep_w(LED, BUZ, BUZZER_FREQUENCY, ms, n); }

inline void beepSystemStartup()         { beep(1000, 1); }
inline void beepOk()                    { beep(500, 1); }

inline void beepTimeError()             { beep(100, 3); }
inline void beepPassError()             { beep(100, 4); }

inline void beepLowBattery()            { beep(100, 5); }
inline void beepBatteryOk()             { beepOk(); }

inline void beepCardCheckOk()           { beepOk(); }

inline void beepCardMarkWritten()       { beepOk(); }
inline void beepCardMarkOk()            { beep(250, 2); }

inline void beepCardClearOk()           { beepOk(); }

inline void beepMasterCardOk()          { beep(250, 1); }
inline void beepMasterCardError()       { beep(250, 2); }
inline void beepMasterCardReadError()   { beep(50, 4); }
inline void beepMasterCardTimeOk()      { beep(1000, 1); }
inline void beepMasterCardSleepOk()     { beep(500, 4); }

inline void beepSerialOk()              { beep(250, 1); }
inline void beepSerialError()           { beep(250, 2); }

#ifdef USE_REED_SWITCH
    inline void enableInterruptReedSwitch() { enablePCINT(digitalPinToPCINT(REED_SWITCH)); }
    inline void disableInterruptReedSwitch() { disablePCINT(digitalPinToPCINT(REED_SWITCH)); }
#else
    inline void enableInterruptReedSwitch() {}
    inline void disableInterruptReedSwitch() {}
#endif


void(*resetFunc)(void) = 0;
bool readConfig(Configuration *config);
bool writeConfig(Configuration *newConfig);
void reedSwitchIrq();
void rtcAlarmIrq();
bool doesCardExpire();
void checkParticipantCard();
void processSerial();
void serialFuncReadInfo(byte *data, byte dataSize);
void serialFuncWriteSettings(byte *data, byte dataSize);
void serialRespStatus(uint8_t code);
void wakeupByUartRx();
uint8_t getPinMode(uint8_t pin);
void setStationNum(uint8_t num);
void setAntennaGain(uint8_t gain);
void setTime(int16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t mi, uint8_t sec);
void setWakeupTime(int16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t mi, uint8_t sec);
void sleep(uint16_t ms);
void setMode(uint8_t md);
void writeCardNumToLog(uint16_t num);
void clearMarkLog();
uint16_t getMarkLogEnd();
void i2cEepromWritePunch(uint16_t cardNum);
uint32_t i2cEepromReadPunch(uint16_t cardNum);
void processRfid();
uint32_t measureBatteryVoltage();
uint8_t batteryVoltageToByte(uint32_t voltage);
void processCard();
void processMasterCard(uint8_t *pageInitData);
void processTimeMasterCard(byte *data, byte dataSize);
void processStationMasterCard(byte *data, byte dataSize);
void processSleepMasterCard(byte *data, byte dataSize);
void processDumpMasterCard(byte *data, byte dataSize);
void processDumpMasterCardWithTimestamps(byte *data, byte dataSize);
void processSettingsMasterCard(byte *data, byte dataSize);
void processGetInfoMasterCard(byte *data, byte dataSize);
void processParticipantCard(uint16_t cardNum);
void findNewPage(uint8_t *newPage, uint8_t *lastNum);
bool writeMarkToParticipantCard(uint8_t newPage);
void clearParticipantCard();
void checkParticipantCard();

// Note: DS3231 works by UTC time!

void setup() {
    MCUSR &= ~(1 << WDRF);
    Watchdog.disable();
    Watchdog.reset();

    pinMode(LED, OUTPUT);
    pinMode(BUZ, OUTPUT);
    pinMode(RC522_RST, OUTPUT);
    pinMode(RC522_SS, OUTPUT);
    pinMode(RC522_IRQ, INPUT_PULLUP);
    pinMode(DS3231_IRQ, INPUT_PULLUP);
    pinMode(DS3231_32K, INPUT_PULLUP);
    pinMode(REED_SWITCH, INPUT_PULLUP);
    pinMode(DS3231_VCC, OUTPUT);
#if defined(ADC_IN) && defined(ADC_ENABLE)
    pinMode(ADC_IN, INPUT);
    pinMode(ADC_ENABLE, INPUT);
#endif

#if HW_VERS > 1
    pinMode(DS3231_RST, INPUT); // if set as pull_up it takes additional supply current
#else
    // not connected in v1
    pinMode(DS3231_RST, INPUT_PULLUP);
#endif

    digitalWrite(LED, LOW);
    digitalWrite(BUZ, LOW);
    digitalWrite(RC522_RST, LOW);
    digitalWrite(DS3231_VCC, HIGH);
    delay(5);
    // initialize I2C
    Wire.begin();
    // Config DS3231
    // Reset all interrupts and disable 32 kHz output
    DS3231_set_addr(DS3231_STATUS_ADDR, 0);
    DS3231_init(DS3231_INTCN | DS3231_A1IE);
    memset(&t, 0, sizeof(t));
    // Check current time
    DS3231_get(&t);
    if(t.year < 2017) {
        beepTimeError();
    }

    // Config DS3231 interrupts
    attachPCINT(digitalPinToPCINT(DS3231_IRQ), rtcAlarmIrq, FALLING);
    
#ifdef USE_REED_SWITCH
    attachPCINT(digitalPinToPCINT(REED_SWITCH), reedSwitchIrq, FALLING);
    disablePCINT(digitalPinToPCINT(REED_SWITCH));
#endif

    // Read settings from EEPROM
    readConfig(&config);

    if(config.stationNumber == 0 || config.stationNumber == 0xff ||
       config.antennaGain > MAX_ANTENNA_GAIN || config.antennaGain < MIN_ANTENNA_GAIN) {
        memset(&config, 0, sizeof(Configuration));

        config.stationNumber = DEFAULT_STATION_NUM;
        config.antennaGain = DEFAULT_ANTENNA_GAIN;
        config.activeModeDuration = DEFAULT_ACTIVE_MODE_DURATION;
        config.password[0] = config.password[1] = config.password[2] = 0;
        writeConfig(&config);
        clearMarkLog();
    }

    setMode(DEFAULT_MODE);
  
    serialProto.init(SERIAL_MSG_START);
    rfid.init(RC522_SS, RC522_RST, config.antennaGain);

    delay(1000);
    
    beepSystemStartup();

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

#ifdef USE_REED_SWITCH
    if(mode == MODE_SLEEP) {
        if(reedSwitchFlag) {
            reedSwitchFlag = 0;
            processRfid();
        }
    } else {
        processRfid();
    }
#else
    // process cards
    processRfid();
#endif

    // process mode
    switch(mode) {
        case MODE_ACTIVE:
            sleep(MODE_ACTIVE_CARD_CHECK_PERIOD);

#ifdef DEBUG
            digitalWrite(LED,HIGH);
#endif

            if(config.activeModeDuration == SETTINGS_ALWAYS_ACTIVE) {
                  workTimer = 0;
            } else if(workTimer >= (1<<(uint32_t)config.activeModeDuration)*3600000UL) {
                setMode(MODE_WAIT);
            }
            break;
        case MODE_WAIT:
            sleep(MODE_WAIT_CARD_CHECK_PERIOD);

#ifdef DEBUG
            digitalWrite(LED,HIGH);
#endif

            if(config.activeModeDuration == SETTINGS_ALWAYS_WAIT) {
                workTimer = 0;
            }
            break;
        case MODE_SLEEP:
        default:
            enableInterruptReedSwitch();
            sleep(MODE_SLEEP_CARD_CHECK_PERIOD);
            disableInterruptReedSwitch();

#ifdef DEBUG
            digitalWrite(LED,HIGH);
#endif
            
            break;
    }

    processSerial();
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

void setNewConfig(Configuration *newConfig) {
    if(newConfig->stationNumber == 0) {
        newConfig->stationNumber = config.stationNumber;
    }
    if(newConfig->antennaGain > MAX_ANTENNA_GAIN || newConfig->antennaGain < MIN_ANTENNA_GAIN) {
        newConfig->antennaGain = DEFAULT_ANTENNA_GAIN;
    }

    memcpy(&config, newConfig, sizeof(Configuration));
    writeConfig(&config);
}

void setStationNum(uint8_t num) {
    if(num == 0) {
        return;
    }

    config.stationNumber = num;
}

void setAntennaGain(uint8_t gain) {
    if(gain > MAX_ANTENNA_GAIN || gain < MIN_ANTENNA_GAIN) {
        return;
    }

    config.antennaGain = gain;
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
    // We can't sleep if there is data received by UART or special interrupts
    if(reedSwitchFlag || rtcAlarmFlag || serialWakeupFlag || Serial.available() > 0) {
        return;
    }
    
    uint16_t period;
    
    // Resolve issue #61
    digitalWrite(RC522_RST,LOW);
    digitalWrite(LED,LOW);
    digitalWrite(BUZ,LOW);
    digitalWrite(DS3231_VCC,LOW);
    Wire.end();
    serialProto.end();

    pinMode(SDA, INPUT);  // it is pulled up by hardware
    pinMode(SCL, INPUT);  // it is pulled up by hardware

    for(byte pin = 0; pin < A5; pin++) {
        if(pin == SDA ||
           pin == SCL ||
           pin == RC522_RST ||
           pin == LED ||
           pin == BUZ ||
           pin == UART_TX ||
           pin == UART_RX ||
#if defined(ADC_IN) && defined(ADC_ENABLE)
           pin == ADC_IN ||
           pin == ADC_ENABLE ||
#endif
           pin == DS3231_VCC ||
           pin == DS3231_IRQ ||
           pin == DS3231_32K ||
           pin == DS3231_RST ||
           pin == REED_SWITCH) {
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
    serialProto.begin();
    Wire.begin();
}

void setMode(uint8_t md) {
    mode = md;
    workTimer = 0;

    // Check mode with settings
    if(config.activeModeDuration == SETTINGS_ALWAYS_WAIT) {
        mode = MODE_WAIT;
    } else if(config.activeModeDuration == SETTINGS_ALWAYS_ACTIVE) {
        mode = MODE_ACTIVE;
    }
}

void writeCardNumToLog(uint16_t num) {
    if(num > MAX_CARD_NUM_TO_LOG) {
        return;
    }
    
#ifdef USE_I2C_EEPROM
    i2cEepromWritePunch(num);
#else
    uint16_t byteAdr = num/8;
    uint16_t bitAdr = num%8;
    uint8_t eepromByte = EEPROM.read(byteAdr);
    bitSet(eepromByte, bitAdr);
    EEPROM.write(byteAdr, eepromByte);
#endif
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
    uint16_t endAdr = MAX_CARD_NUM_TO_LOG/8;
    
    if(endAdr > 1000) {
        endAdr = 1000;
    }

    return endAdr;
}

void i2cEepromWritePunch(uint16_t cardNum) {
    pinMode(I2C_EEPROM_VCC, OUTPUT);
    // Power on I2C EEPROM
    digitalWrite(I2C_EEPROM_VCC, HIGH);
    delay(1);

    DS3231_get(&t);
    uint32_t timestamp = t.unixtime;
    uint16_t recordAddress = (cardNum - 1)*4;
    for(uint8_t i = 0; i < 4; ++i) {
        Wire.beginTransmission(I2C_EEPROM_ADDRESS);
        Wire.write((recordAddress + i) >> 8);
        Wire.write((recordAddress + i) & 0xff);
        Wire.write((timestamp >> (8*i)) & 0xff); // little endian order
        Wire.endTransmission();
        delay(5);
    }

    digitalWrite(I2C_EEPROM_VCC, LOW);
}

uint32_t i2cEepromReadPunch(uint16_t cardNum) {
    //pinMode(I2C_EEPROM_VCC, OUTPUT);
    // Power on I2C EEPROM
    //digitalWrite(I2C_EEPROM_VCC, HIGH);
    //delay(1);

    uint16_t recordAddress = (cardNum - 1)*4;
    Wire.beginTransmission(I2C_EEPROM_ADDRESS);
    Wire.write(recordAddress >> 8);
    Wire.write(recordAddress & 0xff);
    Wire.endTransmission(false);
    Wire.requestFrom(I2C_EEPROM_ADDRESS, 4);

    //digitalWrite(I2C_EEPROM_VCC, LOW);

    uint32_t timestamp = 0;
    for(uint8_t i = 0; i < 4; ++i) {
        if(Wire.available()) {
            // Transform timestamp to big endian order
            timestamp |= ((uint32_t)Wire.read() & 0xff) << (8*i);
        }
    }

    return timestamp;
}

//void i2cEepromErase() {
//    pinMode(I2C_EEPROM_VCC, OUTPUT);
//    digitalWrite(I2C_EEPROM_VCC, HIGH);
//    delay(1);
//
//    const uint16_t lastAddress = MAX_CARD_NUM_TO_LOG*4/64;
//    for(uint16_t i = 0; i < lastAddress; ++i) {
//        digitalWrite(LED, HIGH);
//
//        Wire.beginTransmission(I2C_EEPROM_ADDRESS);
//        uint16_t pageAddress = i*64;
//        Wire.write(pageAddress >> 8);
//        Wire.write(pageAddress & 0xff);
//        const uint8_t pageSize = 64;
//        for(uint8_t j = 0; j < pageSize; ++j) {
//            Wire.write(0);
//            Watchdog.reset();
//        }
//        Wire.endTransmission();
//
//        digitalWrite(LED, LOW);
//
//        delay(5);
//    }
//
//    digitalWrite(I2C_EEPROM_VCC, LOW);
//}

void i2cEepromErase() {
    pinMode(I2C_EEPROM_VCC, OUTPUT);
    digitalWrite(I2C_EEPROM_VCC, HIGH);
    delay(1);

    const uint16_t lastAddress = MAX_CARD_NUM_TO_LOG*4;
    for(uint16_t i = 0; i < lastAddress; ++i) {
        digitalWrite(LED, HIGH);

        Watchdog.reset();

        Wire.beginTransmission(I2C_EEPROM_ADDRESS);
        Wire.write(i >> 8);
        Wire.write(i & 0xff);
        Wire.write(0);
        Wire.endTransmission();

        digitalWrite(LED, LOW);

        delay(5);
    }

    digitalWrite(I2C_EEPROM_VCC, LOW);
}

uint32_t measureBatteryVoltage() {
    analogReference(INTERNAL);
    pinMode(ADC_ENABLE, OUTPUT);
    digitalWrite(ADC_ENABLE, LOW);
    pinMode(ADC_IN, INPUT);
    ADCSRA |= bit(ADEN);

    // Turn on led to increase current
    digitalWrite(LED, HIGH);

    Watchdog.reset();
    delay(2000);

    // Drop first measure, it's wrong
    analogRead(ADC_IN);

    uint32_t value = 0;
    for(uint8_t i = 0; i < 10; ++i) {
        Watchdog.reset();
        value += analogRead(ADC_IN);
        delay(1);
    }
    value /= 10;

    digitalWrite(LED, LOW);
    pinMode(ADC_ENABLE, INPUT);
    const uint32_t R_HIGH = 270000; // Ohm
    const uint32_t R_LOW = 68000; // Ohm
    return value*1100/1023*(R_HIGH + R_LOW)/R_LOW;
}

uint8_t batteryVoltageToByte(uint32_t voltage) {
    const uint32_t maxVoltage = 0xff*20; // mV
    if(voltage > maxVoltage) {
        voltage = maxVoltage;
    }
    return voltage/20;
}

bool checkBattery(bool beepEnabled = false) {
    Watchdog.reset();
    // Turn on ADC
    ADCSRA |=  bit(ADEN);
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

    Watchdog.reset();
    // Turn on led to increase current
    digitalWrite(LED, HIGH);

    delay(5000);

    uint32_t value = 0;
    // Measure battery voltage
    for(uint8_t i = 0; i < 10; i++) {
        // Start to measure
        ADCSRA |= _BV(ADSC);
        while(bit_is_set(ADCSRA, ADSC));
        // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
        uint32_t adcl = ADCL;
        uint32_t adch = ADCH;

        adcl &= 0xFF;
        adch &= 0xFF;
        value += ((adch << 8) | adcl);
    }

    const uint32_t refConst = 1125300L; //voltage constanta
    value = (refConst*10)/value;

    // Turn off ADC
    ADCSRA = 0;

    digitalWrite(LED, LOW);
    delay(250);
    
    Watchdog.reset();

    if(value > 3100) {
        if(beepEnabled) {
            beepBatteryOk();
        }
        return true;
    }

    if(beepEnabled) {
        beepLowBattery();
    }
    return false;
}

void processRfid() {
    rfid.begin(config.antennaGain);
    processCard();
    rfid.end();
}
    
void processCard() {
    if(!rfid.isNewCardDetected()) {
        return;
    }
    byte pageData[4];
    memset(pageData, 0, sizeof(pageData));
    
    if(!rfid.cardPageRead(CARD_PAGE_INIT, pageData)) {
        return;
    }
    // Check the card role
    if(pageData[2] == 0xFF) {
        // This is a master card
        processMasterCard(pageData);
    } else {
        setMode(MODE_ACTIVE);
        // Process a participant card
        switch(config.stationNumber) {
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

void processMasterCard(uint8_t *pageInitData) {
    // Don't change mode if it's the get info card
    if(pageInitData[1] != MASTER_CARD_GET_INFO) {
        setMode(MODE_ACTIVE);
    }

    byte masterCardData[16];
    memset(masterCardData, 0, sizeof(masterCardData));
    memcpy(masterCardData, pageInitData, 4);

    byte pageData[4];
    for(uint8_t i = 1; i < 4; ++i) {
        if(!rfid.cardPageRead(CARD_PAGE_INIT + i, pageData)) {
            return;
        }
        memcpy(masterCardData + 4*i, pageData, 4);
    }

    // Check password
    if( (config.password[0] != masterCardData[4]) ||
            (config.password[1] != masterCardData[5]) ||
            (config.password[2] != masterCardData[6]) ) {
        beepPassError();
        return;
    }

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
#ifdef USE_I2C_EEPROM
            processDumpMasterCardWithTimestamps(masterCardData, sizeof(masterCardData));
#else
            processDumpMasterCard(masterCardData, sizeof(masterCardData));
#endif
            break;
        case MASTER_CARD_CONFIG:
            processSettingsMasterCard(masterCardData, sizeof(masterCardData));
            break;
        case MASTER_CARD_GET_INFO:
            processGetInfoMasterCard(masterCardData, sizeof(masterCardData));
            break;
        default:
            beepMasterCardReadError();
            break;
    }
}

void processTimeMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        return;
    }

    // Note: time is UTC
    setTime(data[9] + 2000, data[8], data[10], data[12], data[13], data[14]);

    if(t.year < 2017) {
        beepTimeError();
    } else {
        beepMasterCardTimeOk();
    }
}

void processStationMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        beepMasterCardError();
        return;
    }

    uint8_t newNum = data[8];

    if(newNum > 0) {
        if(config.stationNumber != newNum) {
            setStationNum(newNum);
            writeConfig(&config);
        }
        beepMasterCardOk();
    } else {
        beepMasterCardError();
    }
}

void processSleepMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        return;
    }

    // don't use setMode because it checks settings
    // in this case we can't sleep if always work is set
    mode = MODE_SLEEP;
    
    // Config alarm
    setWakeupTime(data[9] + 2000, data[8], data[10], data[12], data[13], data[14]);
    
#ifndef USE_I2C_EEPROM
    clearMarkLog();
#endif

    beepMasterCardSleepOk();
}

void processDumpMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        beepMasterCardError();
        return;
    }

    byte pageData[4] = {0,0,0,0};
    // Write station num
    pageData[0] = config.stationNumber;
    bool result = rfid.cardPageWrite(CARD_PAGE_INIT, pageData);

    uint16_t eepromAdr = 0;
    uint16_t eepromEnd = getMarkLogEnd();
    uint8_t maxPage = rfid.getCardMaxPage();
    for(uint8_t page = CARD_PAGE_INIT_TIME; page <= maxPage; page++) {
        Watchdog.reset();
        delay(50);
        
        digitalWrite(LED, HIGH);
        
        for(uint8_t m = 0; m < 4; m++) {
            pageData[m] = EEPROM.read(eepromAdr);
            eepromAdr++;

            if(eepromAdr > eepromEnd) {
                break;
            }
        }

        result &= rfid.cardPageWrite(page, pageData);

        digitalWrite(LED, LOW);

        if(!result) {
            break;
        }
        

        if(eepromAdr > eepromEnd) {
            break;
        }
    }

    if(result) {
        beepMasterCardOk();
    } else {
        beepMasterCardError();
    }
}

void processDumpMasterCardWithTimestamps(byte *data, byte dataSize) {
    if(dataSize < 16) {
        beepMasterCardError();
        return;
    }

    byte pageData[4];
    memcpy(pageData, data, 4);
    pageData[0] = config.stationNumber;
    pageData[3] = 1; // flag: have timestamps
    bool result = rfid.cardPageWrite(CARD_PAGE_INIT, pageData);

    DS3231_get(&t);
    uint32_t now = t.unixtime;
    uint32_t from = t.unixtime - 0xfffff; // about last 12 days

    uint8_t page = CARD_PAGE_INFO1;
    // Write initial timestamp (4 bytes) in first data page
    uint32ToByteArray(from, pageData);
    result &= rfid.cardPageWrite(page++, pageData);

    digitalWrite(LED, HIGH);

    uint8_t maxPage = rfid.getCardMaxPage();
    digitalWrite(I2C_EEPROM_VCC, HIGH);
    delay(5);
    for(uint16_t cardNum = 1; cardNum <= MAX_CARD_NUM_TO_LOG; ++cardNum) {
        Watchdog.reset();

        uint32_t timestamp = i2cEepromReadPunch(cardNum);
        if(timestamp < from || timestamp > now) {
            // No timestamp for cardNum
            continue;
        }

        if(cardNum > 4095) {
            break;
        }

        byte timeData[4];
        uint32ToByteArray(timestamp, timeData);

        // Pack card number in first 12 bits of page
        pageData[0] = cardNum >> 4;
        pageData[1] = ((cardNum << 4)&0xf0) | (timeData[1]&0x0f);
        pageData[2] = timeData[2];
        pageData[3] = timeData[3];
        result &= rfid.cardPageWrite(page++, pageData);

        if(page > maxPage) {
            break;
        }
    }

    result &= rfid.cardErase(page, maxPage);

    digitalWrite(I2C_EEPROM_VCC, LOW);

    digitalWrite(LED, LOW);

    delay(250);

    if(result) {
        beepMasterCardOk();
    } else {
        beepMasterCardError();
    }
}

void processSettingsMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        return;
    }

    setNewConfig((Configuration*)&data[8]);

    beepMasterCardOk();
}

void processGetInfoMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        beepMasterCardError();
        return;
    }  

#if defined(ADC_IN) && defined(ADC_ENABLE)
    // Disable RFID to prevent bad impact on measurements
    rfid.end();
    byte batteryByte = batteryVoltageToByte(measureBatteryVoltage());
    rfid.begin();
#else
    byte batteryByte = checkBattery();
#endif
    uint8_t page = CARD_PAGE_START;
    byte pageData[4] = {0,0,0,0};

    // Write version
    pageData[0] = HW_VERS;
    pageData[1] = FW_MAJOR_VERS;
    pageData[2] = FW_MINOR_VERS;
    pageData[3] = 0;
    bool result = rfid.cardPageWrite(page++, pageData);

    // Write station config
    result &= rfid.cardPageWrite(page++, (byte*)&config);

    // Write station state
    pageData[0] = batteryByte;
    pageData[1] = mode;
    pageData[2] = 0;
    pageData[3] = 0;
    result &= rfid.cardPageWrite(page++, pageData);

    // Get actual time and date
    DS3231_get(&t);
    // Write current time and date
    pageData[0] = t.unixtime >> 24;
    pageData[1] = t.unixtime >> 16;
    pageData[2] = t.unixtime >> 8;
    pageData[3] = t.unixtime & 0xFF;
    result &= rfid.cardPageWrite(page++, pageData);

    // Write wake-up time
    memset(&t, 0, sizeof(t));
    t.sec = bcdtodec(DS3231_get_addr(0x07));
    t.min = bcdtodec(DS3231_get_addr(0x08));
    t.hour = bcdtodec(DS3231_get_addr(0x09));
    t.mday = bcdtodec(DS3231_get_addr(0x0A));
    t.mon = alarmMonth;
    t.year = alarmYear;
    t.unixtime = get_unixtime(t);
    pageData[0] = t.unixtime >> 24;
    pageData[1] = t.unixtime >> 16;
    pageData[2] = t.unixtime >> 8;
    pageData[3] = t.unixtime & 0xFF;
    result &= rfid.cardPageWrite(page++, pageData);
    
    Watchdog.reset();
    delay(250);

    if(result) {
        beepMasterCardOk();
    } else {
        beepMasterCardError();
    }
}

void processParticipantCard(uint16_t cardNum) {
    uint8_t lastNum = 0;
    uint8_t newPage = 0;
    uint8_t maxPage = rfid.getCardMaxPage();
    byte pageData[4] = {0,0,0,0};
    bool checkOk = false;

    if(cardNum) {
        // Find the empty page to write new mark
        if(config.fastMark) {
            if(rfid.cardPageRead(CARD_PAGE_LAST_RECORD_INFO, pageData)) {
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
            if(lastNum != config.stationNumber) {
                // Check Start/Finish marks on a card
                checkOk = true;
                if(config.checkStartFinish) {
                    if(newPage == CARD_PAGE_START && config.stationNumber != START_STATION_NUM) {
                        checkOk = false;
                    } else if(config.stationNumber == START_STATION_NUM && newPage != CARD_PAGE_START) {
                        checkOk = false;
                    } else if(lastNum == FINISH_STATION_NUM) {
                        checkOk = false;
                    }
                    else if(config.stationNumber == FINISH_STATION_NUM && newPage == CARD_PAGE_START) {
                        checkOk = false;
                    }
                }

                if(config.checkCardInitTime) {
                    checkOk = !doesCardExpire();
                }

                // Записываем отметку
                if(checkOk) {
                    if(writeMarkToParticipantCard(newPage)) {
                        writeCardNumToLog(cardNum);
                            
                        beepCardMarkWritten();
                    }
                }
            }
            else {
                checkOk = true;
                beepCardMarkOk();
            }
        }
    }
}

void findNewPage(uint8_t *newPage, uint8_t *lastNum) {
    uint8_t startPage = CARD_PAGE_START;
    uint8_t endPage = rfid.getCardMaxPage();
    uint8_t page = startPage;
    byte pageData[4] = {0,0,0,0};
    byte num = 0;

    *newPage = 0;
    *lastNum = 0;

    while(startPage < endPage) {   
        page = (startPage + endPage)/2;

        if(!rfid.cardPageRead(page, pageData)) {
            return;
        }

        num = pageData[0];
          
        if(num == 0) {
            endPage = page;
        } else {
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
    
    DS3231_get(&t);

    pageData[0] = config.stationNumber;
    pageData[1] = (t.unixtime & 0x00FF0000)>>16;
    pageData[2] = (t.unixtime & 0x0000FF00)>>8;
    pageData[3] = (t.unixtime & 0x000000FF);
            
    bool result = rfid.cardPageWrite(newPage, pageData);

    if(config.fastMark && result) {
        pageData[0] = config.stationNumber;
        pageData[1] = newPage;
        pageData[2] = 0;
        pageData[3] = 0;
        result &= rfid.cardPageWrite(CARD_PAGE_LAST_RECORD_INFO, pageData);
    }

    return result;
}

void clearParticipantCard() {
    byte pageData[4] = {0,0,0,0};
    uint8_t maxPage = rfid.getCardMaxPage();
    bool result = true;

    for(uint8_t page = CARD_PAGE_INIT_TIME; page <= maxPage; page++) {
        Watchdog.reset();
        delay(50);
        
        digitalWrite(LED,HIGH);
        
        result &= rfid.cardPageWrite(page, pageData);
        
        digitalWrite(LED,LOW);

        if(!result) {
            break;
        }
    }

    if(result) {
        DS3231_get(&t);
        
        pageData[0] = (t.unixtime&0xFF000000)>>24;
        pageData[1] = (t.unixtime&0x00FF0000)>>16;
        pageData[2] = (t.unixtime&0x0000FF00)>>8;
        pageData[3] = (t.unixtime&0x000000FF);

        result &= rfid.cardPageWrite(CARD_PAGE_INIT_TIME, pageData);
    }

    if(result) {
        beepCardClearOk();
    }
}

void checkParticipantCard() {
    byte pageData[4] = {0,0,0,0};
    uint16_t cardNum = 0;
    uint8_t newPage = 0;
    uint8_t lastNum = 0;
    bool result = false;
    
    if(rfid.cardPageRead(CARD_PAGE_INIT, pageData)) {
        // Check card number
        cardNum = (((uint16_t)pageData[0])<<8) + pageData[1];
        if(cardNum > 0 && pageData[2] != 0xFF) {
            // It shouldn't be marks on a card
            findNewPage(&newPage, &lastNum);
            if(newPage == CARD_PAGE_START && lastNum == 0) {
                result = true;
                // Check card init time
                if(config.checkCardInitTime) {
                    result = !doesCardExpire();
                }
            }
        }
    }

    if(result) {
        beepCardCheckOk();
    }
}

bool doesCardExpire() {
    byte pageData[4] = {0,0,0,0};

    if(rfid.cardPageRead(CARD_PAGE_INIT_TIME, pageData)) {
        DS3231_get(&t);

        uint32_t cardTime = byteArrayToUint32(pageData);

        if(t.unixtime - cardTime < CARD_EXPIRE_TIME) {
            return false;
        }
    }

    return true;
}

void wakeupByUartRx() {
    serialWakeupFlag = 1;
}

void rtcAlarmIrq() {
    // We can't process interrupt here
    // because it hangs CPU by I2C operations
    // So set flag and process interrupt in main routine
    rtcAlarmFlag = 1;
}

void reedSwitchIrq() {
    reedSwitchFlag = 1;
}

bool readConfig(Configuration *config) {
    uint16_t eepromAdr = EEPROM_CONFIG_ADDR;
    for(uint8_t i = 0; i < sizeof(Configuration); ++i) {
        *((uint8_t*)config + i) = majEepromRead(eepromAdr);
        eepromAdr += 3;
    }

    return true;
}

bool writeConfig(Configuration *newConfig) {
    uint16_t eepromAdr = EEPROM_CONFIG_ADDR;
    for(uint8_t i = 0; i < sizeof(Configuration); ++i) {
        majEepromWrite(eepromAdr, *((uint8_t*)newConfig + i));
        eepromAdr += 3;
    }

    return true;
}

void processSerial() {
    bool error = false;
    uint8_t cmdCode = 0;
    uint8_t dataSize = 0;

    uint8_t *data = serialProto.read(&error, &cmdCode, &dataSize);
    if(error) {
        serialRespStatus(SERIAL_ERROR_CRC);
        return;
    }
    if(data) {
        switch(cmdCode) {
            case SERIAL_FUNC_READ_INFO:
                serialFuncReadInfo(data, dataSize);
                break;
            case SERIAL_FUNC_WRITE_SETTINGS:
                serialFuncWriteSettings(data, dataSize);
                break;
            default:
                serialRespStatus(SERIAL_ERROR_UNKNOWN_FUNC);
                break;
        }
    }
}

void serialFuncReadInfo(byte *data, byte dataSize) {
    if(dataSize < 3) {
        serialRespStatus(SERIAL_ERROR_SIZE);
        return;
    }

    if(data[0] != config.password[0] ||
       data[1] != config.password[1] ||
       data[2] != config.password[2] ) {
        serialRespStatus(SERIAL_ERROR_PWD);
        return;
    }

    serialProto.start(SERIAL_RESP_INFO);
    serialProto.add(HW_VERS);
    serialProto.add(FW_MAJOR_VERS);
    serialProto.add(FW_MINOR_VERS);
    serialProto.add((uint8_t*)&config, sizeof(Configuration));

#if defined(ADC_IN) && defined(ADC_ENABLE)
    serialProto.add(batteryVoltageToByte(measureBatteryVoltage()));
#else
    serialProto.add(checkBattery());
#endif
    serialProto.add(mode);
 
    // Get actual date and time
    DS3231_get(&t);

    // Write current time
    serialProto.add(t.unixtime >> 24);
    serialProto.add(t.unixtime >> 16);
    serialProto.add(t.unixtime >> 8);
    serialProto.add(t.unixtime & 0xFF);

    // Write wake-up time
    t.sec = bcdtodec(DS3231_get_addr(0x07));
    t.min = bcdtodec(DS3231_get_addr(0x08));
    t.hour = bcdtodec(DS3231_get_addr(0x09));
    t.mday = bcdtodec(DS3231_get_addr(0x0A));
    t.mon = alarmMonth;
    t.year = alarmYear;
    t.unixtime = get_unixtime(t);

    // Write current time
    serialProto.add(t.unixtime >> 24);
    serialProto.add(t.unixtime >> 16);
    serialProto.add(t.unixtime >> 8);
    serialProto.add(t.unixtime & 0xFF);

    serialProto.send();

    beepSerialOk();
}

void serialFuncWriteSettings(byte *data, byte dataSize) {
    if(dataSize < 22) {
        serialRespStatus(SERIAL_ERROR_SIZE);
        return;
    }
    
    if(data[0] != config.password[0] ||
       data[1] != config.password[1] ||
       data[2] != config.password[2] ) {
        serialRespStatus(SERIAL_ERROR_PWD);
        return;
    }

    setNewConfig((Configuration*)&data[3]);

    setTime(data[9] + 2000, data[10], data[11], data[12], data[13], data[14]);
    setWakeupTime(data[15] + 2000, data[16], data[17], data[18], data[19], data[20]);

    mode = data[21];

    serialRespStatus(SERIAL_OK);
}

void serialRespStatus(uint8_t code) {
    serialProto.start(SERIAL_RESP_STATUS);
    serialProto.add(code);
    serialProto.send();

    if(code) {
        beepSerialError();
    } else {
        beepSerialOk();
    }
}

