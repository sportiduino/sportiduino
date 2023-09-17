# Base Station Assembly

This instruction for assembling Base station v3.

### Base station components

![](/img/s01.jpg?raw=true)

1. RFID module RC522.
2. PCB. Can be ordered via [EasyEDA](https://easyeda.com/order) or [JLCPCB](https://jlcpcb.com/).
Gerber files are placed in the folder `hardware/BaseStation/prod/v3`.
*It is not in the photo PCB v1.*
3. Chips
   - Microcontroller ATmega328P-AU.
   - DS3231SN clock.
   - Memory 24LC256T-I/SN (optional, not in the photo).
   - Voltage regulator MCP1700T-33.
   - Transistor BSS138 SOT-23.
4. Passive components in the 0805 format.
   - Resistor 100 ohm, 1 pc.
   - Resistor 3.3 kOhm, 2 pcs.
   - Resistor 10 kOhm, 2 pcs.
   - Resistor 33 kOhm, 2 pcs.
   - Resistor 68 kOhm, 1 pc.
   - Resistor 270 kOhm, 1 pc.
   - Capacitor 0.1 uF, 7 pcs.
   - Capacitor 4.7 uF, 2 pcs.
   - Inductor 2.2 uH, max current > 500 mA (e.g. LQH32MN2R2K) 2 pcs, optional.
5. Through-hole components
   - PLS-8 pins. Come with RFID board.
   - Connector BH-10 (aka DS1013-10S or IDC-10MS). It can be replaced by any male connector 2x5 pins 2.54 mm.
   - LED 3 mm. Any color for your choice.
   - Passive buzzer 12085 16 ohm, TR1205Y or HCM1203A.
   - Reed switch normally open 2x14 mm (optional, not in the photo).
   - Connector 1x2 pins 2.54 mm for battery (optional, not in the photo).
6. Battery box 3xAA or 3xAAA.
7. The Gainta G1020BF case.

Equipment which is useful for assembly is given [on a separate page](Equipment.md)

### RFID module preparing

Remove LED D1, resistors R1 and R2 to reduce module power consumption.

Replace inductors L1 and L2 by similar 2.2 uH, but with a large operating current to increase the antenna power (e.g. Murata LQH32MN2R2K).

![](/img/s12_1.JPG?raw=true)

### Soldering the main board

![](/hardware/BaseStation/prod/v3/sportiduino-base-v3-scheme.png?raw=true "Scheme")

![](/img/pcbv3-build-instructions.png?raw=true "PCB with instructions")

Grease flux all places of future soldering.
Next solder chips.
After chips solder the transistor, voltage stabilizer, resistors and capacitors.
Resistor R8 and capacitor C8 can not be soldered, since they are not used now and reserved for future versions.
Contact pads JP1 and JP2 are closed with a drop of solder or jumpers, as shown in the circuit scheme (pins 1 and 2).
At the end solder buzzer, RFID connector and battery box (or connector 1x2 pins).
Solder a LED on the back of the board.
Instead of the output LED, you can solder the SMD LED from the RFID module.
The RC-522 module is not soldered yet.

**Warning!** Do not confuse the polarity of the LED and the buzzer when soldering.

### Programmer

To upload the bootloader to the microcontroller, you need a programmer.
If you do not have a programmer then it can be made from Arduino Nano.

#### Arduino Nano based programmer 

To make it, you will need a flat cable for 10 wires 25 cm long and an IDC-10F connector (aka DS1016-10).
Instead of this connector, you can use any other type of 2x5 pin 2.54 mm female connector.

A connector is installed at the end of the cable.
Solder the other end of the cable to the Arduino Nano board in accordance with the scheme below.
We do not yet install a 10 μF 16V capacitor.

![](/img/ArduinoNanoIspScheme.png?raw=true "Arduino Nano programmer scheme")

Connect the Arduino Nano to the PC via USB and launch the Arduino IDE.
Open the ArduinoISP sketch from Arduino examples (File -> Examples -> ArduinoISP).
In the `Tools` menu, select the `Arduino Nano` board, the `ATmega328P (Old Bootloader)` processor, and the corresponding port.

![](/img/ArduinoIspScr1.jpg?raw=true)

Perform Sketch -> Upload.
After that, solder the 10 uF 16V capacitor between GND and RST as in the scheme above.
If the capacitor is not soldered, an error message will constantly appear during programming:
`avrdude: stk500_getsync() attempt 1 of 10: not in sync: resp=0x15`.
The programmer is ready.

![](/img/ArduinoNanoISP.jpg?raw=true "Arduino Nano Programmer")

### Serial programmer

To upload the main firmware to the microcontroller, a USB-UART converter is required.
The converter must have a DTR pin.
To connect it to the station board, you also need a 10-wire flat cable and IDC-10F connector.

![](/img/SerialProgrammer.png?raw=true "Serial programmer scheme")

If the converter will also be used to configure the station and to read its state
then necessary to provide the possibility of disconnecting the DTR pin.

![](/img/ProgrammerWire.jpg?raw=true "Serial programmer")

### Arduino IDE Setting

In the File->Settings menu change the location of the sketch folder to `path/to/project/firmware`.

Next, create a description of Sportiduino board.
Open the file `path/to/Arduino/hardware/arduino/avr/boards.txt`
and add file `path/to/project/firmware/boards.txt` contents to the end of this file.

After that copy files \*.hex from the folder `path/to/project/firmware/Optiboot` to the folder `path/to/Arduino/hardware/arduino/avr/bootloaders/optiboot`.

Restart the Arduino IDE. In the Tools->Board menu our Sportiduino board should appear.

### Bootloader and firmware upload

In Arduino IDE, open the BaseStation sketch (File->Sketchbook->BaseStation).
In the Tools->Board menu select `Sportiduino`.
Next in the Tools->Programmer menu select `Arduino as ISP`.

![](/img/BaseStationProgConf.jpg?raw=true)

Connect programmer to the board. Power on board.

![](/img/ProgrammerConnect.jpg?raw=true "Programmer connection")

Execute Tools->Burn Bootloader.
If an error message appears in the log then check the assembly of the programmer and its connection to the board, check the assembly of the board itself.
If everything went well then turn off the programmer.

Connect the serial programmer.

![](/img/SerialProgConnect.jpg?raw=true "Serial programmer connection")

Execute Sketch->Upload.
After the upload is complete the station should beeps three times and beeps one long time after 3 seconds.
If this does not happen then check the soldering of the board.

### Soldering RFID to the board and testing 

After uploading the firmware to the microcontroller solder the RFID module and the reed switch (optional) to the board.

![](/img/BaseStationRfidSolded.jpg?raw=true "PCB with RFID and reed switch") 

Then test the operation of the station.
Create master cards of the station number, time, and several regular cards.
After power-up, the station will give three short beeps, which indicates that the clock is not set.
Then after a few seconds there will be a long beep, indicating that the power is normal and the program has entered the work loop.
By default, the station operates in Wait mode and checks for the presence of a chip in the working area once per second.
Bring the Station number master card to the station.
The station will record a new number and is now able to respond to regular cards.
Next, configure the clock.
Bring the Time master card and after beep bring the second regular card.
Read it at the master station and make sure that base station clock works correctly.

![](/img/BaseStationTestAssembly.jpg?raw=true "Station testing")

If the station passed the test, then the board can be washed off the flux residues, for example, in a bath with isopropyl alcohol.
If the board does not work then check first the soldering of chips and the connection to the RFID module.

![](/img/BaseStationClean.jpg?raw=true "Готовая плата после отмывки")

### Installation in the case

In the case drill a 3 mm diameter hole for a LED.
The hole coordinates are shown in the figure below.
To prevent moisture from entering through this hole,
it must be sealed with adhesive tape on the reverse side, and on the other hand, filled with hot or epoxy glue.

![](/img/BaseStationHole.jpg?raw=true "Hole coordinates")

Next, glue the battery box to the cover of the case (optional).

After that install the board so that the LED falls into the center of the drilled hole.

Fix the board with hot glue.
*In the photo show the PCB v2.*

![](/img/BaseStationPcbGlue.jpg?raw=true "Fix the board with hot glue")

![](/img/BaseStationPcbInBox.jpg?raw=true "PCB in case")

After that fill the compound (optional).
Or blur the joint of the housing with a silicone sealant, or glue the cover to the case.

*In the photos below show the PCB v1.*

![](/img/s26.jpg?raw=true)

At a station measure 30 ml of compound

![](/img/s27.jpg?raw=true)

and 1 ml of hardener. Mix the hardener with the compound.

![](/img/s28.jpg?raw=true)

![](/img/s29.jpg?raw=true)

And fill the board.
Silicone should fill all contacts.
There are only programmer connector and buzzer.

![](/img/s30.jpg?raw=true)

![](/img/s31.jpg?raw=true)

In a day the compound solidifies completely.
In this case, all the details can be seen, in which case the compound can easily be removed for repairing.

![](/img/s32.jpg?raw=true)

The battery box is smeared with petroleum jelly or other thick hydrophobic thick oil and insert batteries.
 
![](/img/s34.jpg?raw=true)

Close the case with the cover.
Parts lie tight, you may need a little effort to close the case.

![](/img/s35.jpg?raw=true)

![](/img/s36.jpg?raw=true)

Print stickers form the folder `hardware/BaseStation/2d`.
Glue the sticker on top of the station.

The station is ready!

![](/img/BaseStation1.jpg)

![](/img/BaseStation2.jpg)

