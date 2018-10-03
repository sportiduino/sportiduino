# User manual

## General information

### Chips

Recommended for use are Ntag215 chips. Their memory is enough to record 120 marks with the recording of the full time (year-month-day-hour-minute-second). Can be made in a different form factor. Separate chips can be unstable at stations, depending on the area of ​​the antenna.

### Base stations

They are small boxes with flanges, weigh about 150 grams. Powered by 3 AA batteries, depending on the battery mode, the batteries last for 45 days to 2 years. Buttons and other interface in the station there, all the information exchange, the recording of chips occurs in a contactless mode, the optimum trigger zone is 0.5-1 cm above the orange sticker. The station has a clock that has a small error of travel, but the stations are not a device of the exact time, the time needs to be regularly adjusted.

To ensure a longer battery life, the base stations can operate in three modes:

* Sleep mode. To store stations between competitions. The chip is polled every 25 seconds. Virtually do not consume energy. Batteries will last for more than 2 years. To transfer to this mode, specially recorded master chips are used. The conclusion is made by applying any chip. Also, when waking from sleep, the internal memory is cleared
* Standby mode. Interrogate the chip once every 1 second. By default, when the chip is checked, it switches to the operating mode. Batteries will last about six months.
* Work mode. Interrogate the chip once every 0.25 seconds. The fastest mode of marking. Batteries will last for 45 days. By default, if you are idle for more than 6 hours, it goes into standby mode.

Base stations, depending on the number assigned to them (number from 1 to 255) can perform different functions:

1-239 - usual base station. When the chip is presented, new data is written into it and a signal is output. When the chip is not delivered again, the recording is not made, the signal is output. In the operating mode, the start / finish stations do not make a mark and do not signal for chips that did not start or finish.

240 - start station. In the operating mode, the start / finish stations do not make a mark and do not signal if the chip is not empty. In normal mode, they work as ordinary base stations

245 - finish station. Works as a normal base station

248 - verification station. If the chip is empty and its initialization / cleaning time does not lag more than a month from the current one, the station picks once, otherwise - three. The station does not record anything on the chip and it works on speed as usual, so that participants can also be trained in the mark.

249 - cleaning station. Erases the entire chip, leaving the number unchanged, and also updates the initialization time on the chip. When the chip is lit up, the LED lights up and the cleaning procedure is lit all the time, the audible signal indicates the success of the procedure. When the newly-cleared chip is repeatedly presented, the station simply emits a signal.

### Master station

A station with a USB connector for connection to a computer is needed to configure the system, record and read the chips.

## Instruction for participants

It is advisable to print this instruction in large print and hang it for participants at the competitions.

* For a mark, you need to bring the chip to the area above the orange sticker.
* In the case of a successful mark, the station will emit a beep and / or blink the LED
* Optimum range of operation - about 1 cm above the orange sticker.
* If you bring the chip close, or if the chip is far from the station, the mark may not work the first time.
* When marking, you do not need to move the chip from side to side, this makes it difficult to read and write the chip.
* If it did not work out, remove the chip and again deliver it, slowly lowering it to the station above the orange sticker.
* The time of the mark is from 0.06 to 0.4 seconds, if the station is in the working mode.
* If you came to the station first, it may need to be awakened, the time of marking in this case will be from 0.06 to 1.2 seconds.
* The mark is contactless, you do not need to click on anything.
* If you try to re-mark the station, it will make a longer signal without recording a new mark. If you are not sure whether you checked in from the first time, it is worth repeating.
* Without a mark at the start-up station, the chips may not work, in this case it is necessary to return to the start

## Instruction for the production team

To work with stations it is necessary to use the program SportiduinoPQ, it is the preparation of chips, tuning stations. Reading chips and counting the results is implemented in the program SportOrg.

The program SportiduinoPQ can be downloaded in the latest release https://github.com/alexandervolikov/SportiduinoPQ/releases. In the release there is an exe file that does not require installation. When the program is running, the entire log is saved to the subfolder log.

The SportOrg program is available at: http://sportorg.o-ural.ru/ There is also a user manual

### Connecting the master station

To connect the station, press the Connect button. If the connection is successful, the "master station is connected" will be displayed in the log.
If the station can not be connected, check that the connection is correct. Problems can occur if the system uses a different device connected via com port. In this case, you need to find out the com port number assigned to the station through the Control Panel - Device Manager and select it instead of "auto". Also, stations on Arduino Nano with chips CHG340 may not work without installing the appropriate driver.

### Preparation of chips.

Empty chips must be initialized (write down the number and current time on them). To do this, enter the number of the chip in the Card tab. Bring the chip to the station and press the Init Card button. The initialization process takes 3 to 7 seconds depending on the chip. In case of success, the station will emit one signal. If there is an autoincrement tick, the number in the input field will increase by 1. If the process has failed (three short beeps and one long one), try the procedure again.

