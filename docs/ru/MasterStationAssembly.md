# Сборка станции сопряжения

Компоненты станции сопряжения

![](/img/w01.jpg?raw=true)

1. RFID-модуль RC522.
2. Arduino Nano.
Можно подключать напрямую к компьютеру.
Платы от разных производителей различаются чипом USB интерфейса: более дорогой FT232RL или более дешевый CHG340.
Для подключения второго нужно установить драйвера в Windows.
3. Плата для спайки ([вариант 1](https://upverter.com/AlexanderVolikov/3fc0efdb2586988d/Sportiduino-reading-stantion/) и
[вариант 2](https://upverter.com/design/syakimov/4f7ec0e2d3b9c4e9/sportiduino-master-station/)).
Можно заказать изготовление плат в Китае. Например, на [JLCPCB](https://jlcpcb.com/).
Целесообразно заказывать платы для станции сопряжения одновременно с платами для станций отметки. 
4. Штыревой разъём "гребёнка", светодиод и зуммер (электромагнитный или прьезоизлучатель).
5. Корпус Gainta 1020BF или 1015. Продаются в России.
6. USB-шнур.

### Подготовка Arduino Nano

Если гребёнки идут отдельно от платы, впаиваем их.
Разъём ISP (2x3) можно не паять. 

![](/img/w02.jpg?raw=true)

#### Доработка Arduino Nano для режима эмуляции SPORTident

Необходимо удалить с платы Arduino Nano конденсатор на линии DTR-Reset.
Он расположен рядом с микросхемой контроллера USB.
После этого нужно залить на плату доработанный загрузчик Optiboot (optiboot8_atmega328_38400_without_reset.hex)
с помощью программатора.

### Вариант 1 с печатной платой в корпусе Gainta 1020BF

Впаиваем Arduino Nano в основную плату.
Также в основную плату впаиваем разъём 1x8, согнутый углом светодиод,
зуммер и резисторы типоразмера 0805 на 47 Ом (к зуммеру) и 150 Ом (к светодиоду).

![](/img/w03.jpg?raw=true)

Затем припаиваем модуль RFID. Подключаем через провод USB к компьютеру.

![](/img/w04.jpg?raw=true)

Открываем программу Arduino IDE на компьютере (см. [Настройка Arduino IDE](ArduinoIde.md)). 

В меню `Инструменты->Плата` выбираем плату `Sportiduino Master Station`.

Для активного зуммера изменяем частоту на 0 кГц ("Buzzer frequency: 0 kHz (active buzzer)").

Выбираем нужный COM-порт и заливаем скетч MasterStation на плату (Файл->Папка со скетчами->MasterStation).

![](/img/ArduinoIdeMasterSelect.png?raw=true)

**Важно!** Без конденсатора на линии DTR-Reset не работает привычная процедура заливки прошивки.
Обязательно нужно обновить загрузчик с помощью программатора (`Инструменты->Записать Загрузчик`).
Обновлённый загрузчик ждёт 2 секунды после подачи питания по USB на плату, после чего загружает основную прошивку.
То есть для обновления прошивки (заливки скетча) нужно:
- отключить-подключить плату к USB;
- сразу после подать команду на обновление прошивки.

Примеряем корпус. Для провода вырезается небольшая канавка в корпусе. Светодиод промазывается эпоксидным клеем.

![](/img/w05.jpg?raw=true)

После чего корпус закрывается крышкой и готов к использованию.

![](/img/w06.jpg?raw=true)

### Вариант станции без печатной платы

Можно собрать станцию сопряжения без печатной платы. Светодиод, зуммер и RFID-модуль припаиваются проводками к Arduino Nano согласно схеме.

![](/hardware/MasterStation/usb/sportiduino-master-scheme.png?raw=true)

Рекомендуется использовать провод типа МГТФ-0,12.
Далее для такого варианта сборки разработан свой корпус, который можно распечатать на 3D-принтере.
Файлы для печати корпуса на 3D-принтере находятся в папке `hardware/MasterStation/3d/print`.

![](/img/MasterStationBoxTop.jpg?raw=true)

![](/img/MasterStationBoxBot.jpg?raw=true)

![](/img/MasterStationInBox.jpg?raw=true)

