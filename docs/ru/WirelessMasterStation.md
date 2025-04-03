# Беспроводная станция сопряжения

В системе реализована беспроводная станция сопряжения. Принципиальная схема и плата есть в [Upverter](https://upverter.com/AlexanderVolikov/55b140a993222192/Sportiduino-BTstantion/).

![](/hardware/MasterStation/bluetooth/BTstation.png)

В схеме используются модули Arduino Pro mini, DS3231, RC522 и Bluetooth-плата HC-05. Дополнительно есть место для SPI-flash памяти, в данном репозитории она никак не используется и место можно оставить пустым, но если кто-то хочет реализовать дополнительный функционал, можно задействовать эту память. Аналогично, часы DS3231 не задействованы и их место можно оставить пустым.

Gerber-файлы для заказа платы находятся в директории [`hardware/MasterStation/bluetooth`](https://github.com/sportiduino/sportiduino/tree/master/hardware/MasterStation/bluetooth).

![](/hardware/MasterStation/bluetooth/PCB_BTstation.PNG)

Принципиальная работа данной станции не отличается от обычной станции сопряжения, но обмен данных осуществляется через Bluetooth, что позволяет реализовать работу с системой через Android-устройства. Сборка осуществляется, в основном, навесным монтажом и не представляет сложности.
