# User manual

This manual contains general information about the system and it is intended for users.
More information about the system including assembling instructions are available on page https://github.com/sportiduino/sportiduino/


[General information](#general-information)

[Instructions for participants](#instructions-for-participants)

[Instructions for course installers](#instructions-for-course-installers)

[Preparing participant cards](#preparing-participant-cards)

[Preparing base stations](#preparing-base-stations)

[Checking base stations](#checking-base-stations)

[Reading cards and preparing results](#Reading-cards-and-preparing-results)

## General information

### Cards

The system supports following types of RFID tags:
- [Mifare Ultralight C](http://www.nxp.com/documents/data_sheet/MF0ICU2.pdf) for 36 punches 
- [Mifare Classic 1K](https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf) for 42 punches 
- [Mifare Classic 4K](https://www.nxp.com/docs/en/data-sheet/MF1S70YYX_V1.pdf) for 90 punches 
- [NTAG213/215/216](https://www.nxp.com/docs/en/data-sheet/NTAG213_215_216.pdf) for 32/120/216 punches

This tags are produced in different form factors (cards).
Cards with small antenna may not work well.
NTAG cards are recommended for use.

### Base stations

Base stations are small boxes with flanges, weighing about 150 grams.
It powered by 3 AA batteries.
Depending on the mode of operation the battery life time is from 45 days to 2 years.
There are no buttons and no other interface at the station.
All information is exchanged contactlessly.
The optimum response area is 0.5 - 2 cm above the station.

![](/Images/BaseStation1.jpg?raw=true)

![](/Images/2017_09_30_09_34_45.jpg?raw=true)

A real-time clock IC is installed in the station.
Clock has a small accuracy error,
but the stations are not an accurate time device.
The time needs to be regularly adjusted.

To ensure longer battery life, base stations can operate in three modes:

* **Sleep mode.**
To store stations between competitions.
Card is polled once every 25 seconds. Practically do not consume energy.
Battery will last for more than 2 years.
To enable this mode special Sleep master card are used.
Awakening is carried out by applying any card or card with a magnet for instantaneous operation.
The magnet must be applied to the middle of the left edge of the station front side.
Also at awakening the internal memory is cleared.
* **Wait mode.**
Card is polled every 1 second.
Battery will last for a half of year.
* **Active mode.**
Card is polled every 0.25 seconds.
Fastest mark mode. Battery will last for 45 days.

Base stations, depending on the number assigned to them (from 1 to 255), can perform different functions:

- 1 - 239 — Regular punching station.
When you put down a card, the base station writes new data into it and gives a signal.
When re-presenting the card, the base station does not record the punch again, it just gives a double signal.
In the start/finish checking mode station does not punch and does not signal for cards which don't contain corresponding punches.
- 240 — Start station.
In the start/finish checking mode station does not punch and does not signal if the card is not empty.
In regular mode it works like regular punching stations.
- 245 — Finish station.
Works like a regular punching station.
- 248 — Check station.
If the card is empty and the initialization time is not more than a month behind the current one,
the station picks up once, otherwise it is silent.
The station does not write anything on the card and works as normal in speed so that participants can train in punching.
- 249 — Clear station.
Erases the entire card leaving the number unchanged.
Also it updates the initialization time on the card.
The LED blink all the time of the cleaning procedure.
A beep indicates the success of the procedure.

### Master station

A station with a USB connector for connecting to a computer.
It is needed to configure base stations, write and read cards.

![](/Images/MasterStation1.jpg?raw=true)

## Instructions for participants

* For punching, you should to bring the card approximately to the center of the white circle on the front side of the station.
* In case of successful punching the station will emit a sound signal and blink once with the LED.
* Optimum response range 0.5 — 2 cm.
* If the card is far from the station, the punch may not work the first time.
* When punching, you do not need to move the card from side to side, this makes it difficult to read and write the card.
* If you fail to punch remove the card and bring it up again, slowly lowering it to the center of the white circle at the station.
* Time of punching is from 0.06 to 0.4 seconds if the station is in Active mode.
* If you came to the station first it may need to be woken up station. The time of punching in this case will be from 0.06 to 1.2 seconds.
* When you try to re-punch, the station will emit two short beeps without recording a new punch.
If you are not sure whether you punched in the first time, it’s worth repeating.
* Without a mark at the Start station the card may not work. In this case you need to return to the start.
* If you came to the CP at the same time as another participant then you need to punch one at a time.
It is impossible to simultaneously bring two or more cards to the station. In this case the punch will be recorded only on one of the cards.

## Instructions for course installers

To work with stations you should use programs:
* SportiduinoPQ for preparing the participants' cards, setting up the stations.
* SportOrg for reading cards and competition results counting.

SportiduinoPQ can be downloaded [at page](https://github.com/sportiduino/SportiduinoPQ/releases).
In the release there is an portable exe file for Windows that does not require installation.
When the program is running, the entire log is saved to the log subfolder.

The SportOrg program is available at https://github.com/sportorg/pysport
There is also a user manual.

### Connecting the master station

To connect the station, click the `Connect` button.
If the connection is successful the message `Master station is connected` is displayed in the log area.

![](/Images/SportiduinoPQ-Connect.jpg?raw=true)

If you cannot connect the station, check the connection.
Problems may arise if used another device connected via serial port.
In this case open Device Manager in Control Panel of Windows, find out the Com port number assigned to the station and select it instead of "auto".
Also for station based on Arduino Nano with CHG340 chips you should install the appropriate driver.

### Preparing participant cards

Empty cards must be initialized (write the number and the current time).
To do this in the `Main` tab, you should enter the card number.
Next bring the card to the station and press the `Init Card` button.
Depending on the card type the initialization process takes from 3 to 7 seconds.
If successful the station will beep once.
If the `Autoincrement` box is checked the number in the input field will increase by 1.
If the process failed (series of short beeps) try the procedure again.

![](/Images/SportiduinoPQ-InitCard.jpg?raw=true)

The card number can be set in the range from 1 to 65535.
But base stations will not backup punches for cards with the number over 4000.
Therefore it is recommended to limit numbers up to 4000.

During initialization in addition to the card number the initialization time is also recorded.
This time is necessary for the reading of the card.
If more than six months have passed since the initialization or cleaning the data will be incorrect.
Therefore it is recommended to initialize the cards not long before the competition.

Already initialized cards can be cleared at Clear station with keeping the card number.

## Preparing base stations

### Quick preparing base stations

For quick preparing the base station by UART using USB-to-TTL converter.

![](/Images/ProgrammerWire.jpg?raw=true)

Connect it to the base station Prog connector. DTR pin must be disconnected.

If the station is sleeping, then it is not necessary to wake up it.
In the SportduinoPQ program, in the `Config` tab, in the `Config by UART` section
set the number of serial port corresponding the USB-to-TTL converter and click the `Write` button.
The current time and all other settings will be recorded in the base station.

![](/Images/SportiduinoPQ-ConfigByUart.jpg?raw=true)

To make sure that the settings are recorded correctly click on the `Read` button.
After that the current station settings will appear on the screen.
The station will remain in current mode.

![](/Images/SportiduinoPQ-ConfigByUart2.jpg?raw=true)

**Warning!**
Set the current password correctly in the SportduinoPQ program.
After the station is assembled the default password is 0,0,0.
If you do not correctly set the password, the base station will not accept settings from the master card.
You can reset password by writing new settings by UART.

![](/Images/SportiduinoPQ-Pwd.jpg?raw=true)

### Master cards

Another and main way to set up a station is master cards.
The master card is an ordinary card with settings written to it or commands for transmitting them to the base station.
To record a master card you need to bring it to the master station and press the appropriate button on the `Card` tab.
It is recommended to select the cards specifically for using as master and mark it
to avoid getting it in a heap of participants' cards.

To set up the stations, if they are in Sleep mode, you must wake up them first.
To do this, bring any card for up to 30 seconds.
After 3 seconds of LED lighting the station will beep once (the battery Ok) or beep 4 times (the battery is low).

### Station time setting

To set the time you need to bring card to the master station and click `Create` in the `Date/Time Card` section in the `Card` tab.

![](/Images/SportiduinoPQ-DateTimeCreate.jpg?raw=true)

The station will beep 4 times.
After the third signal bring the card to the base station.
The station will beep once.
If the last beep of the master station and the beep of the base station have merged into one then the time is set accurately.
Process the time setting in the Active mode due to the shorter card poll period.

The accuracy of the clock is 1 second per week.
For the Start and Finish stations it is recommended to update the time on the day of the competition.

### Assigning Station Number

In the SportiduinoPQ program in the `Card` tab set the station number (from 1 to 239),
bring the card to the master station and press the `Set Num` button.
Then bring this card to the base station.
The station will beep once.

There are separate buttons for special stations (Start, Finish, Clear and Check).

![](/Images/SportiduinoPQ-SetNum.jpg?raw=true)

### Put stations into Sleep mode

After the competition it is recommended to put all stations into Sleep mode to extend battery life.
To do this bring the cart to the master station.
In the SportiduinoPQ program in the `Card` tab click `Create` in the `Sleep Card` section.
Then bring the created master card to the base station.
The station will beep 4 times and will go into Sleep mode.
To awakening stations you need to bring a participant card to it for up to 30 seconds.

![](/Images/SportiduinoPQ-SetSleep.jpg?raw=true)

When creating a Sleep card you can specify the date the station automatically wakes up in the `Wake Up Time` field.
At this time the base station will automatically switch to Active mode and the first participant will not have to wait a second for punching.
If you want to put the base stations into Sleep mode for a long time then set any past date and time.

### Other base station settings

The duration of the Active mode, additional checks and antenna gain of the base station can be set on the tab `Config`,
and record to the station using the master card.

![](/Images/SportiduinoPQ-OtherSettings.jpg?raw=true)

Using passwords protects unauthorized change of the stations settings.
After the build, the default password is 0, 0, 0.
If you set a new password and forget it then to reset the password you will need to update the settings by UART.
It is recommended that you keep your password in a safe place.

Settings details:
- `Active Time` — the working time in active mode.
If during this time no one punch at the station the station will go to the Waiting mode.
- `Start/Finish` — enables checking Start and Finish punches at the participants' cards.
- `Check InitTime` — enables checking the initialization time of card.
- `Fast Mark` — enables quick punching mode.
This reduces the time spent by the participant when the base station records the punch on the participant's card.
But it also reduces the resource life of the cards and its will fail faster.
- `Antenna Gain` - gain of MFRC522 module.

### Reading Station Backup Memory

During the work, the base station reads punches to the internal memory.
It is true only for cards with numbers up to 4000.
Using this function, it is possible to recover of the data in case of card lost, check of controversial situations, and also in the search for lost participants.

To read the backup you need to create the master card.
Bring the card to the master station and in the SportiduinoPQ program click `Create` button in the `Dump Card` section in the `Card` tab.
Then bring the master card to the base station.
The process is long, you need to carefully keep the card in the optimal recording area above the center of the white circle.
After successful bring the card to the master station and click `Read` button in the `Dump Card` section.
The result will be displayed in the log area.

![](/Images/SportiduinoPQ-SetDump.jpg?raw=true)

### Checking base stations

To check the settings of the base station, the battery charge level and its current operation mode
create the State master card in the SportiduinoPQ program.
In the `Card` tab click `Create` button in the `State Card` section and after that bring the created master card to the base station.
If the base station is in Sleep mode, you will need to wait for 30 seconds.
The base station will turn on the LED for 3 seconds and beep once,
after that the master card should be removed.
If the base station was in Sleep mode, then it will remain so in it, it does not need to be transferred again to Sleep mode.
If within 30 seconds the base station does not respond to the card then it is broken or the battery is low or the master card should be recreated.
After the base station has recorded the information on the master card
bring the master card to the master station and in the SportidunoPQ program click `Read` in the `State Card` section.
After that all information about the base station will be displayed in the log area.
Also the field values in all tabs of the SportiduinoPQ will changed in accordance with the current settings of the base station.

![](/Images/SportiduinoPQ-GetInfo.jpg?raw=true)

## Reading cards and preparing results

It is recommended to use the SportOrg program for reading cards and generating results.
The peculiarity of using this program is that it is necessary to use the Start station (240) and the Finish station (245),
otherwise an error will occur when reading.

You can also use SportiduinoPQ to quickly view punches from cards.
To do this bring the card to the station and in the `Main` tab, click `Read Card` button. 
The result will be displayed in the log area.

![](/Images/SportiduinoPQ-ReadCard.jpg?raw=true)

