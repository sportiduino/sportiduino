### Компоненты станции отметки

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s01.jpg)

1.	RFID-модуль RC522. Довольно распространён. 

2.	Плата. Заказывал через EasyEDA. Gerber файлы есть в репозитории

3.	Микросхемы

a.	Микроконтроллер Atmega328pAU 

b.	Часы DS3231SN 

c.	Стабилизатор напряжения MCP1700T-33. 

4.	Пассивные компоненты в формате 0805. 

a.	Резистор 150 Ом

b.	Резистор 47 Ом

c.	Три резистора 10 кОм

d.	Три конденсатор 0.1 мкФ

e.	Конденсатор 1 мкФ

f.	Конденсатор 4.7 мкФ

g.      Две индуктивности 2.2 мкГн, >500mA (например LQH32MN2R2K)

5.	Выводные компоненты

a.	Штырьки (идут в комплекте с RFID-платой)

b.	Коннектор PBD-6. Лучше заказать в России, Китайские совершенно не паяются

c.	Светодиод 3мм. Я использую синий

d.	Пищалка.  стоит заказывать более дорогие и надежные, например,  Jl WorldHC0903A. Обратите внимание. что отверстие в плате предназначено для малогабаритных пищалок. стандартные с али не подойдут.

6.	Батарейный отсек. 

7.	Корпус Gainta 1020BF. Хороший корпус из ABS пластика, неплохо поддается допиливанию под свои цели. 


Оборудование, которое пригодится при сборке, приведено [на отдельной странице](https://github.com/alexandervolikov/sportiduino/master/Doc/ru/Equipment.md)


### Пайка основной платы

Идущие в комплекте к модулю RFID штырьки нужно обрезать кусачками, а светодиод согнуть буквой Г и обрезать лишние концы. Не забывайте про полярность. Длинный конец светодиода это плюс, на фото ниже он сверху.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s03.jpg)

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s04.jpg)

Промазываем флюсом все места будущей пайки. Флюс лучше не особо жалеть.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s05.jpg) 

Далее припаиваем условно крупные компоненты: светодиод, разъем PBD-6 (нужно немножко подогнуть ножки), пищалку, штырёк, батарейный отсек.
 
![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s06.jpg)
![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s07.jpg)

Дальше припаиваем пассивные компоненты – резисторы и конденсаторе 0805. Это делать уже удобно с помощью «третьей руки». Если нет опыта пайки мелких компонентов, хорошо бы сначала потренироваться на какой-нибудь ненужной плате. Отпаивать-припаивать разные детали. Можно посмотреть ролики на ютубе или ещё где-нибудь, информации довольно много.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s08.jpg) 

Ну и самая ответственная часть – припаивание микросхем. Сначала стабилизатор, затем часы и атмегу. Стабилизатор и часы совсем просто припаиваются. С Атмегой можно повозиться, если паяльник не очень (как у меня), то можно сделать залипы между соседними ножками, тогда их приходится убирать оплёткой. Главное сильно не спешить и стараться всё аккуратно сделать, хорошо пропаять все ножки. Скорее всего понадобиться лупа, чтобы убедиться в качестве пайки.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s09.jpg)

### Прошивка бутлоадера и загрузка основной прошивки

Я заливаю буатлодер с помощью другой ардуины, можно делать это с помощью специальных программаторов. Ниже описан процесс с использованием Arduino UNO, точно так же можно использовать и другие Arduino, например, Nano или Pro mini.

Сначала залить прошивку ArduinoAsISP на Arduino UNO
(File - Examlpes - ArduinoAsISP). На Arduino UNO разместить конденсатор 10 мкФ между RST и GND. Подключить к программируемой плате пины 11,12,13 к MOSI, MISO, SCK соответственно. Пин 10 подключить к RST программируемой платы и запитать плату (5 В).

Далее установить Optiboot в IDE:
вставить URL-адресс:
https://github.com/Optiboot/optiboot/releases/download/v6.2/package_optiboot_optiboot-additional_index.json
в "Additional Boards Manager URLs" в File-Prefernces
После этого прошивки можно установить через меню Tools - Board - Boards Manager
Для прошивки нужно выбрать:
Board: Optiboot on 32 pins
Processor: ATmega328p
CPU Speed: 8Mhz (int)
Programmer: Arduino As ISP

Нажать Burn bootloader
Если всё нормально, появиться надпись
Done burning boatloader
Если нет, то где допущена ошибка, проверить правильность подключения, пайки и т.п.

Дальше отключаем UNO от программируемой платы. В IDE меняем программатор на "AVRISP mkII". Загружаем скетч Stantion.ino. 
Подключить к плате USB-UART переходник. Подключить контакты RST,RX,TX,GND,3.3V, в некоторых переходниках маркировка пинов выполнена нерпавильно и нужно подключть RX-RX, TX-TX, в других RX-TX, TX-RX. Также проблемы могут появиться, если в переходнике не работает пин RST, нужно читать отзывы перед покупкой. Нажать кнопку загрузки скетча.

