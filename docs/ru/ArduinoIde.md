## Настройка Arduino IDE

В меню `Файл->Настройки` изменяем размещение папки скетчей на `[Проект]/firmware`.

Далее добавляем в `Дополнительные ссылки для Менеджера плат` после запятой строку
`https://raw.githubusercontent.com/sportiduino/BoardsManagerFiles/master/package_sportiduino_index.json`

![](/img/ArduinoIdePref.png?raw=true "Настройка Arduino IDE")

Открываем `Инструменты->Плата:[...]->Менеджер плат...` и вводим в поиске "sportiduino".
Нажимаем `Установка`.

![](/img/ArduinoIdeBoardsManager.png?raw=true "Менеджер плат")

Для станции отметки выбираем плату "Sportiduino Base Station".
Для станции сопряжения выбираем плату "Sportiduino Master Station (Arduino Nano)".

![](/img/ArduinoIdeBoardSelection.png?raw=true "Выбор платы")


