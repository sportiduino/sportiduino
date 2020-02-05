#include <EEPROM.h>
#include <Adafruit_SleepyDog.h>
#include <string.h>
#include "sportiduino.h"

void majEepromWrite(uint16_t adr, uint8_t val) {
    uint8_t oldVal = majEepromRead(adr);
    if(val != oldVal) {
        for(uint16_t i = 0; i < 3; i++) {
            EEPROM.write(adr + i, val);
        }
    }
}

uint8_t majEepromRead(uint16_t adr) {
    uint8_t val1 = EEPROM.read(adr);
    uint8_t val2 = EEPROM.read(adr + 1);
    uint8_t val3 = EEPROM.read(adr + 2);
    
    if(val1 == val2 || val1 == val3) {
        return val1;
    } else if(val2 == val3) {
        return val2;
    }
    
    //BEEP_EEPROM_ERROR;
    return 0;
}

void beep_w(const uint8_t ledPin, const uint8_t buzPin, uint16_t freq, uint16_t ms, uint8_t n) {
    for(uint8_t i = 0; i < n; i++) {
        Watchdog.reset();
        
        digitalWrite(ledPin, HIGH);
        if(freq > 0) {
            tone(buzPin, freq, ms);
        } else {
            digitalWrite(buzPin, HIGH);
        }
        
        delay(ms);
        Watchdog.reset();
        
        digitalWrite(ledPin, LOW);
        digitalWrite(buzPin, LOW);
        
        if(i < n - 1) {
            delay(ms);
            Watchdog.reset();
        }
    }
}

bool uint32ToByteArray(uint32_t value, byte *byteArray) {
    for(uint8_t i = 0; i < 4; ++i) {
        byteArray[3 - i] = value & 0xff;
        value >>= 8;
    }
    return true;
}

uint32_t byteArrayToUint32(byte *byteArray) {
    uint32_t value = 0;
    for(uint8_t i = 0; i < 4; ++i) {
        value |= byteArray[i];
        value <<= 8;
    }

    return value;
}

