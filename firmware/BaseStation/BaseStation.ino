// To compile this project with Arduino IDE change sketchbook to <Project>/firmware

#include <Wire.h>
#include <ds3231.h>
#include <Adafruit_SleepyDog.h>
#include <PinChangeInterrupt.h>
#include <sportiduino.h>

// Uncomment line below to compile in DEBUG mode
//#define DEBUG
// You can also set debug mode by running "make debug=1 ..."

// Set PCB version by running "make pcbv=3 ..."
#ifndef HW_VERS
    // or change here
    #define HW_VERS     3
#endif
#define FW_MAJOR_VERS   10
// If FW_MINOR_VERS more than MAX_FW_MINOR_VERS this is beta version HW_VERS.FW_MAJOR_VERS.0-beta.X
// where X = (FW_MINOR_VERS - MAX_FW_MINOR_VERS)
#define FW_MINOR_VERS   0

// If PCB has reed switch and you don't want RC522 powered every 25 secs uncomment option bellow 
//#define NO_POLL_CARDS_IN_SLEEP_MODE
// You can also run "make nopoll=1 ..."

// Uncomment for BS check battery every 10 min in Sleep Mode and beep SOS if voltage < 3.5V
//#define CHECK_BATTERY_IN_SLEEP
// You can also run "make check_battery=1 ..."

//-------------------------------------------------------------------
// HARDWARE

// Set BUZZER_FREQUENCY by running "make buzzfreq=2500 ..."
#ifndef BUZZER_FREQUENCY
    // or change here
    #define BUZZER_FREQUENCY 4000 // or 0 for buzzer with generator
#endif

#define BUZ           3
#define LED           4

#define RC522_RST     9
#define RC522_SS      10

#define UART_RX       0
#define UART_TX       1

#define SDA           A4
#define SCL           A5

// If you added battery voltage measurement circuit, reed switch or I2C EEPROM to your PCB v1 or v2
// you only need define appropriate pins like for PCB v3

#if HW_VERS == 1
    #define DS3231_VCC     5
    #define DS3231_RST     A0
    #define RC522_IRQ      6

    //#define ADC_IN         pin_number
    //#define ADC_ENABLE     pin_number

    //#define I2C_EEPROM_VCC pin_number
    //#define REED_SWITCH    pin_number
#elif HW_VERS == 2
    #define DS3231_VCC     A1
    #define DS3231_IRQ     A3
    #define DS3231_32K     5 // not used, reserved for future
    #define DS3231_RST     2

    #define RC522_IRQ      6

    #define ADC_IN         A0
    #define ADC_ENABLE     A1

    //#define ADC_IN         pin_number
    //#define ADC_ENABLE     pin_number

    //#define I2C_EEPROM_VCC pin_number
    //#define REED_SWITCH    pin_number
#elif HW_VERS == 3
    #define DS3231_VCC     A3
    #define DS3231_IRQ     A2
    #define DS3231_32K     5 // not used, reserved for future
    #define DS3231_RST     2

    #define RC522_IRQ      8

    #define ADC_IN         A0
    #define ADC_ENABLE     A1

    #define I2C_EEPROM_VCC 6
    #define REED_SWITCH    7
#else
    #error Unknown HW_VERS
#endif

//-------------------------------------------------------------------

struct __attribute__((packed)) Configuration {
    uint8_t stationNumber;
// Active Mode duration
//    (xxx) - 2^(bit2:bit0) hours in Active Mode (1 - 32 hours)
//    (110) - always be in Active Mode (check card in 0.25 second period)
//    (111) - always be in Wait Mode (check card in 1 second period)
    uint8_t activeModeDuration: 3;
    uint8_t checkNoPunchesBeforeStart: 1; // Check no punches on a participant card at start station
    uint8_t checkCardInitTime: 1; // Check init time of a participant card
    uint8_t autosleep: 1; // Go to Sleep Mode after AUTOSLEEP_TIME milliseconds in Wait Mode
    uint8_t oldFastPunchMode: 1; // Deprecated
    uint8_t enableFastPunchForCard: 1; // Enable fast punch for card when clear
    uint8_t antennaGain: 3;
    uint8_t _reserved2: 5;
    uint8_t password[3];
};


#ifdef I2C_EEPROM_VCC
    #define USE_I2C_EEPROM
    #define I2C_EEPROM_ADDRESS  0x50
#endif

#define UNKNOWN_PIN 0xFF

// 31 days = 2678400 (seconds)
#define CARD_EXPIRE_TIME 2678400L

#define EEPROM_CONFIG_ADDR  0x3EE

