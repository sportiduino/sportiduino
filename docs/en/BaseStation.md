# Base Station

### Scheme

The main components of the station: microcontroller ATmega328P-AU, RFID module RC522, real-time clock DS3231SN.
It is powered by 3 x AA batteries via the MCP1700T-33 linear regulator.
The scheme and PCB are made in KiCad program.

![](/hardware/BaseStation/prod/v3/sportiduino-base-v3-scheme.png?raw=true "Scheme")

The components are mounted on a printed circuit board.
Connection with an RFID board is via soldered connection with a pin connector.
You can order the manufacture of PCB in China (e.g. [JLCPCB](https://jlcpcb.com/)).
It will cost around 0.9-1$ per piece with delivery.

![](/hardware/BaseStation/prod/v3/sportiduino-base-v3-pcb.png?raw=true "PCB")

To upload firmware to the station one needs another Arduino with ArduinoAsIsp firmware or any 
AVR microcontroller programming device such as Pickit2.

The outer shell is G1020BF.
Moisture protection is achieved by covering all the PCB electronics with a liquid silicone compound.

About the assembly and the initial setting of the station written in [the assembly manual](BaseStationAssembly.md).

### Consumption

By default the station starts in Wait (standby) mode.
In this case, the cards are read once per second.
Total consumption is up to 0.5 mA.
Fully charged batteries last for 160 days.
While RFID module is powered up to 20 mA (up to 40 mA during the read-write) is consumed.

After the first card is brought (by the station installer or the first athlete),
the station goes into the Active mode.
By default after 2 hours of inactivity it returns to the Wait mode.
The time in Active mode can be changed using the Config card.

In the Active mode standby occurs every 250 ms.
Thus during operation an integral consumption of about 1.7 mA can be expected.
Fully charged batteries last for 45 days of continuous operation.

To reduce consumption while waiting for the competition, the Sleep mode is implemented.
Consumption is about 15 µA.
Fully charged batteries last for 5 years.
To achieve this, on the RFID board the LED and pull-up resistors must be accurately removed (see [Base Station Assembly](BaseStationAssembly.md)).

### Initial setting

Station is configured by Master cards (any card can be written as a Master card) or by USB-to-TTL converter.

[More details in the User manual](UserManual.md).

### Station operation algorithm

After power on or reboot the station checks the internal clock setup.
If it is incorrect, it beeps accordingly (see below).
Then it loads from EEPROM the settings (station number, password, etc.).
Then the battery charge is checked.
LED is powered for 3 seconds.
If the charge level is ok or low, it beeps accordingly (see below).
After the init stage the station enters the Wait mode.
In the Wait mode the station wakes up every second and searches for a card (RFID tag).
When a card is found, it goes into Active mode with a card polling every 0.25 seconds.
At the card punch, the station reads the first card block, where the card number or the master card code is stored. 
If it is a master card, its instruction is being processed.
If the card is normal, then the last recorded page is searched for (using binary search) 
and the station number is read from the last record.
If the station number is the same, the station beeps twice.
If the number is different, then the punch is written to the next block after the recorded one.
If successful, the station signal (a beep and a LED blink),
and also records the fact of the punch in the internal EEPROM memory of the station (into the bit corresponding to the card number).
If the station PCB has EEPROM IC, the punch is written in one.

Search and record time depend on the card volume:

- 32 marks - 60 .. 120 ms
- 64 marks - 60 .. 140 ms
- 120 marks - 60 .. 160 ms
- 220 marks - 60 .. 180 ms

Factor in the Wait or Active mode delay - up to 1000 or 250 ms accordingly.

A Fast mark mode exists.
The last station number is always recorded into the sixth page on a card.
It reduced the card lifetime.

During operation, a watch-dog is running which restarts the station should the system hang up.

### Station configuration 

Using the Config master card, you can set:

- the duration of the Active mode,
- checking start and finish punches,
- checking the initialization time of the cards,
- Fast mark mode,
- antenna gain,
- password.

With checking the start and finish punches, the Start station (number 240) will only accept cleared cards.
Other stations will respond to the cards only with a punch of the Start station.
And after the Finish station (number 245), the card can no longer be punched at other stations before cleaning.
This will avoid any mistakes and accidents.
By default all these checks are disabled, the password is 0-0-0.

### The Clearing station

This station clears cards.
To set up the station as Clearing station, you need to specify station number 249.
This station clears all pages of the card except the page with the card number.
The new time is recorded in the initialization time page
which is used to calculate the results, so it is important that the time at the station is correct.

When the card is presented, the LED lights up on the station and blinks during the cleaning (about 4 seconds).
If the cleaning is successful, the station beeps, the LED goes out, the card needs to be removed.
If there was no signal, the cleaning was not completed, you need to repeat the procedure.

### The Check station

To set up the station as the Check station, you need to specify station number 248.
This station does not write any data to the card but only checks it.
If the card contains punches or the time of initialization exceeds the time at the station for more than a month
the station does not beep.
If everything is ok then it beeps once.
The time of checking is approximately the same as the time of punching, so participants can practice the punching at this station.

### Password system

A password system is used to protect against unauthorized resetting of installed stations.

The password consists of three numbers from 0 to 255 (three bytes).
The default password is 0-0-0.
All master cards work only with the correct password in the first data block.
A Config master card is used to send the password to the base station.
The password is stored in the EEPROM memory.

### Signals

Errors:

| Repetitions | Duration in ms | Meaning |
| --- | --- | --- |
| 3 | 100 | The clock is not right
| 4 | 100 | The master card password is wrong
| 5 | 100 | The battery is low
| 4 |  50 | Unknown master card
| 2 | 250 | Master card error
| 2 | 250 | Serial error

Normal behavior:

| Repetitions | Duration in ms | Meaning |
| --- | --- | --- |
| 1 | 1000 | Battery is ok
| 1 |  500 | Card punching, clearing or checking is ok
| 2 |  250 | Card is already punched
| 1 |  250 | Master card read
| 4 |  500 | Sleep mode is enabled
| 1 |  250 | Serial is ok

