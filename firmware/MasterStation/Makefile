# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile

ifndef ARDMK_DIR
   $(error You should define ARDMK_DIR as enviroment variable or command line argument. See https://github.com/sudar/Arduino-Makefile)
endif
ifndef ARDUINO_DIR
   $(error You should define ARDUINO_DIR as enviroment variable or command line argument. See https://github.com/sudar/Arduino-Makefile)
endif

ARDUINO_SKETCHBOOK  = ..
BOARD_TAG           = nano
BOARD_SUB           = atmega328noreset
ARDUINO_LIBS        = sportiduino-common SPI EEPROM RFID Adafruit_SleepyDog_Library
BOARDS_TXT          = ../boards.txt

BOOTLOADER_PARENT   = ..
BOOTLOADER_PATH     = Optiboot
BOOTLOADER_FILE     = optiboot8_atmega328_38400_without_reset.hex
ISP_PROG = stk500v1
AVRDUDE_ISP_BAUDRATE = 19200

ifdef port
	MONITOR_PORT = $(port)
	ISP_PORT = $(port)
endif

ifdef buzzfreq
	CXXFLAGS += -DBUZZER_FREQUENCY=$(buzzfreq)
endif

$(info CXXFLAGS = $(CXXFLAGS))

include $(ARDMK_DIR)/Arduino.mk

