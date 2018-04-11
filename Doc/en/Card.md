The card Ntag 213 / 215 / 216 can record up to 32/120/216 marks. In the form of labels, these cards cost 0.1 / 0.2 / 0.4 $, in the keychain form twice as expensive.

Important! For reliable operation with these cards, which have a small antenna area (like the keychains below) it is necessary to replace the inductors with the more powerful ones in the RFID RC522 module (described on the page "Base station assembly").

In addition, it is possible to use Mifare Classic S50 chips. These chips are also cheap, come bundled with the RC522 module. The memory of these chips is enough for 42 marks. They work a little slower than Ntag. Firmware for Mifare and Ntag are not compatible, so when using Mifare, at the mark and base stations need to upload other firmware, with Mifare in the title.

Memory of Mifare chips is used only for a quarter, since in their structure pages with a length of 16 bytes are realized. And with more dense recording there is a risk of data loss. The structure of the record is similar to Ntag.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/Chip.JPG)

The memory of the Ntag 213/215/216 cards is structurally divided into 45/135/231 pages, each containing 4 bytes. The first 4 pages contain UID and other service information. Also, the service information is contained in the last 5 pages. Thus, 36/126/222 pages remain for the record.

The structure of the record is presented below. 4-7 pages are reserved, the rest of the pages are for the mark. In the 4th page, the first two bytes contain the programmable card number. The third byte contains information about card type: 3 - Ntag213, 5 - Ntag 215, 6 - Ntag 216, it is automatically set. In the 4th byte, the firmware version. On the fifth page, the initialization time of the card in unixtime format (with it you can restore the first byte time in the marks.) Then two pages of the reserve. The recording of a mark on the station takes a separate page, the first byte of which records the station number, in the remaining three bytes - the lower bytes of the current time in unixtime (the total time is then restored using the initialization time recorded in the 5th page ).

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/Ntag2.JPG)

The cards for programming stations have a different structure. The station recognizes them by the number 255, recorded in the third byte of the 4th page. There are 5 different master cards: time, numbers, sleep, settings and dump. Below is their structure. After reading them, the station performs the function set in them and erases them in order to prevent accidental reuse.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/Master-Ntag2.JPG)