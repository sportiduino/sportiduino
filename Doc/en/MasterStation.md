# Master Station

The Master station can be made in various boxes.

![](/Images/MasterStation1.jpg?raw=true "Master station in the box made on a 3D printer")

or

![](/Images/w06.jpg?raw=true "Master station in the G1020BF box")

The circuit and board are available in Upverter ([variant 1](https://upverter.com/AlexanderVolikov/3fc0efdb2586988d/Sportiduino-reading-stantion/) or [variant 2](https://upverter.com/design/syakimov/4f7ec0e2d3b9c4e9/sportiduino-master-station/)).

![](/hardware/MasterStation/usb/Scheme.PNG?raw=true "Scheme")

All components of the circuit can be placed on a PCB or soldered to the Arduino by a ordinary wires.
The electronics of the master station can be placed in the Gainta G1020BF box or in a specially designed box that can be printed on a 3D printer.
[More about the assembly](/Doc/en/MasterStationAssembly.md).

The station is connected to the computer using a USB port.
The Arduino Nano already has a built-in Serial-to-USB converter, so no additional converters are needed.
The station will be displayed in the connected devices as a COM port or /dev/ttyUSBx in Linux.

### Serial data protocol

The transmission of information between PC and Master Station occurs sequentially by sending up to 32-byte packets.

| | Value | Byte address in the packet |
| --- | --- | --- |
| Start | 0xFE | 0 |
| Function | | 1 |
| The length of the data or packet number + 0x1E | | 2 |
| Data array | | 3..x (x<31) |
| Checksum | x+1 |

The packet starts with byte 0xFE.

The command is specified by the byte with address 1.
Then in byte 2 the data length follows, if the transmitted packet is the last packet or its number + 30, if multipacket transmission is in progress.

Next comes an array of transmitted data up to 28 bytes.

At the end of the packet contains a checksum: the remainder of dividing by 256 the sum of bytes 1..x.
Commands pass only with the correct value of the checksum and start byte.

#### List of commands sent to the station

| Code | Command | Length | Array of data bytes (offset) |
| --- | --- | --- | --- |
| 0x41 | Write the time to the master card | 6 | year-2000 (0), month (1), day (2), hour (3), minute (4), second (5)
| 0x42 | Write the station number to the master card | 1 | number (0)
| 0x44 | Initialize the card | 14 | card number (0-1), current time (2-5), information in 6-7 pages (6-13)
| 0x45 | Write 6,7 pages | 8 | 6-7 pages data (0-7)
| 0x46 | Ask version of the master station | 0 |
| 0x47 | Create Backup master card | 0 |
| 0x48 | Read the Backup master card | 0 |
| 0x4A | Write settings to the master station | 1 | antenna gain (0)
| 0x4B | Read the card | 0 |
| 0x4C | Read raw data from the card | 0 |
| 0x4D | Read settings of the master station | 0 |
| 0x4E | Write a Sleep master card | 6 | year-2000 (0), month (1), day (2), hour (3), minute (4), second (5)
| 0x4F | Set the current password | 3 | password (0-2)
| 0x50 | Create State master card | 0 |
| 0x51 | Read card type | 0 |
| 0x58 | Beep error (three short beeps) | 0 |
| 0x59 | Beep OK (long beep) | 0 |
| 0x5A | Create Config master card | 6 | base station configuration

#### List of station responses

| Code | Command | Length | Array of data bytes (offset) |
| --- | --- | --- | --- |
| 0x61 | Data from Backup master card | N packages | (the first packet is the station number (0)), the number of the marked card 1 (1-2), the number of the marked card 2 (3-4), etc.
| 0x63 | Card punches data | N packages | (the first package - the card number (0-1), information in the 6-7 pages of the card (2-9),) number CP0 (10), time CP0 (11-14), number CP1 (15), time CP1 (16-19), etc.
| 0x65 | Raw data from the card | N packets | page number 4 (0), page 4 bytes (1-4), page number 5 (5), page bytes 5 (6-9), etc.
| 0x66 | Version of the master station firmware | 3 | version (0-2)
| 0x67 | Setting of the master station | 1 | antenna gain (0)
| 0x70 | Card type | 1 | card type(0)
| 0x78 | Error | 2 | error code (0), card type (1)
| 0x79 | OK | 1 | card type (0)

#### Error Codes

| Code | Error |
| --- | --- |
| 0x01 | Serial communication error
| 0x02 | Card writing error
| 0x03 | Card reading error
| 0x04 | EEPROM reading error
| 0x05 | Card not found
| 0x06 | Unknown command

### SPORTident emulation

Firmware of Master station v1.8.0 or greater has support of reading card by the SPORTident data transfer protocol.
All RFID tags are read as SI-Card (192 punch mode).
Communication tested with SPORTident Reader, Rogain Manager, SportOrg and WinOrient 2.0a 2014 and 2020 software.
It doesn't work with WinOrient 2011 yet.