#define LOG_RECORD_SIZE 8  // bytes
const uint16_t I2C_EEPROM_MEMORY_SIZE = (uint16_t)32*1024;  // bytes
const uint16_t MAX_LOG_RECORDS = I2C_EEPROM_MEMORY_SIZE/LOG_RECORD_SIZE;

// Poll time in active mode (milliseconds)
#define MODE_ACTIVE_CARD_CHECK_PERIOD     250
// Poll time in wait mode (milliseconds)
#define MODE_WAIT_CARD_CHECK_PERIOD       1000
// Poll time in sleep mode (milliseconds)
#define MODE_SLEEP_CARD_CHECK_PERIOD      25000

const uint32_t AUTOSLEEP_TIME = 48UL*3600*1000;

#define MODE_ACTIVE   0
#define MODE_WAIT     1
#define MODE_SLEEP    2

// It would be better to have MODE_WAIT as default
// If station resets on competition and default 
// mode is SLEEP in this case the participant can't
// do punch fast
#define DEFAULT_MODE                    MODE_WAIT
#define DEFAULT_STATION_NUM             CHECK_STATION_NUM
#define DEFAULT_ACTIVE_MODE_DURATION    1 // 2 hours

#define SETTINGS_ALWAYS_ACTIVE          0x06
#define SETTINGS_ALWAYS_WAIT            0x07

#define SERIAL_MSG_START            0xFA

#define SERIAL_FUNC_READ_INFO       0xF0
#define SERIAL_FUNC_WRITE_SETTINGS  0xF1
#define SERIAL_FUNC_ERASE_LOG       0xF2

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
uint16_t sleepCount = 0;
Configuration config;
uint8_t mode = DEFAULT_MODE;
uint16_t activeModePollPeriod = MODE_ACTIVE_CARD_CHECK_PERIOD;
Rfid rfid;
SerialProtocol serialProto;
// date/time
ts t;
// We need this variable because DS321 doesn't have Year for Alarms
int16_t alarmYear = 2021;
// We need this variable because DS321 doesn't have Month for Alarms
uint8_t alarmMonth = 1;
uint32_t alarmTimestamp = 0;
// This flag is true when it's DS3231 interrupt
uint8_t rtcAlarmFlag = 0;
uint8_t reedSwitchFlag = 0;
// It's true if there are data from UART in sleep mode
uint8_t serialWakeupFlag = 0;
uint16_t logNextRecordAddress = 0;

//--------------------------------------------------------------------
// FUNCTIONS

// the third parameter should be the frequency of your buzzer if you solded the buzzer without a generator else 0
inline void beep(uint16_t ms, uint8_t n) { beep_w(LED, BUZ, BUZZER_FREQUENCY, ms, n); }

inline void beepOk()                    { beep(500, 1); }

inline void beepTimeError()             { beep(100, 3); }
inline void beepPassError()             { beep(100, 4); }

inline void beepLowBattery()            { beep(100, 5); }
inline void beepBatteryOk()             { beep(1000, 1); }

inline void beepCardCheckOk()           { beepOk(); }

inline void beepCardPunchWritten()        { beepOk(); }
inline void beepCardPunchAlreadyWritten() { beep(250, 2); }

inline void beepCardClearOk()           { beepOk(); }

inline void beepMasterCardOk()          { beep(250, 1); }
inline void beepMasterCardError()       { beep(250, 2); }
inline void beepMasterCardReadError()   { beep(50, 4); }
inline void beepMasterCardTimeOk()      { beep(1000, 1); }
inline void beepMasterCardSleepOk()     { beep(500, 4); }

inline void beepSerialOk()              { beep(250, 1); }
inline void beepSerialError()           { beep(250, 2); }

inline void beepSos() {
    beep(100, 3);
    delay(200);
    beep(500, 3);
    delay(200);
    beep(100, 3);
    delay(200);
    digitalWrite(LED, HIGH);
    delay(3000);
    digitalWrite(LED, LOW);
}

#ifdef REED_SWITCH
    inline void enableInterruptReedSwitch() { enablePCINT(digitalPinToPCINT(REED_SWITCH)); }
    inline void disableInterruptReedSwitch() { disablePCINT(digitalPinToPCINT(REED_SWITCH)); }
#else
    inline void enableInterruptReedSwitch() {}
    inline void disableInterruptReedSwitch() {}
#endif


