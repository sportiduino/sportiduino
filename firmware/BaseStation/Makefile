# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile

ifndef ARDMK_DIR
   $(error You should define ARDMK_DIR as enviroment variable or command line argument. See https://github.com/sudar/Arduino-Makefile)
endif
ifndef ARDUINO_DIR
   $(error You should define ARDUINO_DIR as enviroment variable or command line argument. See https://github.com/sudar/Arduino-Makefile)
endif

ARDUINO_SKETCHBOOK  = ..
BOARD_TAG           = sportiduino
ARDUINO_LIBS        = sportiduino-common SPI EEPROM RFID Adafruit_SleepyDog_Library ds3231-master Wire PinChangeInterrupt
BOARDS_TXT          = ../boards.txt

BOOTLOADER_PARENT   = ..
BOOTLOADER_PATH     = Optiboot
BOOTLOADER_FILE     = optiboot8_38400_sportiduino_led.hex
ISP_PROG = stk500v1
AVRDUDE_ISP_BAUDRATE = 19200

ifdef port
	MONITOR_PORT = $(port)
	ISP_PORT = $(port)
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

ifdef nopoll
	CXXFLAGS += -DNO_POLL_CARDS_IN_SLEEP_MODE
endif

ifdef check_battery
	CXXFLAGS += -DCHECK_BATTERY_IN_SLEEP
endif

$(info CXXFLAGS = $(CXXFLAGS))

include $(ARDMK_DIR)/Arduino.mk