The chip number can be set in the range from 1 to 65535. But the base stations with the mark of chips with the number more than 4000 will not remember the fact of their marking, therefore it is recommended to limit numbers to 4000.

When initializing, in addition to the chip number, the initialization time is also recorded, which is necessary for the subsequent reading of the chip, if more than six months have elapsed since the initialization or cleaning, the data will be incorrect. Therefore, it is desirable to initialize the chips not long before the competition.

Already earlier, the initialized chips can be cleared with saving the number on the cleaning station.
Preparing Stations

### Master Chips

To configure the stations, master chips are used. The master chip is an ordinary chip with the data recorded on it for transmission to the base station. To record the master chip, you need to bring it to the master station and press the desired button depending on the task. It is recommended to select the chip specifically for use for this purpose and somehow mark it, in order to avoid getting it into a heap with chip participants, since in this case various unexpected situations are possible if the chip turns out to be active. After each use of the chip, it is deactivated, it is necessary to record it every time you use it.

To set up stations, if they are in a state of sleep, you must first awaken them - bring any chip for up to 30 seconds. There will be a procedure to exit the sleep state: the LED will light up for 5 seconds, the station will measure the charge of the batteries (if the normal charge produces one long signal, if the batteries need to replace five short ones). Then it will reboot and after 5 seconds will emit one short signal. If the station emits three short beeps 5 seconds before the long one, then the clock on it goes wrong, they need to be adjusted.

### Setting the station time

To set the time, you must attach the chip to the master station and click Set Time in the Station tab. The station will emit three signals at intervals per second. After the second signal, you must attach the chip to the base station. In this case, the station will emit 3 signals and reboot. If the last signal from the master station and the first signal from the base station have merged into one, then the time is adjusted quite accurately. It is better to set the time in the working mode of the station in view of the shorter inactivity period of the station.

Accuracy of the clock (with the use of quality components and compliance with soldering technology) is 1 second per week. For critical stations - start, finish it is recommended to update the time on the day of the competition.

### Assigning the station number

Dial the station number (from 1 to 239), bring the chip to the station and press the Set Number button. Then the chip is brought to the station. The station will issue 5 signals and will reboot with the new number.

To assign special stations, separate buttons are highlighted
Set Start - start station
Set Finish - finish station
Check St - check station
Clear St cleaning station

### Putting the stations into sleep mode

After the competition, it is recommended that all stations be put into sleep mode to extend battery life. To do this, you need to bring the chip to the master station, press the Sleep Card and bring it to the base station. The station will emit 4 signals, reboot and go into sleep mode. To output stations from this mode, you need to attach a chip to it for up to 30 seconds.

### Setting up the station

In the default mode, the station does not use passwords, separate starting and finishing stations, does not check the initialization time, the transition time from the operating mode to the standby mode is 6 hours, no passwords are used. If these parameters do not suit, they can be configured. To do this, in the Settings tab, you need to change the necessary parameters, bring the chip and press Pass Master. The master station will emit two signals, bring the chip to the base station, which will also emit two signals and reboot.
Some stations may not work quite well - the chip does not work when you bring it close, which can make it difficult to mark. To do this, you need to gradually reduce the power of the antenna (by default, 48 dB).

The use of passwords protects the station from the possibility of programming them by someone else's master chips with the wrong password, which can protect against such vandalism. To use passwords, enter three bytes of the old password (numbers from 0 to 255). By default, this is three zeros. Also note that when you put the stations into sleep mode, the password is reset to the default password - 0.

Settings along with passwords can be saved by pressing the save button, then you can load them - load. It is recommended to save the settings when setting new passwords to remember them.

### Removing the station log

During operation, the base station logs - records the fact of a mark by one or another chip number. The time stamp is not recorded, the log is kept only for chips with numbers up to 4000. Using this function, it is possible to restore a part of the data in case of losing chips, checking controversial situations, and searching for lost or lost participants.

To remove the log, you need to record the master chip, for this purpose bring the chip to the master station and press the Sump Card in the Station tab. The process of master-chip preparation takes 5 seconds. Then bring the master chip to the base station. The process is long, you need to carefully hold the chip in the optimal recording area (0.5 - 1 cm above the orange mark). After successfully removing the log, bring the chip to the master station and click Read Dump. The result will appear in the log.

## Reading chips, generating results

It is recommended to use the SportOrg program to read the chips and generate the results. The peculiarity of using this program is that it is necessary to use the starting station (240) and the finishing station (245), otherwise an error will occur during reading. You can pre-mark all the chips at the start station, give out to the participants, and then in the program set the start time or set the start count from the arbitrary station. It is also possible to move the stations to the start / finish station mode to avoid an error.

To quickly view the results for individual chips, you can also use the SportiduinoPQ. To do this, bring the chip to the station and in the Card tab, click Read Chip. The result will be displayed in the log.