# Cards

The system supports these cards (RFID tags):

- [Mifare Ultralight C](http://www.nxp.com/documents/data_sheet/MF0ICU2.pdf) for 36 punches
- [Mifare Classic 1K](https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf) for 42 punches
- [Mifare Classic 4K](https://www.nxp.com/docs/en/data-sheet/MF1S70YYX_V1.pdf) for 90 punches 
- [NTAG213/215/216](https://www.nxp.com/docs/en/data-sheet/NTAG213_215_216.pdf) for 32/120/216 punches

Card type is detected automatically.
One station can work with various cards simultaneously
without any changes in firmware or hardware.

One is advised to use NTAG213/215/216.

![](/img/chip.jpg?raw=true "Key fobs tags")

**Important!**
For reliable operation with these cards which have a small antenna area
it is necessary to replace the inductors with the more powerful ones in the RFID RC522 module (see [Base station assembly](BaseStationAssembly.md)).

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
In the 4th byte, the station firmware version (e.g. 8 for v3.8.0, 9 for v3.9.1).
On the 5th page, the initialization time of the card in Unixtime UTC format.
Then two pages are reserved.
Each station punch takes a separate page,
the first byte of which contains the station number, in the remaining three bytes - the lower bytes of the current time in Unixtime UTC.
Time is then restored using the initialization time recorded in the 5th page.

<img src="/img/ntag-en.png" width="690">

Master cards which set up the stations have a different structure.
The station recognizes them by the number 255, recorded in the 3rd byte of the 4th page.
There are 6 different master cards: Time, Number, Sleep, Config, Backup and State.
Their structure is below.

![](/img/master-card.png?raw=true)

#### Time Master Card

Writes the date and time to the station (in real time clock).

This card is one-shot.
After reading information from the card, page 4 is erased on it
to prevent accidental reuse.

#### Number Master Card

Writes the number to the station if it is greater than 0.

This card is one-shot.

#### Sleep Master Card

Writes the date and time of waking up to the station and puts it into Sleep mode.
When waking up by timer, the station goes into active mode.

This card is reusable.

#### Config Master Card

Writes the number (if > 0), settings byte, antenna gain (if ≥ 2 and ≤ 7) and new password to the station.

This card is reusable.

#### Backup Master Card

The station writes a log of punches to this card.
If the station saves a log with time stamps, then a log for the last 12 days is recorded in ascending order of card numbers.
Since the capacity of the card may not be enough to record the entire time-stamped log.
Then for a complete reading of the log, it is necessary to attach the card to the station several times with an intermediate reading of the contents of the card.

This card is reusable.

#### State Master Card

The station writes to this card:
- firmware version number (3 bytes) to page 4;
- station number, settings byte, antenna gain in page 5;
- battery voltage (mV / 20, 1 byte) and mode (1 byte) in page 6;
- current time in Unix format to page 7;
- wake-up time in Unix format to page 8;

This card is reusable.

