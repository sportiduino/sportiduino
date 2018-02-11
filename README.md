![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/logo.png)

#### Version 1.0.0

![](https://raw.githubusercontent.com/alexandervolikov/sportIDuino/master/Images/Sportiduino.JPG)

[Чиать это на русском](https://github.com/alexandervolikov/sportiduino/blob/master/README.ru.md)

This repository is devoted to the hardware part of the marking system.

At this moment the active development is completed, the forces are transferred to the development of software for working with the system.

Please create an issue if you find a bug or if you have any questions or suggestions regarding the hardware of the system. On other issues, please write to the mail (there is in the profile information).
I will be glad to receive active help in the form of a pull request. Also I support the creation of Forks.

The software is under construction, the links will be placed below in the section "Data processing".

Also, the information placed in the project wiki will be gradually added and updated

This project is open and free. Who is not afraid of difficulties, can try to do it by yourself following the instructions from the wiki. Сheapness of the components of the system can recoup the expended work ($ 10 for one base station, $ 0.2 per chip mark)

I warn that this development is a hobby, the author is not a professional in the field of electronics and programming. Therefore, no guarantees are given, different kinds of problems are possible during reproduction. Support is also not promised. So, act at your own risk. 

****

[documentation on the wiki](https://github.com/alexandervolikov/sportiduino/wiki/Sportiduino)

## Cards

The system uses cards Ntag 213 / 215 / 216. In the form of labels these cards cost 0.1, 0.2 0.4 $ , respectively, in the form of a key fob twice as expensive. The memory of these cards is enough for 32, 120 and 216 marks, respectively.

[Read more here](https://github.com/alexandervolikov/sportiduino/wiki/Card-Ntag)

## Base stations.

The main components of the station are the Atmega328P microcontroller and the MFRC522 module, which operates at a frequency of 13.56MHz. Powered by 3 AA batteries through the MCP1700T-33 stabilizer

There are 3 operating modes of the station:

* Sleep (storage) mode. Looking for a card once every 25 seconds. When the card is found, a reboot takes place with entering to the standby mode. At the same time, the status of the system is signaled - the presence of errors and the status of the battery, cleaning the log of the stations.

* Work mode. looking for a card once every 250 ms. By default, if you are idle for more than 6 hours, it goes into standby mode. It is possible to set the transition time.

* Standby mode. looking for a card once every second. When the card is in the working mode.

The capacity of the kit of three alkaline AA batteries (4.5 V, 2000mAh), roughly, will last for 5 years in sleep mode, 160 days in standby mode or 40 days in operating mode. One set should be enough for a year of active use.
The stations can also be operated at negative temperatures (the work has been tested up to -20 C), but the shortening of the service life due to the drop in the capacity of the batteries in the cold should be taken into account.

The station has a DS3231SN clock. Time is corrected using a special card, which is recorded by the master station.

Reliability of the work of the station from hang-up is ensured by the watchdog, which restarts the station in case of failures. The success of recording the mark on the card is controlled by subsequent reading, only after that the station sends a mark signal. Also in the stations the recording of the numbers of the marked cards is realized. Implemented a system of passwords to avoid vandalism.

The box is available Gainta g1020bf with flanges. Sealing is carried out through the use of sealant.

Totally, the initial components for one base station and the consumables cost about 10$. The manual for assembly is on the wiki.

[Read more here](https://github.com/alexandervolikov/sportiduino/wiki/Base-station)

## Master station

The master station is simpler than the base station, consists of Arduino Nano, RFID module, LED and buzzer.
To the computer connection through USB. With the help of the master station you can perform a number of referee tasks:

1. Setting up the system before the start: setting the time at the stations, setting the numbers at the stations, and also setting up the station - setting the password for the master keys, the operating time.
2. Before and during the start: cleaning and delivery of cards to participants
3. After the start: reading the cards, creating a split table
4. After the finish: reading the log from the base station.

[Read more here](https://github.com/alexandervolikov/sportiduino/wiki/Master-station)

There is also a wireless station with the bluetooth module. 

## Data processing

Software development in IDE Processing is included in a separate repository:
https://github.com/alexandervolikov/sportiduinoProcessing
The functional is minimal.

We are developing a python module for the program [SportOrg](https://github.com/sportorg/pysport)
The module repository is also separate:
https://github.com/alexandervolikov/sportiduinoPython

***********
Available from:  https://github.com/alexandervolikov/sportiduino
 
License:         GNU GPLv3