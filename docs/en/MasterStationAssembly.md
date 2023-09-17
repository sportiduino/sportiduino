# Master Station Assembly

### Master station components

![](/img/w01.jpg?raw=true)

1. RFID module RC522.
2. Arduino Nano.
You can connect directly to the computer.
There are variations on which chip USB-Serial is implemented: more expensive FT232RL or cheaper CHG340.
To connect the second you need to install the driver on Windows.
3. The PCB for soldering ([variant 1](https://upverter.com/AlexanderVolikov/3fc0efdb2586988d/Sportiduino-reading-stantion/) or
[variant 2](https://upverter.com/design/syakimov/4f7ec0e2d3b9c4e9/sportiduino-master-station/)).
You can order the manufacture of PCB in China (e.g. [JLCPCB](https://jlcpcb.com/)).
The cost depends on the lot, the minimum number is 5.
It is advisable to order boards for the master station simultaneously with the PCB for the base stations.
4. Pins, LED and buzzer. 
5. The case is Gainta 1020BF or 1015. Sold in Russia. 
6. USB-lace.
USB type depends on the port used in Arduino Nano (micro- or mini-USB).

### Arduino Nano preparation

First solder Arduino Nano.
ISP (2x3 connector) can not be soldered.

![](/img/w02.jpg?raw=true)

#### Arduino Nano preparation for SPORTident emulation

Remove capacitor at line DTR-Reset from Arduino Nano board.
It is located next to the USB controller chip.
After that burn fixed Optiboot bootloader (optiboot8_atmega328_38400_without_reset.hex)
using the ISP programmer.

### Station with PCB variant 1 and the G1020BF case

Solder Arduino Nano in the main board.
Also in the main board we solder a 1x8 connector, a angle-bent LED,
a buzzer and resistors 0805 47 ohm (to the buzzer) and 150 ohm (to the LED).

![](/img/w03.jpg?raw=true)

Solder the RFID module.
Then connect the master station via a USB wire to the computer.
 
![](/img/w04.jpg?raw=true)

Open the Arduino IDE program on the computer (see [Arduino IDE Setting](BaseStationAssembly.md#Arduino-IDE-Setting)).
In the settings specify the board type `Arduino Nano`, processor `ATmega328P (Old Bootloader)`
(`ATmega328P (without reset)` for SPORTident emulation).
Select the appropriate serial port and upload MasterStation sketch onto the board.

For USB cable cut off a s small groove in the case.
The LED is greased with epoxy glue.
 
![](/img/w05.jpg?raw=true)

After that close the case.
The master station is ready for use.

![](/img/w06.jpg?raw=true)

### Station without PCB

The master station can be assembled without a PCB.
The LED, buzzer and RFID module are soldered by wiring to the Arduino Nano according to the scheme.

![](/hardware/MasterStation/usb/sportiduino-master-scheme.png?raw=true)

The station case is designed for 3D-printing.
Files for 3D-printer placed in the folder `hardware/MasterStation/3d/print`.

![](/img/MasterStationBoxTop.jpg?raw=true)

![](/img/MasterStationBoxBot.jpg?raw=true)

![](/img/MasterStationInBox.jpg?raw=true)

