# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile

ifndef ARDMK_DIR
   $(error You should define ARDMK_DIR as enviroment variable or command line argument. See https://github.com/sudar/Arduino-Makefile)
endif

BOARD_TAG 		  	= nano
BOARD_SUB 			= atmega328old
ARDUINO_SKETCHBOOK 	= ../
ARDUINO_LIBS 		= sportiduino-common SPI EEPROM RFID Adafruit_SleepyDog_Library
#MONITOR_PORT 		= /dev/ttyUSB1

include $(ARDMK_DIR)/Arduino.mk

