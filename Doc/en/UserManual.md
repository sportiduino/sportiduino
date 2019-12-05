# User manual

This instruction contains general information about the system and it is intended for users. More information about the system as well as assembling instuctions are availabe on github: https://github.com/alexandervolikov/sportiduino/


[General information](#general-information)

[Instructions for participatns](#instruction-for-participants)

[Instructions for distance installers](#instruction-for-distance-installers)

[Preparing participant cards](#preparing-participant-cards)

[Preparing base stations](#preparing-base-stations)

[Checking base stations](#checking-base-stations)

[Reading cards and preparing results](#Reading-cards-and-preparing-results)

## General information

### Cards

It is recommended to use Ntag215 cards. Their memory is enough to record 120 marks. It can be made in a different form factor. Individual cards may operate uncertainly at stations, depending on the area of the antenna.

Also the system supports:
- [Mifare Ultralight C](http://www.nxp.com/documents/data_sheet/MF0ICU2.pdf) = 36 marks
- [Mifare Classic 1K](https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf) = 42 marks
- [Mifare Classic 4K](https://www.nxp.com/docs/en/data-sheet/MF1S70YYX_V1.pdf) = 90 marks
- [Ntag213/215/216](https://www.nxp.com/docs/en/data-sheet/NTAG213_215_216.pdf) = 32/120/216 marks

### Base stations

Base stations are small boxes with flanges, weighing about 150 grams, powered by 3 AA batteries. Depending on the mode of operation, the batteries life time is from 45 days to 2 years. There are no buttons and no other interface at the station. All information is exchanged contactlessly. The optimal pickup zone is 0.5-1 cm above the station.

![](/Images/BaseStation1.jpg?raw=true)

![](/Images/2017_09_30_09_34_45.jpg?raw=true)

The station has a clock with a small error of course, but the stations are not a device of the exact time, the time needs to be regularly adjusted.

To ensure longer battery life, base stations can operate in three modes:

* Sleep mode. To store stations between competitions. Chip is polled once every 25 seconds. Practically do not consume energy. Batteries will last for more than 2 years. To transfer to this mode, specially recorded master cards are used. To wake-up it needs to present any card on the station. Also, when removing from sleep, the internal memory is cleared.
* Wait mode. Poll chip every 1 second. Batteries will last for about six months.
* Active mode. Poll chip every 0.25 seconds. Fastest mark mode. Batteries will last for 45 days.

Base stations, depending on the number assigned to them (a number from 1 to 255), can perform different functions:

1-239 - normal station mark. When you put down a card, the base station writes new data into it and gives a signal. When re-presenting the chip, the base station does not record the mark again, it just gives out a signal. In the mode of operation, the start / finish stations do not produce a mark and do not signal for chips that did not start or did not finish.

240 - start station. In the operation mode of the start / finish station, they do not make a mark and do not signal if the chip is not empty. In normal mode, they work like normal base stations.

245 - finish station. Works like a normal base station.

248 - check station. If the chip is empty and the time of its initialization / cleaning is not more than a month behind the current one, the station picks up once, otherwise it is silent. The station does not write anything on the chip and works as normal in speed, so that participants can also train at the mark.

249 - clear station. Erases the entire chip, leaving the number unchanged, and also updates the initialization time on the chip. When the chip is brought up, the LED lights up and lights up all the time of the cleaning procedure, a beep indicates the success of the procedure. When you bring the newly cleaned chip, the station simply beeps.

### Master station

A station with a USB connector for connecting to a computer is needed to configure the system, write and read chips.

![](/Images/MasterStation1.jpg?raw=true)

## Instruction for participant

It is advisable to print these instructions in large print and hang them to familiarize participants in the competition.

* To mark, you need to bring the chip to the area above the orange sticker (or white circle).
* If the station is successful, the station will beep and the LED will flash.
* The optimal response range is about 1 cm above the orange sticker.
* If you bring the chip close, or if the chip is far from the station, the mark may not work the first time.
* When checked, it is not necessary to move the chip from side to side, which makes it difficult to read and write to the chip.
* If it was not possible to check in, remove the chip and re-lift it, slowly lowering it to the station above the orange sticker (or white circle).
* Time stamp is from 0.06 to 0.4 seconds if the station is in operation.
* If you came to the station first, you may need to wake her up, time stamp in this case will be from 0.06 to 1.2 seconds.
* Mark is contactless, no need to press anything.
* When trying to re-mark the station will produce two short repeated signals without recording a new mark. If you are not sure whether you were marked the first time, it is worth repeating.
* Without a mark at the starting station, the chips may not work, in this case you need to return to the start
* If you came to a CP at the same time as another participant, then you need to check in by turns. You can not simultaneously attach to the station mark two or more chips, in this case, the mark will be recorded only on one of the chips

## Instructions for course installers

To work with stations, you should use the SportiduinoPQ program - this is preparing the participants' chips, setting up the stations. Reading chips and counting results is implemented in the SportOrg program.

SportiduinoPQ can be downloaded in the [latest release](https://github.com/alexandervolikov/SportiduinoPQ/releases). In the release there is an exe file that does not require installation. When the program is running, the entire log is saved to the log subfolder.

The SportOrg program is available at: http://sportorg.o-ural.ru/ There is also a user manual

### Connecting the master station

To connect the station, click the Connect button. If the connection is successful, the "master station is connected" is displayed in the log.

![](/Images/SportiduinoPQ-Connect.jpg?raw=true)

If you cannot connect the station, check the connection. Problems may arise if the system uses another device connected via the com port. In this case, through the Control Panel - Device Manager, find out the com port number assigned to the station and select it instead of "auto". Also, stations on Arduino Nano with CHG340 chips may not work without installing the appropriate driver.

### Preparing participant cards

Empty chips must be initialized (write the number and the current time on them). To do this, in the Card tab, you must enter the chip number. Next, bring the chip to the station and press the Init Card button. Depending on the chip, the initialization process takes from 3 to 7 seconds. If successful, the station will emit one signal. If the autoincrement box is checked, the number in the input field will increase by 1. If the process was unsuccessful (three short beeps and one long), try the procedure again.

![](/Images/SportiduinoPQ-InitCard.jpg?raw=true)

The chip number can be set in the range from 1 to 65535. But base stations will not remember the fact that they are marked when the chips with the number over 4000 are marked, therefore it is recommended to limit to numbers up to 4000.

During initialization, in addition to the chip number, the initialization time is also recorded, which is necessary for the subsequent reading of the chip; if more than six months have passed since the initialization or cleaning, the data will be incorrect. Therefore, it is advisable to initialize the chips not long before the competition.

Already previously initialized chips can be cleared while maintaining the number at the cleaning station.

## Preparing base stations

### Quick preparing base stations

A converter is used for quick preparation of a base station in one click [USB-to-TTL](https://ru.aliexpress.com/item/32294938771.html?spm=a2g0v.search0104.3.72.3074220cAo4Mco&ws_ab_test=searchweb0_0%2Csearchweb201602_3_10065_10068_319_317_10696_453_10084_454_10083_10618_10307_10301_537_536_10059_10884_10887_321_322_10915_10103_10914_10911_10910%2Csearchweb201603_52%2CppcSwitch_0&algo_expid=60eadde8-7142-4cf0-a8a6-ed0a9acfa548-10&algo_pvid=60eadde8-7142-4cf0-a8a6-ed0a9acfa548).

![](/Images/UsbToTtl.jpg?raw=true)

Connect it to the base station connector X4. This will require a Pin-to-Pin harness with 3 wires RX, TX and GND.

![](/Images/ConfigWire.jpg?raw=true)

![](/Images/BaseStationSerialConfConnect.jpg?raw=true)

It does not matter whether the station is in active mode or sleep mode. If the station is sleeping, then it is not necessary to wake her up. In the SportduinoPQ program, in the Settings # 2 tab, in the Config by Uart section, set the com-port number of the USB-to-TTL converter and click the Write button. At the same time, the current time and all other settings will be recorded in the base station, the base station will also go to sleep.

![](/Images/SportiduinoPQ-ConfigByUart.jpg?raw=true)

To make sure that the settings are recorded correctly, click on the Read button. After that, the current station settings will appear on the screen. At the same time, the station will remain in sleep mode.

![](/Images/SportiduinoPQ-ConfigByUart2.jpg?raw=true)

<b>Warning: </ b> Set the current password correctly in the SportduinoPQ program. After the station is assembled, the default password is 0.0.0. If you do not correctly set the password, the base station will not accept settings from the master chip or from the USB-to-TTL converter!

![](/Images/SportiduinoPQ-Pwd.jpg?raw=true)

### Master cards

Another way to set up a station is master chips. The master chip is an ordinary chip with settings written to it or commands for transmitting them to the base station. To record a master chip, you need to bring it to the master station and press the desired button depending on the task. It is recommended to select the chip specifically for use in these purposes and somehow mark it to avoid getting it in a heap of chips with participants, as in this case various unexpected situations are possible if the chip is active.

To tune in the stations, if they are in a state of sleep, you must wake them up first. To do this, bring any chip party for up to 30 seconds. The station will emit a short signal or two short signals.

### Station time setting

To set the time, you need to attach any chip to the master station and click Create in the Date / Time Card section in the Settings # 1 tab.

![](/Images/SportiduinoPQ-DateTimeCreate.jpg?raw=true)

The station will emit three signals at intervals of a second. After the second signal, you need to attach the chip to the base station. In this case, the station will emit 3 signals and reboot. If the last signal of the master station and the first signal of the base station have merged into one, then the time is set fairly accurately. The time setting is best carried out in the station working mode due to the shorter period of station inactivity.

The accuracy of the clock (subject to the use of high-quality components and compliance with the soldering technology) is 1 second per week. For critical stations - start, finish, it is recommended to update the time on the day of the competition.

To fine tune the base station time, use a USB-to-TTL converter.

### Assigning Station Number

In the SportiduinoPQ program in the Settings # 1 tab, dial the station number (from 1 to 239), bring the chip to the station and press the Set Num button. Then bring this chip to the station. The station will emit 5 signals and reboot with a new number.

To set special stations, separate buttons are highlighted.
Set Start - station start
Set Finish - finish station
Check St - check station
Clear St - cleaning station

![](/Images/SportiduinoPQ-SetNum.jpg?raw=true)

### Put stations into sleep mode

After the competition, it is recommended to put all stations into sleep mode to extend battery life. To do this, bring the chip to the master station. In the SportiduinoPQ program in the Settings # 1 tab, click Create in the Sleep Card section. Then bring the created master chip to the base station. The station will emit 4 signals, reboot and go into sleep mode. To remove stations from this mode, you need to attach a participant chip to it for up to 30 seconds.

![](/Images/SportiduinoPQ-SetSleep.jpg?raw=true)

When creating a master chip to put the base station into sleep mode, you can specify the date the station automatically wakes up in the Competition Date / Time field. For example, you set up a station to compete. When creating a sleep master chip, specify the date and time of the start of the competition, then the base station will automatically switch to active mode at this time and the first participant will not have to wait for a long time to check in. Also, the distance installer will not have to wake the station before the start. If you want to put the base stations into sleep mode for a long time, then indicate any past date and time.

### Other base station settings

In the default mode, the station does not use passwords, separate starting and finishing stations, does not check the initialization time, the transition time from the operating mode to the standby mode is 6 hours, no passwords are used. If these parameters do not suit, they can be configured. To do this, in the Settings tab # 2, you need to change the necessary parameters, bring the chip and click Create Pwd Card. The master station will emit two signals, bring the chip to the base station, which will also emit two signals and reboot.

![](/Images/SportiduinoPQ-OtherSettings.jpg?raw=true)

The use of passwords protects stations from being programmed by foreign master chips with the wrong password, which can protect against such vandalism. After assembly, the default password is 0.0.0. If you set a new password and forget it, then you will have to reflash the station, so it’s better to write down the new password somewhere on paper!

Settings together with passwords can be saved by clicking the Save Set button, then they can be loaded - Load Set. It is recommended to save the settings when setting new passwords in order to remember them.

Settings details:
- Work Time - Time work in active mode. If during this time no one marks the station, the station will go to standby mode (but not sleep)!
- Start / Finish - Enables checking on the chips of participants mark on the starting station
- Check InitTime - Enables checking the chip's initialization time
- AutoDel Set - If enabled, returns the default settings for the base station after it goes to sleep
- Fast Mark - Enables quick mark mode. This reduces the time spent by the participant when the base station records the mark on the participant's chip. But it also reduces the resource life of the participants and the chips will fail faster
- Antenna Gain - Gain Module MFRC522

### Reading Station Log

During the work, the base station keeps a log - records the fact of marking with one or another chip number. The time stamp is not recorded at the same time, the log is kept only for chips with numbers up to 4000. Using this function, it is possible to recover part of the data in case of loss of chips, check of controversial situations, and also in the search for dropped or lost participants. The base station log is cleared when going to sleep.

To remove the log, you need to record the master chip, to do this, bring the chip to the master station and in the SportiduinoPQ program, click Create in the Dump Card section in the Settings # 1 tab. The process of preparing the master chip takes 5 seconds. Then bring the master chip to the base station. The process is long, you need to carefully keep the chip in the optimal recording area (0.5 - 1 cm above the orange mark (or white circle)). After successful logging, bring the chip to the master station and click Read in the Dump Card section. The result will appear in the log.

![](/Images/SportiduinoPQ-SetDump.jpg?raw=true)

### Checking base stations

To check the settings of the base station, the battery charge level and its current operation mode - create a master chip in the SportiduinoPQ program to read the station status. To do this, in the Settings # 1 tab, click Create in the Status Card section and after that bring the created master chip to the base station. If the base station is in sleep mode, you will need to wait for 30 seconds. The base station will turn on the LED for 5 seconds and after a while will emit a short beep, after which the master chip should be removed. If the base station was in sleep mode, then it will remain so in it, it does not need to be transferred again to sleep mode. If within 30 seconds the base station does not respond to the chip in any way, then either it is out of order or the batteries are exhausted or the master chip should be recreated. After the base station has recorded the information on the master chip, attach the master chip to the referee station and in the SportidunoPQ program, click Read in the Status Card section. After that, all information about the base station will be displayed in the log. Also, the field values ​​in all tabs of the SportiduinoPQ will change in accordance with the current settings of the base station.

![](/Images/SportiduinoPQ-GetInfo.jpg?raw=true)

## Reading cards and preparing results

It is recommended to use the SportOrg program for reading chips and generating results. The peculiarity of using this program is that it is necessary to use the starting station (240) and the finishing station (245), otherwise an error will occur when reading. You can pre-mark all the chips at the launch station, distribute to the participants, and then in the program, set the start time or set the start countdown from an arbitrary station. You also need to put the station into the mode of start / finish stations to avoid errors.

You can also use SportiduinoPQ to quickly view results on individual chips. To do this, bring the chip to the station and in the Main tab, click Read Card. The result will be displayed in the log.

![](/Images/SportiduinoPQ-ReadCard.jpg?raw=true)
