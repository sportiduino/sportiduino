As a master station, you can use a base station, reupload it, or make a simpler one based on the Arduino Nano, connecting to a LED, buzzer and an RFID module. The station connected with a computer, powered via USB, sending info by Serial. The circuit and board are available in [Upverter](https://upverter.com/AlexanderVolikov/3fc0efdb2586988d/Sportiduino-reading-stantion/)

With the help of the master station you can perform a number of referee tasks:

1. Setting up the system before the start: setting the time at the stations, setting the numbers at the stations. Setting a password and settings
2. Before and during the start: cleaning and delivery of cards to participants
3. After the start: reading the cards, creating a split table
4. After the finish: reading the log from the base station.

![](/harware/MasterStation/Scheme.PNG)

There are gerber files in the folder, you can order a pcb and solder, so it will be more reliable, but you can do everything with wires and double-sided scotch, the benefit of components is quite small.

[Read more about the assembly here](/Doc/en/MasterStationAssembly.md)

The station is connected to the computer using a USB connection. The Arduino nano already has a built-in Serial-to-USB converter, so no additional converters are needed. The station will be displayed in the connected devices as a COM port.

The transmission and retrieval of information occurs sequentially by sending up to 32-byte packets.

|| value | the byte address in the packet |
| --- | --- | --- |
| start | 0xFE | 0 |
| function || 1 |
| the length of the packet, if the packet is not complete, (package number + 0x1E) || 2 |
| data array || 3--x (x<31) |
| checksum | x+1 |

The packet starts with start byte 0xFE

The command is specified by the byte with address 1. Then in byte 2, the packet length follows if the transmitted packet is the last packet or its number is + 30 if multipacket transmission is in progress. The fact that in the byte 2 value is less than 30 confirms the end of the data transfer.

Next comes an array of transmitted data up to 28 bytes.

At the end of the packet contains a checksum (the remainder of dividing the sum of bytes extept start byte of the packet by 256). Commands pass only with the correct value of the checksum and start byte.

List of commands sent to the station:

| function | Commands | length | Array of data bytes (in the address bracket in the array) |
| --- | --- | --- | --- |
| 0x41 | Write the time for the master card | 6 | year-2000 (0) month (1) day (2) hour (3) minute (4) second (5)
| 0x42 | Write the station number to the master card | 1 | number (0)
| 0x43 | Write the password and settings for the master card | 7 | new password (0-2) old password (3-5) settings (6)
| 0x44 | Initialize the card | 14 | card number (0-1), current time (2-5), information in 6-7 pages (6-13)
| 0x45 | Pass parameters 6,7 page | 8 | information in 6-7 pages (0-7)
| 0x46 | Ask version of the master station | 0 |
| 0x48 | Read the log rent | 0 |
| 0x47 | Write a tenant of the log | 0 |
| 0x4B | Read the card | 0 |
| 0x4C | Read raw data from the card | 0 |
| 0x4E | Write a sleep master | 0 |
| 0x58 | Three short signals (error) | 0 |
| 0x59 | Long signal (OK) | 0 |

List of responses transmitted by the station:

| function | Commands | length | Array of data bytes (in the address bracket in the array) |
| --- | --- | --- | --- |
| 0x61 | Full log transmition|many packages | (the first packet is the station number (0)), the number of the marked card 1 (1-2), the number of the marked card 2 (3-4), ..., zero (25)
| 0x63 | Data transfer from the card mark | a lot of packages | (the first package - the card number (0-1), information in the 6-7 pages of the card (2-9),) number cp0 (10), time cp0 (11-14 ), the number of cp1 (15), the time of cp1 (16-19), ...
| 0x65 | Send raw data from the card | many packets | page number 4 (0), page 4 bytes (1-4), page number 5 (5), page bytes 5 (6-9), ...
| 0x66 | version of the master station harware | 1 | version (0)
| 0x69 | work regime of master station | 1 | regime (0)
| 0x78 | Error | 1 | error code (0)
| 0x79 | OK | 0 |

Error Codes:

| N | error |
| --- | --- |
| 0x01 | communication error via com port |
| 0x02 | card writing error |
| 0x03 | card reading error |
| 0x04 | EEPROM reading error |

