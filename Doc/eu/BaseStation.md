### Scheme

The main components of the stations: microcontroller - Atmega328p-au, RFID module RC522, clock DS3231. Powered by 3 x AA batteries via the MCP1700T-33 linear regulator. Scheme and pcb can be viewed in [Upvertet](https://upverter.com/AlexanderVolikov/a6d775cd45a22968/Sportiduino-MarkStantion/)

![Scheme](https://raw.githubusercontent.com/alexandervolikov/sportIDuino/master/Base%20station/Scheme.jpg)

The components are mounted on a printed circuit board, with an RFID card, a soldered connection via a pin connector. You can order the manufacture of pcb in China, it will cost around 1 $ per piece with delivery, gerber files are in the folder base stantion.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Base%20station/PCB.JPG)

Interface implemented on the connector PBD-6, 6 contacts in two rows. As the box g1020BF used. In the box a hole is drilled for the LED and the excess plastic is cut off so that the battery compartment gets into it.

[About the assembly and the initial setting of the station written here](https://github.com/alexandervolikov/sportiduino/master/Doc/eu/BaseStationAssembly.md)

### Consumption

To reduce consumption while waiting for the competition, the Sleep mode is implemented for the controller as well as for the RC522, while consuming 0.02 mA (on the RFID card, the LED and pull-up resistor on the RST line must be removed / removed and the inductors replaced). During the operation of the sensor, up to 20 mA (up to 40 mA during the read-write) is consumed, one card polling cycle takes up to 20 ms.

By default, the station starts in standby mode. In this case, the cards are read once per second. Integral consumption up to 0.5 mA. Fully charged batteries last for 160 days. At the card punch (this is done by the installer or the first athlete), the station goes into the operating mode, by default, after 6 hours of inactivity, returns to the standby mode (the time can be changed using the byte settings see below).

In the operating mode, sleep occurs every 250 ms, thus, during operation, an integral consumption of about 1.7 mA can be expected. Fully charged batteries last for 45 days of continuous operation.

When the sleep card master punch (UID is set in the firmware of the station), the station goes into sleep mode (entering sleep for 24 seconds), picks 3 times and reloads. In the sleep mode, the station integrally consumes 0.02 mA. Fully charged batteries last for 5 years. When any card is enclosed, the station goes into standby mode, while the remaining battery charge is measured (3 signals - charge is less than 15%, 1 signal - charge is more than 15%), then a reboot

### Initial setting.

When using the firmware where watch-dog (avr / wdt.h) is used, it is necessary to re-boot the Bootloder on the microcontroller so that it works correctly. The boatloader and instructions are here: https://github.com/Optiboot/optiboot

Also for work it is necessary to install the libraries DS3231 and RC522, in the latter some parameters are changed (the time to exit the sleep mode is reduced from 50 ms to 1 ms). Another thing to do is change the wire library in the folder where the Arduino IDE is installed (so that the station continues to work when the i2c line breaks down, and does not go into an endless loop). files located in the Arduino_Scetches /! library folder

After soldering all the components, You need to upload the Bootloader, I performed it with the help of another Arduino through a special firmware Arduino as isp, connected the SPI pins through the pins leading to the RFID module

### Firmware (Stantion)

After the feed enters the standby mode: the station wakes up every second and searches for a card. When a card is found, it goes into working mode with a card polling every 0.25 seconds.

At the card punch, the station reads the first card block, where the card number or the master card mark is stored, in the case of which it is being processed. If the card is normal, then the last recorded page is searched and the station number is read at the last mark. If the station number is the same, the station simply picks up. If the number is different, then the mark is written to the next block after the recorded one. If successful, the station sends a signal, and also records the fact of the mark in the internal EEPROM memory of the station (in the bit corresponding to the card number)

During operation, watch-dog is started, which restarts when the system hangs.

The time correction at the stations is carried out using a special card. To do this, it is applied to the master stantion with the correct time and three seconds later to the base station. It is also possible to adjust the station number, similarly with a card first applied to the master stantion, in which the new number is entered via the Serial port, and then applying a mark to the station, is written to the EEPROM memory of the station. To download the data recorded on the EEPROM memory of the marked card numbers, use a dump-card

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/Stantion-blockscheme.png)

Byte settings. It is programmed using the master-card pass and settings. Responsible for the duration of the operating mode according to the table below and for the operation mode of the stations - with or without separate start and finish stations. In the switched-on mode, the start station (number = 240) will only accept cleaned cards, other stations will react to the card only with a mark at the start-up station, and after the finishing station (number = 245) the card will no longer be marked at other stations before cleaning. This will avoid annoying mistakes and accidents. In the future, perhaps, the functional will develop. Default setting byte = 0.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/Setting-byte2.PNG)

### Password system.
Because the system is open and all the specifications are available to everyone, someone can write with any NFC device any master card. There is the possibility of vandalism - reprogramming of established stations. And then, unlike if the station was simply broken down, the error might turn into results, which is difficult to notice and which can affect the places. To protect against this phenomenon, a password system was introduced.

The password consists of three numbers from 0 to 255 (three bytes). The default password is 0,0,0. A master configuration card is used to send the password to the base station. When you apply this card to the station marks, if the old password is the same, then a new password is recorded. All master-cards work only with the correct password in the first line. Also a byte of the settings is copied. The station picks twice and reboots. When the sleep card is presented (with the correct password), the station will reset the password and settings, leading the bytes to 0.0.0.0. Enters sleep mode and reboots. Information about passwords and station setup is stored in the EEPROM memory.