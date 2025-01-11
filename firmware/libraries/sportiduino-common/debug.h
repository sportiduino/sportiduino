#ifndef SPORTIDUINO_DEBUG_H
#define SPORTIDUINO_DEBUG_H

#ifdef DEBUG
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN_FORMAT(x, format) Serial.println(x, format)
    #define DEBUG_PRINT_FORMAT(x, format) Serial.print(x, format)
#else
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN_FORMAT(x, format)
    #define DEBUG_PRINT_FORMAT(x, format)
#endif

#endif

