## Tags

The system supports these tags:

- Ntag 213 / 215 / 216, 32 / 120 / 216 marks
- Mifare Ultralight C, 36 marks
- Mifare Classic 1K / 4K, 42 / 90 marks

Tag type is detected automatically. One station can work with various tags simultaineously 
without any changes in firmware or hardware.

One is advised to use Ntag 213 / 215 / 216. They cost USD $0.1, 0.2, 0.4 as stickers and twice as much as key fobs.

Important! For reliable operation with these tags, which have a small antenna area (like the keychains below) it is necessary to replace the inductors with the more powerful ones in the RFID RC522 module (described on the page "Base station assembly").

Mifare Classic S50 tags come bundled with the RC522 module. The memory of these tags is enough for 42 marks. They work a little slower than Ntag. 

Only a quarter of the Mifare tags memory is used due to page size of 16 bytes (compared with 4 bytes for Ntag). With more dense recording there is a risk of data loss. The structure of the record is similar to Ntag.

![](/Images/Chip.JPG)

The memory of the Ntag 213/215/216 cards is structurally divided into 45/135/231 pages, each containing 4 bytes. The first 4 pages contain UID and other service information. Also, the service information is contained in the last 5 pages. Thus, 36/126/222 pages are available.

The structure of the records is presented below. Page numbers start from 0. Pages 4-7 are reserved, the rest of the pages are for the marks. In the 4th page, the first two bytes contain the programmable tag number. The third byte contains information about card type: 3 - Ntag213, 5 - Ntag 215, 6 - Ntag 216, it is automatically set. In the 4th byte, the station firmware version. On the fifth page, the initialization time of the card in unixtime UTC format (with it you can restore the first byte time in the marks.) Then two pages are reserved. Each station mark takes a separate page, the first byte of which contains the station number, in the remaining three bytes - the lower bytes of the current time in unixtime UTC. Time is then restored using the initialization time recorded in the 5th page.

![](/Images/Ntag2.JPG)

Master tags which set up the stations have a different structure. The station recognizes them by the number 255, recorded in the third byte of the 4th page. There are 5 different master tags: time, numbers, sleep, settings and dump. Their structure is below. After reading any of them, the station performs the requested function and erases the tag to prevent its accidental reuse.

![](/Images/master-card.png?raw=true)

A tag with a station dump has the following structure.

- pages 0 .. 3 - tag service info, irrelevant
- page 4: station number,0,0,0
- starting from page 5: if a tag number N approached this station, the N-th bit (from the start of the 5-th page) is 1, otherwise 0.
