mark station components

![](/Images/w01.jpg)

1. RFID module RC522. Pretty common.

2. Arduino nano. You can connect directly to the computer. There are variations on which chip is implemented Serial-USB port, more expensive FT232RL and cheaper CHG340, to connect the second you need to install the driver on the computer. I used a cheaper option

3. The pcp for soldering. The cost depends on the lot, the minimum number is 5. It is advisable to order boards for the gateway station simultaneously with the pcb for the mark stations.

4. Pins, LED and buzzer. 

5. The case is Gainta 1020BF. A good body made of ABS plastic, can be doped well for your purposes. 

6. USB-lace. Depends on the port used in Arduino Nano / I have a micro-usb.

First you need to solder arduino nano, although you can order with already soldered legs. Spread the legs and holes with flux, solder. ISP (2 * 3 connector) can not be soldered, but with it you can upload stations and do without an additional programmer in the form of Arduino Uno.

![](/Images/w02.jpg )

Further solder in the main board. Also in the main board we solder the 1 * 8 connector, the angle-bent LED, the buzzer and the resistors 0805 by 47 ohm (to the buzzer) and 150 ohms to the LED

![](/Images/w03.jpg)
 

Then solder the RFID module. We connect it via a USB wire to the computer.
 
![](/Images/w04.jpg)


Open the Arduino program on the computer, in the settings we specify the board type (Arduino Nano), select the required COM port and upload the RFID_ReadSet firmware onto the board.

We try on the case. The board I got a little unsuccessful, the cable barely damp, it would be nice to slightly shift the arduino nano and put it at an angle. But you can squeeze it. A small groove in the body is cut out for the wire. The LED is greased with epoxy.
 
![](/Images/w05.jpg)

After that the body is twisted and ready for use.

![](/Images/w06.jpg)
