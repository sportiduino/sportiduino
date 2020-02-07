# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile

ifndef ARDMK_DIR
   $(error You should define ARDMK_DIR as enviroment variable or command line argument. See https://github.com/sudar/Arduino-Makefile)
endif

BOARD_TAG           = sportiduino
ARDUINO_SKETCHBOOK  = ../
ARDUINO_LIBS        = sportiduino-common SPI EEPROM RFID Adafruit_SleepyDog_Library ds3231-master Wire PinChangeInterrupt
ifdef port
	MONITOR_PORT    = $(port)
endif

include $(ARDMK_DIR)/Arduino.mk

