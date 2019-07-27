A wireless master stantion is implemented in the system. Schematic diagram and board is in [Upverter](https://upverter.com/AlexanderVolikov/55b140a993222192/Sportiduino-BTstantion/)

![](/Master%20station/hardware/bluetooth/BTstation.png)

The circuit uses Arduino Pro mini, DS3231, RC522 and Bluettoth HC-05 cards. In addition, there is space for SPI-flash memory, in this repository it is not used and the space can be left blank, but if someone wants to implement additional functionality, you can use this memory. Similarly, the DS3231 clock is not used and their space can be left blank.

Gerber for ordering the board finds in the subfolder BTstantion. It looks like this:

![](/Master%20station/hardware/bluetooth/PCB_BTstation.PNG)

The principle work does not differ from a simple master stantion, but the data exchange is via Bluetooth, which allows you to implement the work with the system through Android devices. Assembly is carried out, mainly, mounted installation and does not represent complexity.