void(*resetFunc)(void) = 0;
void reedSwitchIrq();
void rtcAlarmIrq();
bool checkCardInitTime();
void checkParticipantCard();
void processSerial();
void serialFuncReadInfo(byte *data, byte dataSize);
void serialFuncWriteSettings(byte *data, byte dataSize);
void serialFuncEraseLog(byte *data, byte dataSize);
void serialRespStatus(uint8_t code);
void wakeupByUartRx();
void setStationNum(uint8_t num);
void setTime(int16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t mi, uint8_t sec);
void setWakeupTime(int16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t mi, uint8_t sec);
void sleep(uint16_t ms);
void setMode(uint8_t newMode);
void setModeIfAllowed(uint8_t newMode);
void clearPunchLog();
void i2cEepromInit();
uint16_t i2cEepromReadCardNumber(uint16_t address);
void i2cEepromWritePunch(uint16_t cardNum);
bool i2cEepromReadRecord(uint16_t address, uint16_t *cardNum, uint32_t *timestamp);
void i2cEepromErase();
void processRfid();
uint16_t measureBatteryVoltage(bool silent = false);
uint8_t batteryVoltageToByte(uint16_t voltage);
bool checkBattery(bool beepEnabled = false);
void processCard();
void processMasterCard(uint8_t pageInitData[]);
void processTimeMasterCard(byte *data, byte dataSize);
void processStationMasterCard(byte *data, byte dataSize);
void processSleepMasterCard(byte *data, byte dataSize);
void processBackupMasterCardWithTimestamps(byte *data, byte dataSize);
void processSettingsMasterCard(byte *data, byte dataSize);
void processPasswordMasterCard(byte *data, byte dataSize);
void processGetInfoMasterCard(byte *data, byte dataSize);
void processParticipantCard(uint16_t cardNum);
bool writePunchToParticipantCard(uint8_t newPage, bool fastPunch);
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
    pinMode(DS3231_VCC, OUTPUT);
#ifdef DS3231_IRQ
    pinMode(DS3231_IRQ, INPUT);
#endif
#ifdef DS3231_32K
    pinMode(DS3231_32K, INPUT_PULLUP);
#endif
#ifdef REED_SWITCH
    pinMode(REED_SWITCH, INPUT_PULLUP);
#endif
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

#ifdef DS3231_IRQ
    // Config DS3231 interrupts
    attachPCINT(digitalPinToPCINT(DS3231_IRQ), rtcAlarmIrq, FALLING);
#endif
    
#ifdef REED_SWITCH
    attachPCINT(digitalPinToPCINT(REED_SWITCH), reedSwitchIrq, FALLING);
    disablePCINT(digitalPinToPCINT(REED_SWITCH));
#endif

    // Read settings from EEPROM
    readConfig(&config, sizeof(Configuration), EEPROM_CONFIG_ADDR);

    if(config.stationNumber == 0 || config.stationNumber == 0xff ||
       config.antennaGain > MAX_ANTENNA_GAIN || config.antennaGain < MIN_ANTENNA_GAIN) {
        memset(&config, 0, sizeof(Configuration));

        config.stationNumber = DEFAULT_STATION_NUM;
        config.antennaGain = DEFAULT_ANTENNA_GAIN;
        config.activeModeDuration = DEFAULT_ACTIVE_MODE_DURATION;
        config.password[0] = config.password[1] = config.password[2] = 0;
        writeConfig(&config, sizeof(Configuration), EEPROM_CONFIG_ADDR);
#ifdef USE_I2C_EEPROM
        i2cEepromErase();
#endif
    }

    setModeIfAllowed(DEFAULT_MODE);
  
    serialProto.init(SERIAL_MSG_START);
    rfid.init(RC522_SS, RC522_RST, config.antennaGain);

    delay(500);

#ifdef USE_I2C_EEPROM
    i2cEepromInit();
#endif
    
    checkBattery(true);

    Watchdog.enable(8000);
}

void wakeupIfNeed() {
    if(alarmTimestamp > 0) {
        DS3231_get(&t);
        if(t.unixtime >= alarmTimestamp) {
            alarmTimestamp = 0;
            setModeIfAllowed(MODE_ACTIVE);
        }
    }
}

