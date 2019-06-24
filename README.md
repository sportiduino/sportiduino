![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/logo.png)

#### Version 1.5.0

![](https://raw.githubusercontent.com/alexandervolikov/sportIDuino/master/Images/Sportiduino.JPG)

[Перейти на русский язык](https://github.com/alexandervolikov/sportiduino/blob/master/README.ru.md)

This project is devoted to the development of cheap electronic marking system for sports and tourist orientering. It is also possible to use it on rogaining events, adventure races, trails, wherever time fixing is required. Here is the hardware part of the marking system. The software is under construction, the links is placed [below](https://github.com/alexandervolikov/sportiduino#data-processing)

[Download latest release](https://github.com/alexandervolikov/sportiduino/releases)

[Manual](https://github.com/alexandervolikov/sportiduino/blob/master/Doc/en.md)

This project is open and free. Who is not afraid of difficulties, can try to do it by yourself following the instructions from the wiki. Сheapness of the components of the system can recoup the expended work ($ 10 for one base station, $ 0.2 per chip mark)

I warn that this development is a hobby, the author is not a professional in the field of electronics and programming. Therefore, no guarantees are given, different kinds of problems are possible during reproduction. Support is also not promised. So, act at your own risk. 

## Reporting Issues and Asking for Help

Issues and possible improvements can be posted to our [issue](https://github.com/alexandervolikov/sportiduino/issues). Please make sure you provide all relevant information about your problem or idea.

## Contributing

You can contribute by writing code. Programs for working with the system on a PC via Serial and on Androide via Bluetooth. The data transfer protocol and commands are described in the [manual](https://github.com/alexandervolikov/sportiduino/blob/master/Doc/en/MasterStation.md)  With pleasure we will add a link to your developments working with Sportidoino.

It also supports the creation of forks, pull rquests, developing any new ideas.

You can also help by translatiing the documentation. At this moment it is executed roughly.

# Parts of the system

## Cards

The system uses cards Mifare Classic S50. These come bundled with the RC522 module. The memory of these chips is enough for 42 marks.

<b>NTAG cards are not supported in new version 1.5.0!!!</b>

[Read more here](https://github.com/alexandervolikov/sportiduino/blob/master/Doc/en/Card.md)

## Base stations.

The main components of the station are the Atmega328P microcontroller and the MFRC522 module, which operates at a frequency of 13.56MHz. Clock DS3231SN. Powered by 3 AA batteries through the MCP1700T-33 stabilizer
The capacity of the kit of three alkaline AA batteries should be enough for a year of active use.

Totally, the initial components for one base station and the consumables cost about 10$.

[Read more here](https://github.com/alexandervolikov/sportiduino/blob/master/Doc/en/BaseStation.md)

## Master station

The master station is simpler than the base station, consists of Arduino Nano, RFID module, LED and buzzer.
To the computer connection through USB. With the help of the master station you can perform a number of referee tasks.

[Read more here](https://github.com/alexandervolikov/sportiduino/blob/master/Doc/en/MasterStation.md)

There is also a wireless station with the bluetooth module. 

## Data processing

### SportiduinoPQ

Cards and stations are tuned in the program [SportiduinoPQ](https://github.com/alexandervolikov/SportiduinoPQ)

The program is based on [a python module](https://github.com/alexandervolikov/sportiduinoPython) and also on the PyQt package for creating window applications

### SportOrg

Reading chips is implemented in the program [SportOrg](https://github.com/sportorg/pysport)

[Site with programm](http://sportorg.o-ural.ru/)

***********
Available from:  https://github.com/alexandervolikov/sportiduino
 
License:         GNU GPLv3
