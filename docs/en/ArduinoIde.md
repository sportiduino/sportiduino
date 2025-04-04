# Arduino IDE setup

### Setup location for sketches and url for boards manager

In the menu `Arduino IDE->Preferences` change sketchbook location to `[/path/to/project]/firmware`.

Then add new additional boards manager url
`https://raw.githubusercontent.com/sportiduino/BoardsManagerFiles/master/package_sportiduino_index.json`

![](/img/ArduinoIdePref-en.png "Preferences")

### Install boards

Select `Boards Manager` in the left panel and type `sportiduino` in the search bar.
Click `Install`.

![](/img/ArduinoIdeBoardsManager-en.png "Boards manager")

### Select board to upload firmware

In the menu `Tools` for the base station, select the "Sportiduino Base Station" board.
For the master station, select the "Sportiduino Master Station (Arduino Nano)" board.

![](/img/ArduinoIdeBoardSelection-en.png "Board selection")