void loop() {
    Watchdog.reset();

    // process DS3231 alarm
    if(rtcAlarmFlag) {
        rtcAlarmFlag = 0;
        DS3231_clear_a1f();
        wakeupIfNeed();
    } else if(mode == MODE_SLEEP) {
        // if HV version is 1 or RTC alarm didn't occur
        wakeupIfNeed();
    }

    processRfid();

    // process mode
    switch(mode) {
        case MODE_ACTIVE:
            sleep(activeModePollPeriod);

#ifdef DEBUG
            digitalWrite(LED,HIGH);
#endif

            if(config.activeModeDuration == SETTINGS_ALWAYS_ACTIVE) {
                  workTimer = 0;
            } else if(workTimer >= (1<<(uint32_t)config.activeModeDuration)*3600000UL) {
                setModeIfAllowed(MODE_WAIT);
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

            if(config.autosleep && workTimer > AUTOSLEEP_TIME) {
                setModeIfAllowed(MODE_SLEEP);
            }
            break;

        case MODE_SLEEP:
        default:
#if defined(CHECK_BATTERY_IN_SLEEP) && defined(ADC_IN) && defined(ADC_ENABLE)
            if(sleepCount % 20 == 0) {
                uint16_t voltage = measureBatteryVoltage(true);
                if (voltage < 3500) {
                    beepSos();
                }
            }
            ++sleepCount;
#endif
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

void setNewConfig(Configuration *newConfig) {
    if(newConfig->stationNumber == 0) {
        newConfig->stationNumber = config.stationNumber;
    }
    if(newConfig->antennaGain > MAX_ANTENNA_GAIN || newConfig->antennaGain < MIN_ANTENNA_GAIN) {
        newConfig->antennaGain = config.antennaGain;
    }

    memcpy(&config, newConfig, sizeof(Configuration));
    writeConfig(&config, sizeof(Configuration), EEPROM_CONFIG_ADDR);
}

void setStationNum(uint8_t num) {
    if(num == 0) {
        return;
    }

    config.stationNumber = num;
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
    DS3231_get(&t);
    uint32_t currentTimestamp = t.unixtime;
    memset(&t, 0, sizeof(t));
    alarmMonth = t.mon = mon;
    alarmYear = t.year = year;
    t.mday = day;
    t.hour = hour;
    t.min = mi;
    t.sec = sec;
    alarmTimestamp = get_unixtime(t);
    if(alarmTimestamp < currentTimestamp) {
        alarmTimestamp = 0;
    }

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
#ifdef REED_SWITCH
           pin == REED_SWITCH ||
#endif
           pin == DS3231_VCC ||
#ifdef DS3231_IRQ
           pin == DS3231_IRQ ||
#endif
#ifdef DS3231_32K
           pin == DS3231_32K ||
#endif
           pin == DS3231_RST) {
            continue;
        }

        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
    // Turn off ADC
    ADCSRA = 0;
    pinMode(UART_RX, INPUT_PULLUP);
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

void setMode(uint8_t newMode) {
    if(newMode == MODE_SLEEP) {
        sleepCount = 0;
    } else {
        if(mode == MODE_SLEEP) {
            checkBattery(true);
        }
    }
    if(newMode != MODE_ACTIVE) {
        activeModePollPeriod = MODE_ACTIVE_CARD_CHECK_PERIOD;
    }
    mode = newMode;
    workTimer = 0;
}

void setModeIfAllowed(uint8_t newMode) {
    // Check mode with settings
    if(config.activeModeDuration == SETTINGS_ALWAYS_WAIT) {
        setMode(MODE_WAIT);
    } else if(config.activeModeDuration == SETTINGS_ALWAYS_ACTIVE) {
        setMode(MODE_ACTIVE);
    } else {
        setMode(newMode);
    }
}

#ifdef USE_I2C_EEPROM
void i2cEepromInit() {
    pinMode(I2C_EEPROM_VCC, OUTPUT);
    // Power on I2C EEPROM
    digitalWrite(I2C_EEPROM_VCC, HIGH);
    digitalWrite(LED, HIGH);
    delay(1);

    for(uint16_t i = 0; i < MAX_LOG_RECORDS; ++i) {
        uint16_t address = i*LOG_RECORD_SIZE;
        uint16_t cardNum = i2cEepromReadCardNumber(address);
        if(cardNum == 0) {
            logNextRecordAddress = address;
            break;
        }
    }

    digitalWrite(LED, LOW);
    digitalWrite(I2C_EEPROM_VCC, LOW);
    delay(100);
}

void i2cEepromEraseRecord(uint8_t address) {
    Wire.beginTransmission(I2C_EEPROM_ADDRESS);
    Wire.write(address >> 8);
    Wire.write(address & 0xff);
    Wire.write(0);
    Wire.write(0);
    Wire.endTransmission();
}

void i2cEepromWritePunch(uint16_t cardNum) {
    pinMode(I2C_EEPROM_VCC, OUTPUT);
    // Power on I2C EEPROM
    digitalWrite(I2C_EEPROM_VCC, HIGH);
    delay(1);

    DS3231_get(&t);
    uint32_t timestamp = t.unixtime;
    uint16_t recordAddress = logNextRecordAddress;
    if(recordAddress > I2C_EEPROM_MEMORY_SIZE - LOG_RECORD_SIZE) {
        recordAddress = 0;
    }
    Wire.beginTransmission(I2C_EEPROM_ADDRESS);
    Wire.write(recordAddress >> 8);
    Wire.write(recordAddress & 0xff);
    Wire.write(cardNum & 0xff);
    Wire.write(cardNum >> 8);
    for(uint8_t i = 0; i < 4; ++i) {
        Wire.write((timestamp >> (8*i)) & 0xff); // little endian order
    }
    Wire.endTransmission();
    logNextRecordAddress = recordAddress + LOG_RECORD_SIZE;
    delay(5);
    i2cEepromEraseRecord(logNextRecordAddress);

    digitalWrite(I2C_EEPROM_VCC, LOW);
}

bool i2cEepromReadRecord(uint16_t address, uint16_t *cardNum, uint32_t *timestamp) {
    Wire.beginTransmission(I2C_EEPROM_ADDRESS);
    Wire.write(address >> 8);
    Wire.write(address & 0xff);
    Wire.endTransmission(false);
    Wire.requestFrom(I2C_EEPROM_ADDRESS, 6);

    if(Wire.available() < 2) {
        return false;
    }
    *cardNum = Wire.read();
    *cardNum |= (uint16_t)Wire.read() << 8;
    *timestamp = 0;
    for(uint8_t i = 0; i < 4; ++i) {
        if(!Wire.available()) {
            return false;
        }
        // Transform timestamp to big endian order
        *timestamp |= ((uint32_t)Wire.read() & 0xff) << (8*i);
    }

    return true;
}

uint16_t i2cEepromReadCardNumber(uint16_t address) {
    Wire.beginTransmission(I2C_EEPROM_ADDRESS);
    Wire.write(address >> 8);
    Wire.write(address & 0xff);
    Wire.endTransmission(false);
    Wire.requestFrom(I2C_EEPROM_ADDRESS, 2);

    if(Wire.available() < 2) {
        return 0;
    }
    uint16_t cardNum = Wire.read();
    cardNum |= (uint16_t)Wire.read() << 8;
    return cardNum;
}

void i2cEepromErase() {
    pinMode(I2C_EEPROM_VCC, OUTPUT);
    digitalWrite(I2C_EEPROM_VCC, HIGH);
    digitalWrite(LED, HIGH);
    delay(1);

    const uint8_t pageSize = 32;
    const uint16_t nPages = I2C_EEPROM_MEMORY_SIZE/pageSize;
    for(uint16_t i = 0; i < nPages; ++i) {
        Watchdog.reset();

        if(i % 32 == 0) {
            digitalWrite(LED, HIGH);
        } else if(i % 16 == 0) {
            digitalWrite(LED, LOW);
        }

        Wire.beginTransmission(I2C_EEPROM_ADDRESS);
        uint16_t pageAddress = i*pageSize;
        Wire.write(pageAddress >> 8);
        Wire.write(pageAddress & 0xff);
        for(uint8_t j = 0; j < pageSize; ++j) {
            Wire.write(0);
            Watchdog.reset();
        }
        Wire.endTransmission();
        delay(5);
    }
    digitalWrite(LED, LOW);
    digitalWrite(I2C_EEPROM_VCC, LOW);
}
#endif

#if defined(ADC_IN) && defined(ADC_ENABLE)
uint16_t measureBatteryVoltage(bool silent) {
    analogReference(INTERNAL);
    pinMode(ADC_ENABLE, OUTPUT);
    digitalWrite(ADC_ENABLE, LOW);
    pinMode(ADC_IN, INPUT);
    ADCSRA |= bit(ADEN);

    if (silent) {
        delay(500);
    } else {
        // Turn on led to increase current
        digitalWrite(LED, HIGH);

        Watchdog.reset();
        delay(3000);
    }

    // Drop first measure, it's wrong
    analogRead(ADC_IN);

    uint32_t value = 0;
    for(uint8_t i = 0; i < 10; ++i) {
        Watchdog.reset();
        value += analogRead(ADC_IN);
        delay(1);
    }
    value /= 10;

    if (!silent) {
        digitalWrite(LED, LOW);
    }
    pinMode(ADC_ENABLE, INPUT);
    const uint32_t R_HIGH = 270000; // Ohm
    const uint32_t R_LOW = 68000; // Ohm
    const uint32_t k = 1100*(R_HIGH + R_LOW)/R_LOW;
    return value*k/1023;
}
#endif

uint16_t measureVcc() {
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
    
    return value;
}

uint8_t batteryVoltageToByte(uint16_t voltage) {
    const uint16_t maxVoltage = 0xff*20; // mV
    if(voltage > maxVoltage) {
        voltage = maxVoltage;
    }
    return voltage/20;
}

bool checkBattery(bool beepEnabled) {
#if defined(ADC_IN) && defined(ADC_ENABLE)
    uint16_t voltage = measureBatteryVoltage();
    const uint16_t minVoltage = 3600;
#else
    uint16_t voltage = measureVcc();
    const uint16_t minVoltage = 3100;
#endif

    Watchdog.reset();
    delay(250);

    if(voltage > minVoltage) {
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
#if defined(REED_SWITCH) && defined(NO_POLL_CARDS_IN_SLEEP_MODE)
    if(mode == MODE_SLEEP && !reedSwitchFlag) {
        return;
    }
#endif

    // Visual feedback to display rfid works via reed switch
    if(mode == MODE_SLEEP && reedSwitchFlag) {
        digitalWrite(LED, HIGH);
    }

    reedSwitchFlag = 0;

    rfid.begin(config.antennaGain);
    processCard();
    rfid.end();

    if(mode == MODE_SLEEP) {
        digitalWrite(LED, LOW);

        // Clear uid to process it more than once in sleep mode
        rfid.clearLastCardUid();
    }
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
    if(pageData[2] == MASTER_CARD_SIGN) {
        // This is a master card
        processMasterCard(pageData);
    } else {
        setModeIfAllowed(MODE_ACTIVE);
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

void processMasterCard(uint8_t pageInitData[]) {
    // Don't change mode if it's the get info card
    if(pageInitData[1] != MASTER_CARD_GET_INFO) {
        setModeIfAllowed(MODE_ACTIVE);
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
        case MASTER_CARD_READ_BACKUP:
#ifdef USE_I2C_EEPROM
            processBackupMasterCardWithTimestamps(masterCardData, sizeof(masterCardData));
#endif
            break;
        case MASTER_CARD_CONFIG:
            processSettingsMasterCard(masterCardData, sizeof(masterCardData));
            break;
        case MASTER_CARD_PASSWORD:
            processPasswordMasterCard(masterCardData, sizeof(masterCardData));
            break;
        case MASTER_CARD_GET_INFO:
            processGetInfoMasterCard(masterCardData, sizeof(masterCardData));
            break;
        default:
            beepMasterCardReadError();
            break;
    }
}

void deinitCard() {
    rfid.cardPageErase(CARD_PAGE_INIT);
}

void processTimeMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        return;
    }

    // Note: time is UTC
    setTime(data[9] + 2000, data[8], data[10], data[12], data[13], data[14]);

    deinitCard();

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
            writeConfig(&config, sizeof(Configuration), EEPROM_CONFIG_ADDR);
        }
        deinitCard();
        beepMasterCardOk();
    } else {
        beepMasterCardError();
    }
}

void processSleepMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        return;
    }

    setMode(MODE_SLEEP);

    // Config alarm
    setWakeupTime(data[9] + 2000, data[8], data[10], data[12], data[13], data[14]);

    beepMasterCardSleepOk();
}

