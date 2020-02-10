### Scheme

The main components of the stations: microcontroller - Atmega328p-au, RFID module RC522, clock DS3231. Powered by 3 x AA batteries via the MCP1700T-33 linear regulator. Scheme and pcb can be viewed in [Upvertet](https://upverter.com/AlexanderVolikov/a6d775cd45a22968/Sportiduino-MarkStantion/)

![](/hardware/BaseStation/prod/v3/sportiduino-base-v3-scheme.png?raw=true "Scheme")

The components are mounted on a printed circuit board, with an RFID card, a soldered connection via a pin connector. You can order the manufacture of pcb in China, it will cost around 1 $ per piece with delivery, gerber files are in the folder base station.

![](/hardware/BaseStation/prod/v3/sportiduino-base-v3-pcb.png?raw=true "PCB")

Interface implemented on the connector PBD-6, 6 contacts in two rows. As the box g1020BF used. In the box a hole is drilled for the LED and the excess plastic is cut off so that the battery compartment gets into it.

About the assembly and the initial setting of the station written in [the assembly manual](/Doc/en/BaseStationAssembly.md)

### Consumption

To reduce consumption while waiting for the competition, the Sleep mode is implemented for the controller as well as for the RC522, while consuming 0.02 mA (on the RFID card, the LED and pull-up resistor on the RST line must be removed / removed and the inductors replaced). During the operation of the sensor, up to 20 mA (up to 40 mA during the read-write) is consumed, one card polling cycle takes up to 20 ms.

By default, the station starts in standby mode. In this case, the cards are read once per second. Integral consumption up to 0.5 mA. Fully charged batteries last for 160 days. At the card punch (this is done by the installer or the first athlete), the station goes into the operating mode, by default, after 6 hours of inactivity, returns to the standby mode (the time can be changed using the byte settings see below).

In the operating mode, sleep occurs every 250 ms, thus, during operation, an integral consumption of about 1.7 mA can be expected. Fully charged batteries last for 45 days of continuous operation.

When the sleep card master punch (UID is set in the firmware of the station), the station goes into sleep mode (entering sleep for 24 seconds), picks 3 times and reloads. In the sleep mode, the station integrally consumes 0.02 mA. Fully charged batteries last for 5 years. When any card is enclosed, the station goes into standby mode, while the remaining battery charge is measured (3 signals - charge is less than 15%, 1 signal - charge is more than 15%), then a reboot

### Initial setting.

When using the firmware where watch-dog (avr / wdt.h) is used, it is necessary to re-boot the Bootloder on the microcontroller so that it works correctly. The boatloader and instructions are here: https://github.com/Optiboot/optiboot

Also for work it is necessary to install the libraries DS3231 and RC522, in the latter some parameters are changed (the time to exit the sleep mode is reduced from 50 ms to 1 ms). Another thing to do is change the wire library in the folder where the Arduino IDE is installed (so that the station continues to work when the i2c line breaks down, and does not go into an endless loop). files located in the Arduino_Scetches /! library folder

After soldering all the components, You need to upload the Bootloader, I performed it with the help of another Arduino through a special firmware Arduino as isp, connected the SPI pins through the pins leading to the RFID module

### Firmware (Station)

After the feed enters the standby mode: the station wakes up every second and searches for a card. When a card is found, it goes into working mode with a card polling every 0.25 seconds.

At the card punch, the station reads the first card block, where the card number or the master card mark is stored, in the case of which it is being processed. If the card is normal, then the last recorded page is searched and the station number is read at the last mark. If the station number is the same, the station simply picks up. If the number is different, then the mark is written to the next block after the recorded one. If successful, the station sends a signal, and also records the fact of the mark in the internal EEPROM memory of the station (in the bit corresponding to the card number)

During operation, watch-dog is started, which restarts when the system hangs.

The time correction at the stations is carried out using a special card. To do this, it is applied to the master station with the correct time and three seconds later to the base station. It is also possible to adjust the station number, similarly with a card first applied to the master station, in which the new number is entered via the Serial port, and then applying a mark to the station, is written to the EEPROM memory of the station. To download the data recorded on the EEPROM memory of the marked card numbers, use a dump-card

![](/Images/Stantion-blockscheme.png)

Byte settings. Programmed using the master chip settings. Responsible for the duration of the operating mode according to the table below, for the operation mode of the stations - with or without separate start and finish stations, checking for overdue chips, tuning the antenna power and saving or deleting settings after a reboot. When the start and finish stations are on (which is necessary when working with the SportOrg program), the start station (number = 240) will receive only cleared chips, other stations will only respond to the chip with a mark at the starting station, and after the finish station (number = 245 ) the chip will no longer be noted at other stations before cleaning. This will avoid annoying mistakes and accidents. By default, the settings byte is 0. It also sets up saving or resetting the settings when a station is getting out of sleep.

![](/Images/Setting-byte2.PNG)

### Clear station.

To transfer the station to the cleaning mode, you need to specify stations number 249.
In cleanup mode, the station clears all pages of the chip except of the page with the chip number and pages of the specified information. That is, all the marks are erased, the given chip number remains the same. The new time is recorded in the initialization time page, which is used to calculate the results, so it is important that the time at the station is correct.

When the chip is presented, the LED lights up on the station and lights up all the cleaning time (about 5 seconds). If the cleaning is successful, the station emits a signal, the LED goes out, the chip needs to be removed. If there was no signal, the cleaning was not completed, you need to repeat the procedure.

### The Check station.

To transfer the station to the check mode, you need to specify stations number 248.
In the verification mode, the station does not write anything to the chip but only checks it. If the chip is not empty or the time of initialization exceeds the time at the station for more than a month, the station emits three short signals. If everything is ok, then one short signal.
The test time is approximately the same as the time of the mark, so participants can practice the mark at this station.

### Password system.
Because the system is open and all the specifications are available to everyone, someone can write with any NFC device any master card. There is the possibility of vandalism - reprogramming of established stations. And then, unlike if the station was simply broken down, the error might turn into results, which is difficult to notice and which can affect the places. To protect against this phenomenon, a password system was introduced.

The password consists of three numbers from 0 to 255 (three bytes). The default password is 0,0,0. A master configuration card is used to send the password to the base station. When you apply this card to the station marks, if the old password is the same, then a new password is recorded. All master-cards work only with the correct password in the first line. Also a byte of the settings is copied. The station picks twice and reboots. When the sleep card is presented (with the correct password), the station will reset the password and settings, leading the bytes to 0.0.0.0. Enters sleep mode and reboots. Information about passwords and station setup is stored in the EEPROM memory.

### Signals

(the duration of milliseconds, the number of repetitions)

Errors:

50, 2 - an error when reading its memory;
50, 3 - the clock is not right;
50, 4 - the master chip password is not suitable;
50, 5 - batteries should be replaced;
50, 6 - attempt to add stations number 0;
200, 3 - the chip is incorrect when verified;

Normal behavior:

1000, 1 - signal when the station is loaded;
500, 1 - the voltage is normal;
500, 1 - repetition of the applied chip;
200, 1 - the chip is marked on the chip;
200, 1 - the chip is cleaned at the cleaning station;
200, 1 - the chip is normal when tested;

Reading master-chips

500, 2 - read the master password chip;
500, 3 - the station read the master time chip;
500, 4 - the master of the dream chip is read;
500, 5 - the station read the master-chip numbers;
500, 6 - read the master chip dump;
