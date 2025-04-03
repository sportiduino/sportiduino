# Настройка Arduino IDE

### Настройка путей для скетчей и менеджера плат

В меню `Файл->Настройки` изменяем размещение папки скетчей на `[Проект]/firmware`.

Далее добавляем в `Дополнительные ссылки для Менеджера плат` после запятой строку
`https://raw.githubusercontent.com/sportiduino/BoardsManagerFiles/master/package_sportiduino_index.json`

![](/img/ArduinoIdePref.png "Настройка Arduino IDE")

### Установка плат

Открываем `Инструменты->Плата:[...]->Менеджер плат...` и вводим в поиске `sportiduino`.
Нажимаем `Установка`.

![](/img/ArduinoIdeBoardsManager.png "Менеджер плат")

### Выбор платы для загрузки прошивки

Для станции отметки выбираем плату "Sportiduino Base Station".
Для станции сопряжения выбираем плату "Sportiduino Master Station (Arduino Nano)".

![](/img/ArduinoIdeBoardSelection.png "Выбор платы")


