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
### delete antena gain from setting-byte

## [1.4.1] - 2018-10-17
### mark beep change, reduce power for master station

## [1.4.0] - 2018-10-03
### Added the ability to reduce the power of the antenna to choose the optimal range of the station

## [1.3.8] - 2018-06-25
### Added the program for work with the system
- The program is available at (separete rep)[https://github.com/alexandervolikov/SportiduinoPQ]/ It is
based on [a python module](https://github.com/alexandervolikov/sportiduinoPython) and also on the PyQt package for creating window applications

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
- Resetting the station settings with enterening to sleep mode is configurable

## [1.3.4] - 2018-06-13
### Some changes
- reset stantion config with sleep entering
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
- Bug fix with set settings to statioin
- Add clear station mode
- Sound of the station is amplified by adding transistor to scheme
- Firmware for working with Mifare Classic S50 cards moved to the main repository

## [1.2.0] - 2018-02-17
### Changed the master station communication protocol
- The fixed packet up to 32 bytes is changed to a variable length packet with a maximum length of 32 bytes.
- Added functions to request the firmware version of the master station
- Added the function of transferring the station to automatic regime of cards reading

## [1.1.0] - 2018-02-13
### Delete regime stantion-page, bug fix
- To unify the work with the system, the mode in which the label was written to the page equal to the station number was deleted. A sequential mode is left with the search for the last blank page.
- Fixed the bug that occurs when initializing ntag 213 card
- Fixed the bug thst occurs with transmitting log from base stantion

## [1.0.0] - 2018-02-11
### First stable version
