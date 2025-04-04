﻿[![Sportiduino version](https://img.shields.io/github/v/release/sportiduino/sportiduino)](https://github.com/sportiduino/sportiduino/releases)

![Sportiduino logo](/img/logo.png?raw=true)

![](/img/Sportiduino.JPG?raw=true)

[Перейти на русский язык](README.ru.md)

The inexpensive electronic timing system for orienteering, rogaining events, adventure races, trail running and etc.
This resository contains hardware and firmware parts of the timing system.
Links to data processing software are placed [below](#software).

[Download latest release](https://github.com/sportiduino/sportiduino/releases/latest)

[Manual](/docs/en.md)

This project is open and free. Anyone can reproduce it yourself. Just follow the instructions from the Manual.

This development is a hobby.
No guarantees are given, various kinds of problems are possible during reproduction.
Support is also not guaranteed. So, act at your own risk. 

## Version

The version consists of three numbers. The first number indicates the version of the hardware.
If any changes are made to the circuit or to the PCB, this number is incremented by 1.

The second and third numbers indicates the version of the firmware.
If any new function is added to the firmware, the second number is incremented by 1.
If the firmware just fixes bugs, the third number in the version is incremented by 1.
When a new version of the firmware is released with new functions, the third number is reset to 0.

The base station and the master station have their own versions. The release version is the largest of these two numbers.

**The current base station version is 3.10.0**

**The current master station version is 1.9.0**

[Changelog](/CHANGELOG.md)

Build the firmware of the base station with `#define HW_VERS 1` to install the firmware vX.6.X or greater on the PCB v1 or v2.

## Reporting Issues and Asking for Help

Issues and suggested improvements can be posted on [Issues](https://github.com/sportiduino/sportiduino/issues) page.
Please make sure you provide all relevant information about your problem or idea.

We also have our [Telegram chat](https://t.me/Sportiduino) where you can ask any questions about the system.

## Contributing

You can contribute by writing code.
We welcome software for working with the system on a PC via USB and on Android via Bluetooth or NFC.
The data transfer protocol and commands are described in the [Manual](/docs/en/MasterStation.md).
With pleasure we will add a link to your developments working with Sportiduino.

Pull requests, creation of forks, developing any new ideas are welcome.

You can also help by improving documentation and its translation.

# Parts of the system

## Cards

The system uses cards NTAG213/215/216.
Memory of these cards can keep 32, 120 and 216 marks, respectively.

Also it is possible to use Mifare Classic 1K and 4K cards.
Cards 1K are also cheap and come bundled with the RC522 module.
The memory of these chips is enough for 42 marks. They work a little slower than NTAG.

The system automatically detects the type of used cards.

[Read more](/docs/en/Cards.md)

## Base stations

The main components of the station are the ATmega328P-AU microcontroller and the MFRC522 module,
which operates at a frequency of 13.56 MHz, real-time clock DS3231SN.
All powered by 3 AA batteries through the MCP1700T-33 stabilizer.
The capacity of the kit of three alkaline AA batteries should be enough for a year of active use.
Tested at ambient temperatures from -20 to +50 degrees Celcius.

[Read more](/docs/en/BaseStation.md)

## Master station

With the master station you can read and write tags and configure base stations.
It is simpler than the base station.
It consists of Arduino Nano, RFID module, LED and buzzer.
It connects with a PC via USB. 

[Read more](/docs/en/MasterStation.md)

There is also a wireless station with the Bluetooth module. 

## Software

### Prepare cards and stations

#### [SportiduinoPQ](https://github.com/sportiduino/SportiduinoPQ)

PC software (Windows or Linux).

It is written on Python, based on PyQt5 and [SportiduinoPython module](https://github.com/sportiduino/sportiduinoPython).

#### [SportiduinoApp](https://github.com/sportiduino/sportiduinoapp)

Android application for smartphones with NFC.

### For event management

Reading cards is implemented in [SportOrg](https://github.com/sportorg/pysport) and [SportOrgPlus](https://github.com/sembruk/sportorg-plus).

Also you can use software for SPORTident (MeOS, WinOrient, Rogain Manager etc).

***********

This system and its variants have been used in Russia at a number of events
up to approx. 1400 participants and approx. 70 check points.

***********

Available from:  https://github.com/sportiduino/sportiduino

License:         [GNU GPLv3](/LICENSE)

