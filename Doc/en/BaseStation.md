### Scheme

The main components of the station: microcontroller - Atmega328p-au, RFID module RC522, clock DS3231. Powered by 3 x AA batteries via the MCP1700T-33 linear regulator. Scheme and pcb drawing are available at [Upvertet](https://upverter.com/AlexanderVolikov/a6d775cd45a22968/Sportiduino-MarkStantion/)
One can view them in KiCad program.

![](/hardware/BaseStation/prod/v2/sportiduino-base-v2-scheme.jpg?raw=true "Scheme")

The components are mounted on a printed circuit board. Connection with an RFID card is via soldered connection with a pin connector. You can order the manufacture of pcb in China, it will cost around 0.9-1 $ per piece with delivery. Gerber files are in the folder 'base station'.

![](/hardware/BaseStation/prod/v2/sportiduino-base-v2-assembly.jpg?raw=true "PCB")

To upload firmware to the station one needs another Arduino with ArduinoAsIsp firmware or any 
AVR microcontroller programming device such as Pickit2.

The outer shell is G1020BF. A hole is drilled in the box for the LED and the excess plastic is cut off so that the battery compartment fits in.
Moisture protection is achieved by covering all the PCB electronics with a liquid silicone compound.

[More details about the assembly and the initial setting of the station](https://github.com/alexandervolikov/sportiduino/blob/master/Doc/en/BaseStationAssembly.md)

### Consumption

(Translator's note: modes of operation are not very clear in Russian.)

To reduce consumption while waiting for the competition, the Hiberate mode is implemented for the controller as well as for the RC522. Consumption is 0.09 mA. To achieve this, on the RFID card the LED, resistor R1 and pull-up resistor R7/R2 on the RST line must be accurately removed and to increase antenna power the inductors L1, L2 replaced (described on the page "Base station assembly"). While Operating, up to 20 mA (up to 40 mA during the read-write) is consumed. One tag polling cycle takes up to 20 ms.

By default, the station starts in Standby mode. In this case, the tags are read once per second. Total consumption is up to 0.5 mA. Fully charged batteries last for 160 days. After the first tag approaches (by the umpire or the first athlete), the station goes into the Operating mode. By default after 6 hours of inactivity it returns to the Standby mode (the time can be changed using the settings byte, see below).

In the Operating mode, standby occurs every 250 ms. Thus, during operation, an integral consumption of about 1.7 mA can be expected. Fully charged batteries last for 45 days of continuous operation.

When approached by the Hibernate master tag, a station goes into the Hibernate mode, beeps 3 times and reloads. In the Hibernate mode the station total consumption is 0.09 mA. Fully charged batteries last for 5 years. When any tag approaches, the station goes into standby mode.

### Initial setting.

Station is set up with help of Master tags (any tag can be written as a Master tag) or an USB-to-TTL converter.

[More details in the User manual](https://github.com/sportiduino/sportiduino/blob/master/Doc/en/UserManual.md)

### Firmware (Station)

After power on the station checks the internal clock setup. If it is incorrect, it beeps accordingly (see below).
Then it loads from EEPROM a settings byte, station number and password.
Then the battery charge is checked. LED is powered for 5 seconds.
If the charge is low, it beeps accordingly.
At the end of the init stage the station emits one long beep and enters the Standby mode.
In the Standby mode the station wakes up every second and searches for a tag. When a tag is found, it goes into working mode with a tag polling every 0.25 seconds.
At the tag punch, the station reads the first card block, where the tag number or the master tag mark is stored. 
If it is a master tag, its instruction is being processed. If the tag is normal, then the last recorded page is searched for (using binary search) and the station number is read from the last record. If the station number is the same, the station emits a long beep. If the number is different, then the mark is written to the next block after the recorded one. If successful, the station sends a signal (a short beep and a LED blink), and also records the fact of the mark in the internal EEPROM memory of the station (into the bit corresponding to the card number, up to 4000 athletes for Ntag215).
Search and record time depend on the tag volume:

- 32 marks - 60 .. 120 ms
- 64 marks - 60 .. 140 ms
- 120 marks - 60 .. 160 ms
- 220 marks - 60 .. 180 ms

Factor in the Standby or Operational mode delay - up to 1000 or 250 ms accordingly.

During operation, a watch-dog is running, which restarts the station should the system hang up.

A fast mark mode exists. The last mark is always recorded into the sixth page on a tag. It reduced the tag lifetime.

### Settings byte. 

Programmed using a Master tag settings. Responsible for the duration of the operating mode according to the table below, for the operation mode of the stations - with or without separate start and finish stations, checking for overdue tags, tuning the antenna power and saving or deleting settings after reboot. When the start and finish stations are present (which is necessary when working with the SportOrg program), the start station (number = 240) will receive only cleared tags, other stations will only respond to tags with a mark at the starting station, and after the finish station (number = 245) the tag will no longer be noted at other stations before cleaning. This will avoid annoying mistakes and accidents. By default, the settings byte is 0. It also possible to set up saving or resetting the settings when a station goes out of Hibernation.

![](/Images/Setting-byte2.PNG)

### Station that clears tag.

To transfer the station to the cleaning mode, you need to specify station number 249.
In cleanup mode, the station clears all pages of the tag except of the page with the tag number and pages of the specified information. That is, all the marks are erased, the given chip number remains the same. The new time is recorded in the initialization time page, which is used to calculate the results, so it is important that the time at the station is correct.

When the tag is presented, the LED lights up on the station and blinks during the cleaning (about 5 seconds). If the cleaning is successful, the station emits a signal, the LED goes out, the chip needs to be removed. If there was no signal, the cleaning was not completed, you need to repeat the procedure.

### The Check station.

To transfer the station to the check mode, you need to specify station number 248.
In the verification mode, the station does not write anything to the chip but only checks it. If the chip is not empty or the time of initialization exceeds the time at the station for more than a month, the station emits three short signals. If everything is ok, then one short signal.
The test time is approximately the same as the time of the mark, so participants can practice the mark at this station.

### Password system.
Because the system is open and all the specifications are available to everyone, someone can write with any NFC device any master tag. There is a chance of vandalism - reprogramming of installed stations. And then, unlike if the station was simply broken down, the error might make its way into final results, which is difficult to notice and which can affect the standings. To protect against this phenomenon, a password system was introduced.

The password consists of three numbers from 0 to 255 (three bytes). The default password is 0,0,0. A master configuration tag is used to send the password to the base station. When you approach this tag to the station, if the old password is the same, then a new password is recorded. All master tags work only with the correct password in the first line. Also a byte of the settings is copied. The station beeps twice and reboots. When the Hibernate card is presented (with the correct password), the station will reset the password and settings, setting the bytes to 0.0.0.0. Enters Hibernate mode and reboots. Information about passwords and station setup is stored in the EEPROM memory.

### Signals

(the duration in milliseconds, the number of repetitions)

Errors:

- 100, 2 - an error when reading EEPROM memory;
- 100, 3 - the clock is not right;
- 100, 4 - the master chip password is wrong;
- 100, 5 - batteries should be replaced;
- 100, 6 - attempt to set station number as 0;

Normal behavior:

- 1000, 1 - signal when the station starts up;
- 500, 1 - the voltage is normal;
- 250, 2 - repetition of the approached tag;
- 500, 1 - the station mark was writen on the tag;
- 500, 1 - the chip is cleaned at the cleaning station;
- 500, 1 - the chip is normal at checking station;

Reading master tags (could be outdated):

- 500, 2 - read the master password tag;
- 500, 3 - the station read the master time tag;
- 500, 4 - the Hibernate master tag was read;
- 500, 5 - the station read the master tag with the station number;
- 500, 6 - read the master tag to dump station data;