#ifdef USE_I2C_EEPROM
void processBackupMasterCardWithTimestamps(byte *data, byte dataSize) {
    if(dataSize < 16) {
        beepMasterCardError();
        return;
    }

    byte pageData[4];
    memcpy(pageData, data, 4);
    pageData[0] = config.stationNumber;
    pageData[3] = FW_MAJOR_VERS;
    bool result = rfid.cardPageWrite(CARD_PAGE_INIT, pageData);

    uint8_t maxPage = rfid.getCardMaxPage();
    uint8_t stationNumberFromCard = data[0];
    uint16_t lastRecordAddressFromCard = 0xffff;
    if (stationNumberFromCard == config.stationNumber) {
        result &= rfid.cardPageRead(CARD_PAGE_INFO1, pageData);
        byte lastPageData[4];
        result &= rfid.cardPageRead(maxPage, lastPageData);
        if (pageData[0] == 0 && pageData[1] == 0 && !pageIsEmpty(lastPageData)) {
            lastRecordAddressFromCard = byteArrayToUint32(pageData) & 0xffff;
        }
    }

    uint8_t page = CARD_PAGE_INFO1;
    // Clear page for lastRecordAddressFromCard
    result &= rfid.cardPageErase(page++);
    uint16_t lastTimestampHiHalf = 0;

    // Power on I2C EEPROM
    digitalWrite(I2C_EEPROM_VCC, HIGH);
    delay(5);
    digitalWrite(LED, HIGH);
    uint16_t address = logNextRecordAddress;
    if(lastRecordAddressFromCard < I2C_EEPROM_MEMORY_SIZE) {
        address = lastRecordAddressFromCard;
    }
    for(uint16_t i = 1; i <= MAX_LOG_RECORDS; ++i) {
        Watchdog.reset();
        if(page > maxPage) {
            break;
        }

        if(i % 100 == 0) {
            digitalWrite(LED, LOW);
        } else if(i % 50 == 0) {
            digitalWrite(LED, HIGH);
        }

        if(address == 0) {
            address = I2C_EEPROM_MEMORY_SIZE;
        }
        address -= LOG_RECORD_SIZE;
        uint16_t cardNum = 0;
        uint32_t timestamp = 0;
        if(!i2cEepromReadRecord(address, &cardNum, &timestamp)) {
            beepMasterCardError();
            return;
        }
        if(cardNum == 0) {
            break;
        }
        if(cardNum == 0xffff && timestamp == 0xffffffff) {
            // No punch
            continue;
        }
        uint16_t timestampHiHalf = timestamp >> 16;
        if(timestampHiHalf != lastTimestampHiHalf) {
            pageData[0] = 0;
            pageData[1] = 0;
            pageData[2] = timestampHiHalf >> 8;
            pageData[3] = timestampHiHalf & 0xff;
            result &= rfid.cardPageWrite(page++, pageData);
            if(!result || page > maxPage) {
                break;
            }
            lastTimestampHiHalf = timestampHiHalf;
        }

        pageData[0] = cardNum >> 8;
        pageData[1] = cardNum & 0xff;
        pageData[2] = (timestamp >> 8) & 0xff;
        pageData[3] = timestamp & 0xff;
        result &= rfid.cardPageWrite(page++, pageData);
    }
    // Write current address for lastRecordAddressFromCard in first data page
    pageData[0] = 0;
    pageData[1] = 0;
    pageData[2] = address >> 8;
    pageData[3] = address & 0xff;
    result &= rfid.cardPageWrite(CARD_PAGE_INFO1, pageData);

    digitalWrite(LED, HIGH);
    result &= rfid.cardErase(page, maxPage);
    digitalWrite(LED, LOW);

    digitalWrite(I2C_EEPROM_VCC, LOW);

    delay(250);

    if(result) {
        beepMasterCardOk();
    } else {
        beepMasterCardError();
    }
}
#endif