Если всё хорошо, то станция должна три раза пикнуть (что говорит о том, что часы работают) и через 5 секунд ещё раз продолжительно пикнуть. Если всё так, то можно приступать к спайке с RFID-платой. Если не так, то попробовать подключить питание 5 вольт напрямую к атмеге (вывод гребенки RFID) и прошить, иногда это помогает. Если нет, проверять пайку.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s10.jpg)

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s11.jpg)

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s12.jpg)

После успешной заливки бутлоадера и прошивки можно припаять модуль RFID. Но сначала у него необходимо выламать/выпаять светодиод и подтягивающий резистор к линии RST, иначе всё это будет сильно потреблять ток во время работы устройства.

Также нужно заменить индуктивности L1 и L2 на аналогичные 2.2 мкГн но с большим рабочим током для увеличения мощности антены, например Murata LQH32MN2R2K (Спасибо за совет Александру Якимову).

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s12_1.JPG)

Опять-таки промазываем флюсом и припиваем.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s13.jpg) 

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s14.jpg)
 
Затем тестистируем работу станции. Предварительно стоит записать мастер-чипы номера станции, обновления времени и пару обычных чипов. После вставления батареек станция три раза пикнет, что говорит и том, что часы выставлены не верно. Затем через 5 секунд будет длинный сигнал, сигнализирующий, что программа вошла в цикл работы. По умолчанию это режим сна, поэтому станция среагирует на чип устаноки номера станции только через 25 секунд после её поднесения, но можно сразу положить станцию на чип перед вставкой батарей. Станция запишет новый номер перезагрузится, и теперь уже способна реагировать на обычные чипы. Подносим чип, ждем до 25 секунд, станция переходит в рабочий режим. Но часы по-прежнему сбиты. В рабочем цикле подносим мастер-чип времени и после перезагрузки подносим второй обычный чип. Считываем его на станции сопряжения и убеждаемся в корректной работе. Если всё так, то станцию можно отмывать. Если нет, то нужно поискать возможные ошибки, прежде всего, в спайке модуля RFID.
 
![](https://github.com/alexandervolikov/sportiduino/blob/master/Images/s15.jpg)

В принципе, если флюс неактивный, то можно флюс и не смывать. Я смывал в спирте, но есть много разных жидкостей, с помощью которых можно смыть флюс.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s16.jpg)
 
После смытия флюса плата выглядит довольно опрятно. Можно её упаковывать.
 
![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s17.jpg) 

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s18.jpg)

### Установка в корпус
В корпусе нужно срезать боковые стойки, чтобы влез батарейный блок. Я делаю это канцелярских ножом, режется пластик хорошо. Примерить нашу плату и просверлить дырку для светодиода 3мм.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s20.jpg)

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s21.jpg)

Намешать эпоксидного клея и промазать место для вставки диода

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s22.jpg) 

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s23.jpg) 

Вставить плату, диод обмазать погуще клеем, чтобы хорошо сидел. С другой стороны протереть диод салфеткой. И важдать пока клей застынет

https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s24.jpg

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s25.jpg)

После того как клей застыл, можно заливать компаунд. В принципе, можно обойтись и без него. Например, если промазать стык корпуса силиконовым герметиком. Либо вообще приклеивать крышку к корпусу и делать корпус одноразовым на срок работы батарей (порядка года) и менять батареи вместе с корпусом, что может быть вполне оправданным в виду его небольшой стоимостью и надежностью получаемых станций.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s26.jpg)

На 1 станцию отмеряем 30 мл компаунда

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s27.jpg)

И 1 мл отвердителя. Перемешиваем отвердитель с компаундом.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s28.jpg)

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s29.jpg)

И заливаем нашу плату. Должно получиться так, что все контакты залиты силиконом. Выходят только PBD-6 и пищалка. Внутри PBD тоже может быть силикон, но он не будет мешать в случае перезаливки прошивки.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s30.jpg)

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s31.jpg)

Через сутки компаунд полностью затвердевает. При этом все детали видно, в случае чего компаунд можно легко расковырять и произвести ремонт.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s32.jpg)

Батарейный отсек промазываем вазелином или другим густым гидрофобным маслом и вставляем батарейки.
 
![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s34.jpg)

И закручиваем наш корпус. Детали лежат туго, может понадобиться небольшое усилие, чтобы закрыть корпус.
 
![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s35.jpg)

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s36.jpg)

Наклеиваем метку и светоотражайки в нижней части станции (поближе к антене) А сверху остается место для номера. Станция готова.

![](https://raw.githubusercontent.com/alexandervolikov/sportiduino/master/Images/s37.jpg)
