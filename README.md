![](/Images/logo.png?raw=true)

![](/Images/Sportiduino.JPG?raw=true)

[Перейти на русский язык](README.ru.md)

This project is devoted to the development of the electronic timing system for orienteering.
It is also possible to use one on rogaining events, adventure races, trails, wherever time keeping is required.
Here are hardware and firmware parts of the timing system.
Links to data processing software are placed [below](#data-processing).

[Download latest release](https://github.com/sportiduino/sportiduino/releases/latest)

[Manual](/Doc/en.md)

This project is open and free. Who is not afraid of difficulties, can try to do it by yourself following the instructions from the wiki.
The low cost of the system components can pay back work spent (about $10 for one base station and $0.2 per chip mark)

This development is a hobby.
No guarantees are given, different kinds of problems are possible during reproduction.
Support is also not guaranteed. So, act at your own risk. 

## Version

The version consists of three numbers. The first number indicates the version of the hardware.
If any changes are made to the circuit or to the PCB, this number is incremented by 1.

The second and third numbers indicates the version of the firmware.
If any new function is added to the firmware, the second number is incremented by 1.
If the firmware just fixes bugs, the third number in the version is incremented by 1.
When a new version of the firmware is released with new functions, the third number is reset to 0.

The base station and the master station has its own version. The release version is the largest of these two numbers.

**The current release version is 2.6.3**

**The current base station version is 2.6.3**

**The current master station version is 1.6.3**

Sorry, assembly instructions for hardware version > 1 are not translated to English yet.
You can translate Russian manual by yourself.

[View changelog](CHANGELOG.md)

Build the firmware of the base station with `#define HW_VERS 1` to install the firmware v6.x on the hardware v1.

## Reporting Issues and Asking for Help

Issues and possible improvements can be posted on [Issues](https://github.com/sportiduino/sportiduino/issues) page.
Please make sure you provide all relevant information about your problem or idea.

## Contributing

You can contribute by writing code.
We welcome software for working with the system on a PC via Serial and on Android via Bluetooth or NFC.
The data transfer protocol and commands are described in the [Manual](/Doc/en/MasterStation.md).
With pleasure we will add a link to your developments working with Sportidoino.

It also supports the creation of forks, pull rquests, developing any new ideas.

You can also help by translatiing the documentation. At this moment it is executed roughly.

# Parts of the system

## Cards

The system uses cards Ntag 213 / 215 / 216. In the form of labels these cards cost $0.1, 0.2, 0.4, respectively.
In the form of a key fob cost is twice as expensive.
The memory of these cards is enough for 32, 120 and 216 marks, respectively.

In addition, it is possible to use Mifare Classic 1K cards.
These cards are also cheap and come bundled with the RC522 module.
The memory of these chips is enough for 42 marks. They work a little slower than Ntag.

The system automatically detects the type of used cards.

[Read more here](/Doc/en/Card.md)

## Base stations

The main components of the station are the Atmega328P microcontroller and the MFRC522 module, which operates at a frequency of 13.56MHz.
Clock DS3231SN.
All powered by 3 AA batteries through the MCP1700T-33 stabilizer.
The capacity of the kit of three alkaline AA batteries should be enough for a year of active use.

Totally, the initial components for one base station and the consumables cost about $10.

[Read more here](/Doc/en/BaseStation.md)

## Master station

The master station is simpler than the base station.
It consists of Arduino Nano, RFID module, LED and buzzer.
To the computer connection through USB. Using the master station you can read and write cards and configure base stations.

[Read more here](/Doc/en/MasterStation.md)

There is also a wireless station with the bluetooth module. 

## Data processing

### SportiduinoPQ

Cards and stations are configured by [SportiduinoPQ](https://github.com/sportiduino/SportiduinoPQ) program.

The program is based on [the special python module](https://github.com/sportiduino/sportiduinoPython) and also on the PyQt5 package for creating window applications.

### SportOrg

Reading cards is implemented in the [SportOrg](https://github.com/sportorg/pysport) program.


***********
Available from:  https://github.com/sportiduino/sportiduino

License:         GNU GPLv3
