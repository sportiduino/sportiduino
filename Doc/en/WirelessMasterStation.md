A wireless master station is implemented in the system. Schematic diagram and board is in [Upverter](https://upverter.com/AlexanderVolikov/55b140a993222192/Sportiduino-BTstantion/)

![](/hardware/MasterStation/bluetooth/BTstation.png)

The circuit uses Arduino Pro mini, DS3231, RC522 and Bluettoth HC-05 boards.
In addition, there is space for SPI-flash memory, in this repository it is not used and the space can be left blank, but if someone wants to implement additional functionality, you can use this memory.
Similarly, the DS3231 clock is not used and their space can be left blank.

Gerber for ordering the board is placed in the `hardware/MasterStation/bluetooth`.

![](/hardware/MasterStation/bluetooth/PCB_BTstation.PNG)

The principle work of the station does not differ from a regular master station,
but the data exchange is via Bluetooth, which allows you to implement the work with the system through Android devices.

