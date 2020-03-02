# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile

ifndef ARDMK_DIR
   $(error You should define ARDMK_DIR as enviroment variable or command line argument. See https://github.com/sudar/Arduino-Makefile)
endif
ifndef ARDUINO_DIR
   $(error You should define ARDUINO_DIR as enviroment variable or command line argument. See https://github.com/sudar/Arduino-Makefile)
endif

BOARD_TAG           = sportiduino
ARDUINO_SKETCHBOOK  = ../
ARDUINO_LIBS        = sportiduino-common SPI EEPROM RFID Adafruit_SleepyDog_Library ds3231-master Wire PinChangeInterrupt

ifdef port
	MONITOR_PORT    = $(port)
endif

ifdef debug
	CXXFLAGS += -DDEBUG
endif

ifdef buzzfreq
	CXXFLAGS += -DBUZZER_FREQUENCY=$(buzzfreq)
endif

ifdef pcbv
	CXXFLAGS += -DHW_VERS=$(pcbv)
endif

$(info CXXFLAGS = $(CXXFLAGS))

include $(ARDMK_DIR)/Arduino.mk