void processSettingsMasterCard(byte *data, byte dataSize) {
    if(dataSize < 16) {
        return;
    }

    setNewConfig((Configuration*)&data[8]);

    beepMasterCardOk();
}

void processPasswordMasterCard(byte *data, byte dataSize) {
    config.password[0] = data[8];
    config.password[1] = data[9];
    config.password[2] = data[10];
    writeConfig(&config, sizeof(Configuration), EEPROM_CONFIG_ADDR);
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
    rfid.begin(config.antennaGain);
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
    if(cardNum == 0) {
        return;
    }

    uint8_t lastNum = 0;
    uint8_t newPage = 0;
    uint8_t maxPage = rfid.getCardMaxPage();
    bool fastPunch = false;

    // Find the empty page to write new punch
    byte pageData[4] = {0,0,0,0};
    if(rfid.cardPageRead(CARD_PAGE_LAST_RECORD_INFO, pageData)) {
        fastPunch = (pageData[3] == FAST_PUNCH_SIGN);
    } else {
        return;
    }
    if(fastPunch) {
        activeModePollPeriod = 100;
        lastNum = pageData[0];
        newPage = pageData[1] + 1;
        if(newPage < CARD_PAGE_START || newPage > maxPage) {
            newPage = CARD_PAGE_START;
        }
        if(rfid.cardPageRead(newPage, pageData)) {
            if (!pageIsEmpty(pageData)) {
                findNewPage(&rfid, &newPage, &lastNum);
            }
        } else {
            return;
        }
    } else {
        findNewPage(&rfid, &newPage, &lastNum);
    }

    if(newPage < CARD_PAGE_START || newPage > maxPage) {
        return;
    }

    if(lastNum != config.stationNumber) {
        if(config.checkNoPunchesBeforeStart
                && config.stationNumber == START_STATION_NUM
                && newPage > CARD_PAGE_START) {
            return;
        }

        if(config.checkCardInitTime && !checkCardInitTime()) {
            return;
        }

        if(writePunchToParticipantCard(newPage, fastPunch)) {
#ifdef USE_I2C_EEPROM
            i2cEepromWritePunch(cardNum);
#endif
            beepCardPunchWritten();
        }
    } else {
        beepCardPunchAlreadyWritten();
    }
}

