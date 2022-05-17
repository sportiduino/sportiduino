## v3.10.0 - 2022-05-17

Base station firmware **v3.10.0**:
- Added Password master card
- New format of punches log (store timestamp for cards numbers 1...65535, maximum 4000 records)
- Wake up from Sleep mode if RTC alarm doesn't work
- New fast punch mode
- Check battery every ~10 min in Sleep Mode (disabled by default) if voltage < 3.5V beep SOS

Master station firmware **v1.9.0**:
- Write Password master card
- Read Log master card of new format

Added 3D model of the custom box for Base station

For working with this firmware version use [SportiduinoPQ](https://github.com/sportiduino/sportiduinopq/releases/latest) >= 0.11, [SportiduinoApp](https://github.com/sportiduino/sportiduinoapp/releases/latest) >= 1.2 and [SportOrgPlus](https://github.com/sembruk/sportorg-plus/releases/latest).

---

Прошивка базовой станции **v3.10.0**:
- Добавлен мастер-чип пароля
- Новый формат лога отметок (записывается время для чипов от 1 до 65535, максимум 4000 записей)
- Реализован выход из спящего режима, даже если будильник RTC не сработает
- Новый режим быстрой отметки
- Проверка аккумулятора каждые ~10 минут в спящем режиме (по умолчанию отключено), если напряжение < 3,5 В, станция подаёт сигнал SOS

Прошивка станции сопряжения **v1.9.0**:
- Добавлена поддержка мастер-чипа пароля
- Добавлена поддержка мастер-чипа лога отметок нового формата

Добавлена 3D модель корпуса для базовой станции. 

Для работы с данной версией прошивок используйте [SportiduinoPQ](https://github.com/sportiduino/sportiduinopq/releases/latest) >= 0.11, [SportiduinoApp](https://github.com/sportiduino/sportiduinoapp/releases/latest) >= 1.2 и [SportOrgPlus](https://github.com/sembruk/sportorg-plus/releases/latest).

[All changes/Все изменения](https://github.com/sportiduino/sportiduino/compare/v3.9.0...v3.10.0)

## MS-v1.8.4

- Fix SPORTident emulation for card reading

## MS-v1.8.3

- Disable autosend all SI-Card 6 pages for MeOS

## MS-v1.8.2

- SPORTident legacy protocol emulation for WinOrient 2014

## MS-v1.8.1

- Fix beep pause for time card

## [3.9.0] - 2021-05-14
Base station PCB v3b (2021-01-01):
- Added TP4056 with leds for Li-ion battery charging
- RC522 mini or RC522 boards
- Change board length to 73 mm

Base station firmware:
- Autosleep option in Config (go to sleep after ~48h in Wait Mode)
- Docs update

## [3.8.0] - 2020-11-25
Master station firmware:
- Emulation of SPORTident serial protocol ([GH-90](https://github.com/sportiduino/sportiduino/issues/90))
- Change baudrate to 38400 (SportiduinoPQ v0.9 and SportOrg v1.5.0+ >= [bf6c804](https://github.com/sportorg/pysport/commit/bf6c804))

Base station firmware:
- Fix card clearing ([GH-99](https://github.com/sportiduino/sportiduino/issues/99)) and checking ([GH-96](https://github.com/sportiduino/sportiduino/issues/96))
- Check Station: write punch to EEPROM ([GH-95](https://github.com/sportiduino/sportiduino/issues/95))

## [3.7.0] - 2020-04-13
New base station PCB:
- Added battery voltage measurement circuit (GH-63)
- Added EEPROM IC for backuping punches (GH-69)
- Added reed switch for wake up (GH-75)
- New 10-pin slot for programming (GH-68)
- Fixed increased power consumption by DS3231 (GH-61)

Firmware bug fixes and improvements:
- Decreased power consumption (GH-61)
- Fix overwriting last mark in full card (GH-84)
- Signal battery state at wake-up (GH-81)
- Card clearing accelerated
- New 3-bytes version format
- Deinitialize time and station number master cards by BS
- Simplified signal system
- Don't check password at serial BS communication
- Added makefiles for building and firmware uploading
- Refactoring

## [2.6.3] - 2020-01-20
- Fix NTAG cards detection (GH-73)
- Fix increased power consumption in sleep mode for PCB v2 (GH-61)

## [2.6.2] - 2019-12-12
- Fix increased power consumption in sleep mode for PCB v1 (GH-61)
- Changed default antenna gain to 33 dB

## [2.6.1] - 2019-08-26
- Fixed working with SportOrg
- Changed algorithm to work with Serial in BaseStation
- Added wake-up function for BaseStation v1

## [2.6.0] - 2019-07-26
- Base station - DS3231-Pin3 (Interrupt) has been connected to MCU-Pin26 (PC3)
- Base station - DS3231-Pin4 (Reset) has been connected to MCU-Pin32 (PD2)
- Base station - DS3231-Pin2 (VCC) has been disconnected from MCU-Pin9 (PD5) and connected to +3V3 directly (as single-supply scheme from datasheet)
- Base station - DS3231-Pin1 (32 kHz) has been connected to MCU-Pin9 (PD5)
- Base station - RC522-IRQ has been connected to MCU-Pin10 (PD6)
- Base station - BC847 (bipolar) has been changed to BSS138 (mosfet)
- Base station - The value of I2C pull-up resistors has been reduced from 10 kOm to 3.3 kOm
- Base station - New pcb board design
- Firmware of a base station has been fully refactored
- Firmware of a master station has been fully refactored
- Added wake-up function
- Added an ability to config a base station by UART
- Added GetInfo master-chip
- Added fast-mark setting
- Added antenna-gain setting
- Added an ability to determine a participant card type. And now the system can work with various card types simalteniously
- Designed a box for a master station to print on 3d-printer

## [1.4.2] - 2018-10-23
Delete antenna gain from setting-byte

## [1.4.1] - 2018-10-17
Mark beep change, reduce power for master station

## [1.4.0] - 2018-10-03
Added the ability to reduce the power of the antenna to choose the optimal range of the station

## [1.3.8] - 2018-06-25
### Added the program for work with the system
- The program is available at [separete rep](https://github.com/sportiduino/SportiduinoPQ). It is
based on [a python module](https://github.com/sportiduino/sportiduinoPython) and also on the PyQt package for creating window applications

## [1.3.7] - 2018-06-25
### Fix dump uploading
- Fix dump uploading

## [1.3.6] - 2018-06-23
### Mifare Bug fixes
- Fix setting bag fix for Mifare Base Station

## [1.3.5] - 2018-06-19
### Bug fixes
- The Check station signals the master chips
- The Clear station can clear a repeatedly non-empty chip with the same number
- Resetting the station settings with entering to sleep mode is configurable

## [1.3.4] - 2018-06-13
### Some changes
- reset station config with sleep entering
- fix sounds
- remove auto reading mode at master station

## [1.3.3] - 2018-06-10
### Added the capacitor to the battery power of the clock
- In some parts of the station, a problem arose with the hours rushing for 2-5 minutes per day. It turned out that the problem consists in unstabilized power supply, the output capacitor after the stabilizer does not cope. We need to add one more closer to clock. In new gerber files that problem is fixed. In older boards, the bug is easily corrected by soldering the capacitor directly to the clock's power outputs.

## [1.3.2] - 2018-06-08
### Add check station
- To simplify the procedure for checking the chips before the start, a check station mode was added. Also Since it does not record anything on the chip, participants can be trained in the marking chip on the base station

## [1.3.1] - 2018-06-06
### Speeded up the mark, changes in the settings of the base stations
- Due to optimization of chip reading, the time of the mark is reduced by 40 ms
- Added the ability to limit the maximum possible number of marks on the chip and, thereby, further reduce the time of the mark at the base stations
- In the cleaning station mode, the problem of re-cleaning the chip is solved.

## [1.3.0] - 2018-04-11
### Bug fix and new features
- Bug fix with set settings to station
- Add clear station mode
- Sound of the station is amplified by adding transistor to scheme
- Firmware for working with Mifare Classic S50 cards moved to the main repository

## [1.2.0] - 2018-02-17
### Changed the master station communication protocol
- The fixed packet up to 32 bytes is changed to a variable length packet with a maximum length of 32 bytes.
- Added functions to request the firmware version of the master station
- Added the function of transferring the station to automatic regime of cards reading

## [1.1.0] - 2018-02-13
### Delete regime station-page, bug fix
- To unify the work with the system, the mode in which the label was written to the page equal to the station number was deleted. A sequential mode is left with the search for the last blank page.
- Fixed the bug that occurs when initializing ntag 213 card
- Fixed the bug that occurs with transmitting log from base station

## [1.0.0] - 2018-02-11
### First stable version
