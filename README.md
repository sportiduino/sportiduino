[![Sportiduino version](https://img.shields.io/github/v/release/sportiduino/sportiduino)](https://github.com/sportiduino/sportiduino/releases)

![Sportiduino logo](/Images/logo.png?raw=true)

![](/Images/Sportiduino.JPG?raw=true)

[Перейти на русский язык](README.ru.md)

The inexpensive electronic timing system for orienteering, rogaining events, adventure races, trail running and etc.
This resository contains hardware and firmware parts of the timing system.
Links to data processing software are placed [below](#data-processing).

[Download latest release](https://github.com/sportiduino/sportiduino/releases/latest)

[Manual](/Doc/en.md)

This project is open and free. Anyone can reproduce it yourself. Just follow the instructions from the Manual.
The low cost of the components can be worth your efforts (about USD $10 for one base station and $0.2 per RFID tag).

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

**The current release version is 3.8.0**

**The current base station version is 3.8.0**

**The current master station version is 1.8.0**

[Changelog](/CHANGELOG.md)

Build the firmware of the base station with `#define HW_VERS 1` to install the firmware vX.6.X or greater on the PCB v1 or v2.

## Reporting Issues and Asking for Help

Issues and suggested improvements can be posted on [Issues](https://github.com/sportiduino/sportiduino/issues) page.
Please make sure you provide all relevant information about your problem or idea.

We also have our [Telegram chat](https://t.me/Sportiduino) where you can ask any questions about the system.

## Contributing

You can contribute by writing code.
We welcome software for working with the system on a PC via USB and on Android via Bluetooth or NFC.
The data transfer protocol and commands are described in the [Manual](/Doc/en/MasterStation.md).
With pleasure we will add a link to your developments working with Sportiduino.

Pull requests, creation of forks, developing any new ideas are welcome.

You can also help by improving documentation and its translation.

# Parts of the system

## Cards

The system uses cards NTAG213/215/216.
As stickers on Chinese web marketplaces they cost about $0.1, 0.2 and 0.4, respectively.
As key fobs the cost is doubled.
Memory of these cards can keep 32, 120 and 216 marks, respectively.

Also it is possible to use Mifare Classic 1K and 4K cards.
Cards 1K are also cheap and come bundled with the RC522 module.
The memory of these chips is enough for 42 marks. They work a little slower than NTAG.

The system automatically detects the type of used cards.

[Read more](/Doc/en/Card.md)

## Base stations

The main components of the station are the ATmega328P microcontroller and the MFRC522 module,
which operates at a frequency of 13.56 MHz, real-time clock DS3231SN.
All powered by 3 AA batteries through the MCP1700T-33 stabilizer.
The capacity of the kit of three alkaline AA batteries should be enough for a year of active use.
Tested at ambient temperatures from -20 to +50 degrees Celcius.

Totally, the initial components for one base station and the consumables cost about $10 (in 2019).

[Read more](/Doc/en/BaseStation.md)

## Master station

With the master station you can read and write tags and configure base stations.
It is simpler than the base station.
It consists of Arduino Nano, RFID module, LED and buzzer.
It connects with a PC via USB. 

[Read more](/Doc/en/MasterStation.md)

There is also a wireless station with the Bluetooth module. 

## Data processing

### SportiduinoPQ

Cards and stations are configured by [SportiduinoPQ](https://github.com/sportiduino/SportiduinoPQ) program.

The program is written on Python, based on PyQt5 and [SportiduinoPython module](https://github.com/sportiduino/sportiduinoPython).

### SportOrg

Reading cards is implemented in the [SportOrg](https://github.com/sportorg/pysport) program.

***********

This system and its variants have been used in Russia at a number of events
up to approx. 1400 participants and approx. 70 check points.

***********

Available from:  https://github.com/sportiduino/sportiduino

License:         GNU GPLv3