bool writePunchToParticipantCard(uint8_t newPage, bool fastPunch) {
    byte pageData[4] = {0,0,0,0};
    
    DS3231_get(&t);

    pageData[0] = config.stationNumber;
    pageData[1] = (t.unixtime & 0x00FF0000)>>16;
    pageData[2] = (t.unixtime & 0x0000FF00)>>8;
    pageData[3] = (t.unixtime & 0x000000FF);
            
    bool result = rfid.cardPageWrite(newPage, pageData);

    if(fastPunch && result) {
        pageData[0] = config.stationNumber;
        pageData[1] = newPage;
        pageData[2] = 0;
        pageData[3] = FAST_PUNCH_SIGN;
        result &= rfid.cardPageWrite(CARD_PAGE_LAST_RECORD_INFO, pageData);
    }

    return result;
}

void clearParticipantCard() {
    uint8_t maxPage = rfid.getCardMaxPage();
    bool result = true;

    digitalWrite(LED, HIGH);
    // Clear card from last page
    for(uint8_t page = maxPage; page >= CARD_PAGE_INIT_TIME; --page) {
        Watchdog.reset();

        if(page % 10 == 0) {
            digitalWrite(LED, HIGH);
        } else if(page % 5 == 0) {
            digitalWrite(LED, LOW);
        }
        
        if(!rfid.cardPageErase(page)) {
            result = false;
            break;
        }
    }
    digitalWrite(LED, LOW);

    if(result) {
        DS3231_get(&t);
        
        byte pageData[4];
        pageData[0] = t.unixtime >> 24;
        pageData[1] = t.unixtime >> 16;
        pageData[2] = t.unixtime >> 8;
        pageData[3] = t.unixtime & 0xFF;

        result &= rfid.cardPageWrite(CARD_PAGE_INIT_TIME, pageData);

        if(config.enableFastPunchForCard) {
            pageData[0] = config.stationNumber;
            pageData[1] = 0;
            pageData[2] = 0;
            pageData[3] = FAST_PUNCH_SIGN;
            result &= rfid.cardPageWrite(CARD_PAGE_LAST_RECORD_INFO, pageData);
        }
    }

    if(result) {
        delay(50);
        beepCardClearOk();
    }
}

