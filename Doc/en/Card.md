## Cards

The system supports these cards (RFID tags):

- [Mifare Ultralight C](http://www.nxp.com/documents/data_sheet/MF0ICU2.pdf) for 36 punches
- [Mifare Classic 1K](https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf) for 42 punches
- [Mifare Classic 4K](https://www.nxp.com/docs/en/data-sheet/MF1S70YYX_V1.pdf) for 90 punches 
- [NTAG213/215/216](https://www.nxp.com/docs/en/data-sheet/NTAG213_215_216.pdf) for 32/120/216 punches

Card type is detected automatically.
One station can work with various cards simultaneously
without any changes in firmware or hardware.

One is advised to use NTAG213/215/216.
They cost USD $0.1, 0.2, 0.4 as stickers and twice as much as key fobs.

![](/Images/chip.jpg?raw=true "Key fobs tags")

**Important!**
For reliable operation with these cards which have a small antenna area
it is necessary to replace the inductors with the more powerful ones in the RFID RC522 module (see [Base station assembly](/Doc/en/BaseStationAssembly.md)).

Mifare Classic 1K cards come bundled with the RC522 module.
The memory of these cards is enough for 42 marks. They work a little slower than NTAG. 
Only a quarter of the Mifare cards memory is used due to page size of 16 bytes (compared with 4 bytes for NTAG).
With more dense recording there is a risk of data loss.
The structure of the record is similar to NTAG.

The memory of the NTAG213/215/216 cards is structurally divided into 45/135/231 pages, each containing 4 bytes.
The first 4 pages contain UID and other service information.
Also the service information is contained in the last 5 pages.
Thus, 36/126/222 pages are available.

### The structure of the records

Page numbers start from 0.
Pages 4-7 are reserved, the rest of the pages are for the punches.
In the 4th page, the first two bytes contain the programmable card number.
The third byte contains information about card type: 3 - NTAG213, 5 - NTAG215, 6 - NTAG216, it is automatically set.
In the 4th byte, the station firmware version.
On the 5th page, the initialization time of the card in Unixtime UTC format.
Then two pages are reserved.
Each station punch takes a separate page,
the first byte of which contains the station number, in the remaining three bytes - the lower bytes of the current time in Unixtime UTC.
Time is then restored using the initialization time recorded in the 5th page.

![](/Images/Ntag2.JPG)

Master cards which set up the stations have a different structure.
The station recognizes them by the number 255, recorded in the 3rd byte of the 4th page.
There are 6 different master cards: Time, Number, Sleep, Config, Backup and State.
Their structure is below.

![](/Images/master-card.png?raw=true)