void checkParticipantCard() {
    byte pageData[4];
    if(!rfid.cardPageRead(CARD_PAGE_INIT, pageData)) {
        return;
    }
    uint16_t cardNum = (((uint16_t)pageData[0])<<8) + pageData[1];
    if(cardNum == 0 || pageData[2] == MASTER_CARD_SIGN) {
        return;
    }

    // It shouldn't be punches on a card
    uint8_t newPage = 0;
    uint8_t lastNum = 0;
    findNewPage(&rfid, &newPage, &lastNum);
    if(newPage != CARD_PAGE_START || lastNum != 0) {
        return;
    }
    if(!checkCardInitTime()) {
        return;
    }

#ifdef USE_I2C_EEPROM
    i2cEepromWritePunch(cardNum);
#endif

    beepCardCheckOk();
}

bool checkCardInitTime() {
    byte pageData[4] = {0,0,0,0};

    if(!rfid.cardPageRead(CARD_PAGE_INIT_TIME, pageData)) {
        return false;
    }

    uint32_t cardTime = byteArrayToUint32(pageData);

    if(cardTime < 1577826000) { // 01-01-2020
        return false;
    }

    if(config.checkCardInitTime) {
        DS3231_get(&t);
        if(t.unixtime - cardTime > CARD_EXPIRE_TIME) {
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
            case SERIAL_FUNC_ERASE_LOG:
                serialFuncEraseLog(data, dataSize);
                break;
            default:
                serialRespStatus(SERIAL_ERROR_UNKNOWN_FUNC);
                break;
        }
        return;
    } else {
        // drop bytes before start byte
        serialProto.dropByte();
    }
}

void serialFuncReadInfo(byte *data, byte dataSize) {
    if(dataSize < 3) {
        serialRespStatus(SERIAL_ERROR_SIZE);
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

    setNewConfig((Configuration*)&data[3]);

    setTime(data[9] + 2000, data[10], data[11], data[12], data[13], data[14]);
    setWakeupTime(data[15] + 2000, data[16], data[17], data[18], data[19], data[20]);

    setMode(data[21]);

    serialRespStatus(SERIAL_OK);
}

void serialFuncEraseLog(byte *data, byte dataSize) {
#ifdef USE_I2C_EEPROM
    i2cEepromErase();
#endif
    logNextRecordAddress = 0;
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

