#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>

#include <MFRC522.h>
#include <ds3231.h>
#include <Adafruit_SleepyDog.h>
#include <PinChangeInterrupt.h>

// Версия прошивки
#define FIRMWARE_VERSION 105

// Для компиляции проекта необходимо установить бибилиотеки
// - AdafruitSpeepyDog by Adafruit (https://github.com/adafruit/Adafruit_SleepyDog)
// - MFRC522 by GithubCommunity (https://github.com/miguelbalboa/rfid)
// - DS3231-master из папки Sportiduino/Libraries/DS3231-master
// - PinChangeInterrupt by NicoHood https://github.com/NicoHood/PinChangeInterrupt
// Для этого откройте Скетч->Подключить Библиотеку->Управление Бибилиотеками.
// В поиске введите соответствующие библиотеки и нажмите для каждой INSTALL

//-------------------------------------------------------------------
// НАСТРОЙКА

// По умолчанию с завода МК настроен на работу от встроенного RC генератора с делителем на 8
// В итоге системная частота = 1 МГц
// Ардуино по умолчанию настроен на частоту 16 МГц, поэтому необходимо отредактировать
// текстовый файл с настройками платы Ардиуно и установить системную частоту 1 МГц

// Уберите комментарий со строки ниже, если на плате установлен пьезоизлучатель без генератора (например, HC0905A)
//#define PIEZO_FREQ 3200

// Внимание: DS3231 настраиваются по времени UTC!

// Уберите комментарий ниже для отладки
//#define DEBUG

// Коэффициент усиления антенны модуля RC522. Max = (7<<4) (48 dB), Mid = (3<<4) (23 dB), Min = (0<<4) (18 dB)
#define RC522_ANTENNA_GAIN (7<<4)

// Максимально-допустимый адерс страницы на карточке = Кол-во блоков на карте - кол-во трейл-блоков + 2
// Количество отметок = Максимальный адрес - 8
// MIFARE Classic 1K (MF1S503x) = 50 страниц (Всего 64 блока - 16 трейл-блоков + 2 = 50); 42 отметки = 50 - 8
// MIFARE Classic 4K (MF1S703x) = 98 страниц (Всего 128 основных блоков - 32 трейл-блока + 2 = 98); 90 отметок = 98 - 8. Расширенные блоки не поддерживаются!
// MIFARE Classic Mini (MF1 IC S20) = 17 страниц (Всего 20 блоков - 5 трейл-блоков + 2 = 17); 9 отметок = 17 - 8
// Карты MIFARE Ultralight (MF0ICU1) и MIFARE Ultralight C (MF0ICU2) не поддерживаются!
#define CARD_PAGE_MAX 50

// Срок годности карточки участника (в секундах)
// 31 день = 2678400
#define CARD_EXPIRE_TIME 2678400L

// Максимально-допустимый номер карточки
#define MAX_CARD_NUM (CARD_PAGE_MAX - 5)*4*8

// Настройки станции
// Bit1,Bit0 - Время перехода в режим ожидания при бездействия
// (0,0) - сон через <период1>;
// (0,1) - сон через <период2>;
// (1,0) - всегда проверять чипы через 1 секундe;
// (1,1) - всегда проверять чипы через 0.25с
// Bit2 - Проверять отметки стартовой и финишной станции на чипах участников (0 - нет, 1 - да)
// Bit3 - Проверять время на чипах участников (0 - нет, 1 - да)
// Bit4 - Сбрасывать настройки при переходе в сон (0 - нет, 1 -да)
// Bit7 - Настройки валидны (0 - да, 1 - нет)

#define SETTINGS_INVALID                0x80
#define SETTINGS_WAIT_PERIOD1           0x0
#define SETTINGS_WAIT_PERIOD2           0x1
#define SETTINGS_ALWAYS_WAIT            0x2
#define SETTINGS_ALWAYS_ACTIVE          0x3
#define SETTINGS_CHECK_START_FINISH     0x4
#define SETTINGS_CHECK_CARD_TIME        0x8
#define SETTINGS_CLEAR_ON_SLEEP         0x10

// Настройки по умолчанию (побитовое или из макросов SETTINGS, например, SETTINGS_WAIT_PERIOD1 | SETTINGS_CHECK_START_FINISH)
#define DEFAULT_SETTINGS  SETTINGS_WAIT_PERIOD1

// Зарезервированный номер стартовой станции
#define START_STATION_NUM         240
// Зарезервированный номер финишной станции
#define FINISH_STATION_NUM        245
// Зарезервированный номер станции проверки
#define CHECK_STATION_NUM         248
// Зарезервированный номер станции очистки
#define CLEAR_STATION_NUM         249

// Номер станции по умолчанию после сборки
#define DEFAULT_STATION_NUM       CHECK_STATION_NUM

// Период1 работы в режиме ожидания (в миллисекундах)
// 6 часов = 2160000
#define WAIT_PERIOD1              21600000L
// Период2 работы в режиме ожидания (в миллисекундах)
// 48 часов = 172800000
#define WAIT_PERIOD2              172800000L

// Период проверки чипов в активном режиме (в миллисекундах)
#define MODE_ACTIVE_CARD_CHECK_PERIOD     250
// Период проверки чипов в режиме ожидания (в миллисекундах)
#define MODE_WAIT_CARD_CHECK_PERIOD       1000
// Период проверки чипов в режиме сна (в миллисекундах)
#define MODE_SLEEP_CARD_CHECK_PERIOD      25000

// Время сна после чтения мастер-чипа, очистки чипа (Этот сон нужен, чтобы спокойно убрать мастер-чип)
#define SLEEP_BETWEEN_MASTER_CARD         3000

// Cигнал при загрузке станции
#define BEEP_SYSTEM_STARTUP     //beep(1000,1)

// Сигнал-ошибка при чтении еепром памяти
#define BEEP_EEPROM_ERROR       beep(50,2)
// Сигнал-ошибка не верно идут часы
#define BEEP_TIME_ERROR         beep(50,3)
// Сигнал-ошибка не подходит пароль мастер чипа
#define BEEP_PASS_ERROR         beep(50,4)
// Сигнал нужно заменить батареи
#define BEEP_LOW_BATTERY        beep(50,5)
#define BEEP_BATTERY_OK         beep(500,1)
// Сигнал чип не прошёл проверку

#define BEEP_CARD_CHECK_ERROR   //beep(200,3)
#define BEEP_CARD_CHECK_OK      beep(500,1)
// Сигнал записи отметки на карточку участника
#define BEEP_CARD_MARK_WRITTEN  beep(500,1)
// Сигнал повторного прикладывания карточки участника
#define BEEP_CARD_MARK_OK       beep(250,2)
// Сигнал ошибки записи на карточку участника
#define BEEP_CARD_MARK_ERROR
// Сигнал успешной очистки карточку участника
#define BEEP_CARD_CLEAR_OK      beep(500,1)
// Сигнал успешной проверки карточки участника
#define BEEP_CARD_CLEAR_ERROR

// Сигнал прочитан мастер чип паролей
#define BEEP_MASTER_CARD_PASS_OK            beep(500,2)
#define BEEP_MASTER_CARD_PASS_ERROR
// Сигнал прочитан мастер-чип времени
#define BEEP_MASTER_CARD_TIME_OK            beep(500,3)
#define BEEP_MASTER_CARD_TIME_ERROR
// Сигнал прочитан мастер-чип сна
#define BEEP_MASTER_CARD_SLEEP_OK           beep(500,4)
#define BEEP_MASTER_CARD_SLEEP_ERROR
// Сигнал изменён номер станции
#define BEEP_MASTER_CARD_STATION_WRITTEN    beep(500,5)
#define BEEP_MASTER_CARD_STATION_OK         beep(500,1)
#define BEEP_MASTER_CARD_STATION_ERROR      beep(50,6)
// Сигнал прочитан мастер-чип дампа
#define BEEP_MASTER_CARD_DUMP_OK            beep(500,6)
#define BEEP_MASTER_CARD_DUMP_ERROR
// Сигнал прочитан мастер-чип чтения информации о станции
#define BEEP_MASTER_CARD_GET_INFO_OK        beep(250,1)
#define BEEP_MASTER_CARD_GET_INFO_ERROR     beep(250,2)

//--------------------------------------------------------------------
// Пины

#define LED           4
#define BUZ           3
#define RC522_RST     9
#define RC522_SDA     10
#define RC522_IRQ     6
#define DS3231_IRQ    A3
#define DS3231_32K    5

#define UNKNOWN_PIN 0xFF

//--------------------------------------------------------------------
// Константы

#define MASTER_CARD_GET_INFO        249
#define MASTER_CARD_SET_TIME        250
#define MASTER_CARD_SET_NUMBER      251
#define MASTER_CARD_SLEEP           252
#define MASTER_CARD_READ_DUMP       253
#define MASTER_CARD_SET_PASS        254

// Адрес страницы на карторчке с информацией о ней
#define CARD_PAGE_INFO              4
// Адрес страницы на карточке со временем инициализации чипа
#define CARD_PAGE_INIT_TIME         5
// Адрес страницы на карточке с информацией о последней отметке (не используется!)
#define CARD_PAGE_LAST_RECORD_INFO  6
// Адрес начала отметок на чипе
#define CARD_PAGE_START             8

#define EEPROM_STATION_NUM_ADDR     800
#define EEPROM_PASS_ADDR            850
#define EEPROM_SETTINGS_ADDR        859

//--------------------------------------------------------------------
// Переменные

// Время работы (в миллисекундах)
uint32_t workTimer;
// Пароль для проверки мастер-чипов
uint8_t pass[3];
// Ключ шифрования карточек
MFRC522::MIFARE_Key key;
// RC522
MFRC522 mfrc522(RC522_SDA, RC522_RST);
// Номер станции
uint8_t stationNum;
// Настройки станции
uint8_t settings;
// Режим работы станции
uint8_t mode;
// Дата/Время
ts t;
// Год для выхода из режима сна (эта переменная нужна, потому что DS321 не поддерживает год и месяц в Alarms)
int16_t alarmYear;
// Месяц для выхода из режима сна (эта переменная нужна, потому что DS321 не поддерживает год и месяц в Alarms)
uint8_t alarmMonth;
// Этот флаг устанавливается, когда приходит прерывание от DS3231
uint8_t rtcAlarmFlag;

#define MODE_ACTIVE   0
#define MODE_WAIT     1
#define MODE_SLEEP    2

#define DEFAULT_MODE MODE_WAIT

//--------------------------------------------------------------------
// Прототипы функций

/**
 * Фунцкия программной перезагрузки
 */
void(*resetFunc)(void) = 0;

/**
 * Возвращает текущий режим работы пина
 */
uint8_t getPinMode(uint8_t pin);

/**
 * Функция записи во внутреннюю память микроконтроллера
 * Запись приосходит с мажоритарным резервированием в три подряд ячейки
 */
void eepromWrite (uint16_t adr, uint8_t val);

/**
 * Считывание ячейки из внутренней памяти МК с учетом мажоритарного резервирования
 */
uint8_t eepromRead(uint16_t adr);

/**
 * Переводит устройство в энергосберегающий режим сна на заданное время
 */
void sleep(uint16_t ms);

/**
 *  Устанавливает текущий режим работы
 */
void setMode(uint8_t md);

/**
 * Запись номера карточки участника в лог. Только факт отметки, время отметки не записывается
 */
void writeCardNumToLog(uint16_t num);

/**
 * Очистка лога отметок
 * адреса 0 - 750
 */
void clearMarkLog();

/**
 * Выдача сигнала. Принимает продолжительность сигнала и число сигналов подряд
 * В ходе работы сбрасывает вотчдог, чтобы не произошла перезагрузка
 */
void beep(uint16_t ms, uint8_t n);

/**
 * Функция считывания напряжения питания МК. 
 * Сравнение происходит по внутреннему источнику опроного напряжения в 1.1 В
 */
uint32_t readVcc(uint32_t refConst);

/**
 * Измерение напряжения. Включает диод на 5 секунд. Затем происходит измерение.
 * Если напряжение меньше 3.1 В, то станция выдает три длинные сигнала. Если больше, то один.
 * 
 * @return true если напряжение на батарее в норме
 */
bool voltage();

/**
 * Читает страницу карточки. Эта функция должна вызываться после инициализации RC522.
 * 
 * @param data Указатель на буфер, куда будут сохранены данные. Длина буфера должна быть не менее 18 байт
 * @param size Указатель на переменную, хранящую размер буфера, после чтения в этой переменной будет количество прочитанных байт
 */
bool cardPageRead(uint8_t pageAdr, byte *data, byte *size);

/**
 * Записывает блок данных на карточку
 * 
 * @param data Буфер данных. Размер буфер должен быть не менее 16 байт
 * @param size Размер буфера
 */
bool cardPageWrite(uint8_t pageAdr, byte *data, byte size);

/**
 * Основная функция работы с чипами
 */
void rfid();

/**
 * Функция обработки мастер-чипа времени.
 * С чипа считыввается новое время и устанавливается 
 * внутреннее время. Станция пикает 5 раз в случае успеха
 */
void processTimeMasterCard(byte *data, byte dataSize);


/**
 * Функция установки нового номера станции
 * станция считывает чип, записывает новый номер
 * во внутреннюю память, пикает 5 раз и перезагружается
 */
void processStationMasterCard(byte *data, byte dataSize);

/**
 * Функция обработки мастер-чипа сна. 
 * Станция стирает данные о пароле и настройках,
 * пикает три раза и входит в сон
 */
void processSleepMasterCard(byte *data, byte dataSize);

/**
 * Функция записи дамп-чипа. Станция считывает все данные по чипам из внутренней памяти
 * и записывает их последовательно на дамп-чип. После чего один раз пикает и выходит.
 */
void processDumpMasterCard(byte *data, byte dataSize);

/**
 * Функция обработки мастер-чипа смены пароля. Станция считывает новый пароль и байт настроек. Записывает его в память.
 * Пикает два раза и перезагружается.
 */
void processPassMasterCard(byte *data, byte dataSize);

/**
 * Функция обработки мастер-чипа получения информации о работе станции
 */
void processGetInfoMasterCard(byte *data, byte dataSize);

/**
 * Функция обработки карточки участника
 */
void processParticipantCard(uint16_t cardNum);

/**
 * Функция поиска последней записанной страницы по алгоритму бинарного поиска.
 * 
 * @note Проще не использовать бинарный поиск и при отметке станции на чипе записывать, например,
 * в страницу 6 номер последней станции и адрес свободной страницы. Но при этом эта страница
 * будет очень часто перезаписываться и чип быстро выйдет из строя. Поэтому реализован
 * бинарный поиск свободной страницы
 * 
 * @param newPage Указатель на переменную, куда будет сохранен номер первой чистой страницы
 * @param lastNum Указатель на переменную, куда будет сохранён номер последней записанной станции
 */
void findNewPage(uint8_t *newPage, uint8_t *lastNum);

/**
 * Функция записи отметки в карточку участника.
 * Записывает номер и поседние 3 байта юникстайм в чип.
 */
bool writeMarkToParticipantCard(uint8_t newPage);

/**
 * Функция очистки карточки участника
 */
void clearParticipantCard();

/**
 * Функция проверки карточки участника
 */
void checkParticipantCard();

/**
 * Проверяет время инициализации чипа
 */
bool doesCardExpire();

/**
 * Обработчик прерывания от RTC
 */
void rtcAlarmIrq();

//--------------------------------------------------------------------
// Реализация

/**
 * После запуска или перезагрузки станция считывает показание часов
 * Если они сбиты, то издает три коротких звуковых сигнала
 * Запоминает промежуточное время temph, чтобы вовремя перейти в режим ожиданиния при бездействии
 * 
 * Далее присходит счтиывание из EEPROM памяти настроек станций:
 * - номера станции
 * - паролей мастер-чипа
 * - байта настроек работы станции
 * 
 * Затем станция выжидает 5 секунд и после длинного сигнала выходит в цикл loop
 */
void setup()
{
  // Блокируем и сбрасываем вотчдог
  MCUSR &= ~(1 << WDRF);
  Watchdog.disable();
  Watchdog.reset();

  // Настраиваем пины
  pinMode(LED,OUTPUT);
  pinMode(BUZ,OUTPUT);
  pinMode(RC522_RST,OUTPUT);
  pinMode(RC522_SDA,OUTPUT);
  pinMode(RC522_IRQ,INPUT_PULLUP);
  pinMode(DS3231_IRQ,INPUT_PULLUP);
  pinMode(DS3231_32K,INPUT_PULLUP);

  digitalWrite(LED,LOW);
  digitalWrite(BUZ,LOW);
  digitalWrite(RC522_RST,LOW);
  
  // Настраиваем неиспользуемые пины
  // Согласно документации на них лучше включить встроенные pull-up резисторы
  for(byte pin = 0; pin < A5; pin++)
  {
    if(getPinMode(pin) == INPUT)
      pinMode(pin, INPUT_PULLUP);
  }
  
  // Настраиваем RTC
  // Сбрасываем все прерывания и выключаем выход 32 кГц
  DS3231_set_addr(DS3231_STATUS_ADDR, 0);
  DS3231_init(DS3231_INTCN | DS3231_A1IE);
  alarmYear = 2017;
  alarmMonth = 1;
  memset(&t, 0, sizeof(t));
  // Читаем текущее время
  DS3231_get(&t);
  
  if(t.year < 2017)
  {
    // Часы не настроены
    BEEP_TIME_ERROR;
  }

  // Настраиваем приём прерываний от RTC
  attachPCINT(digitalPinToPCINT(DS3231_IRQ), rtcAlarmIrq, FALLING);

  // Читаем настройки из EEPROM
  stationNum = eepromRead(EEPROM_STATION_NUM_ADDR);

  for (uint8_t i = 0; i < 3; i++)
    pass[i] = eepromRead(EEPROM_PASS_ADDR + i*3);
    
  settings = eepromRead(EEPROM_SETTINGS_ADDR);

  // После сборки станции номер не установлен, применяем по умолчанию
  if(stationNum == 0 || stationNum == 255)
    stationNum = DEFAULT_STATION_NUM;

  // Применяем настройки по умолчанию после сборки станции
  if(settings & SETTINGS_INVALID)
  {
    settings = DEFAULT_SETTINGS;
    pass[0] = pass[1] = pass[2] = 0;

    // Очищаем лог отметок
    clearMarkLog();

    // Сохраняем настройки и пароль по умолчаню в EEPROM
    eepromWrite(EEPROM_SETTINGS_ADDR, settings);
    for (uint8_t i = 0; i < 3; i++)
      eepromWrite(EEPROM_PASS_ADDR + i*3, pass[i]);
  }

  // Устанавливаем режим работы по умолчанию
  setMode(DEFAULT_MODE);
 
  // Инициализруем ключ для карточек MIFARE
  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;
  
  // Проверяем батарейки
  voltage();
  // Для разделения сигналов от батарейки и перезагрузки
  delay(1000);
  // Сигнализируем о переходе в основной цикл
  BEEP_SYSTEM_STARTUP;
  // Сбрасываем программный таймер
  workTimer = 0;
  // Включаем вотчдог для защиты от зависаний
  Watchdog.enable(8000);
}

void loop()
{
  Watchdog.reset();

  // Обрабатываем прерывание от RTC
  if(rtcAlarmFlag)
  {
    // Сбрасываем флаг прерывания RTC
    rtcAlarmFlag = 0;
    DS3231_clear_a1f();
    // Узнаем текущую дату
    DS3231_get(&t);
    // DS3231 не поддерживает месяц и год в Alarms! Поэтому проверка реализована на MCU
    if(t.year == alarmYear && t.mon == alarmMonth)
    {
      setMode(MODE_ACTIVE);
    }
  }

  // Обрабатываем карточки
  rfid();

  // Управление режимами работы
  switch(mode)
  {
    case MODE_ACTIVE:
      sleep(MODE_ACTIVE_CARD_CHECK_PERIOD);

      #ifdef DEBUG
        digitalWrite(LED,HIGH);
      #endif

      if(settings & SETTINGS_ALWAYS_ACTIVE)
      {
         workTimer = 0;
      }
      else if(workTimer >= WAIT_PERIOD1)
      {
        setMode(MODE_WAIT);
      }
      break;
    case MODE_WAIT:
      sleep(MODE_WAIT_CARD_CHECK_PERIOD);

      #ifdef DEBUG
        digitalWrite(LED,HIGH);
      #endif

      if(settings & SETTINGS_ALWAYS_WAIT)
      {
        workTimer = 0;
      }
      else if(workTimer >= WAIT_PERIOD1 && (settings & SETTINGS_WAIT_PERIOD1))
      {
        setMode(MODE_SLEEP);
      }
      else if(workTimer >= WAIT_PERIOD2 && (settings & SETTINGS_WAIT_PERIOD2))
      {
        setMode(MODE_SLEEP);
      }
      break;
    case MODE_SLEEP:
      sleep(MODE_SLEEP_CARD_CHECK_PERIOD);

      #ifdef DEBUG
        digitalWrite(LED,HIGH);
      #endif
      
      break;
  }
}

uint8_t getPinMode(uint8_t pin)
{
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);

  // I don't see an option for mega to return this, but whatever...
  if (NOT_A_PIN == port) return UNKNOWN_PIN;

  // Is there a bit we can check?
  if (0 == bit) return UNKNOWN_PIN;

  // Is there only a single bit set?
  if (bit & bit - 1) return UNKNOWN_PIN;

  volatile uint8_t *reg, *out;
  reg = portModeRegister(port);
  out = portOutputRegister(port);

  if (*reg & bit)
    return OUTPUT;
  else if (*out & bit)
    return INPUT_PULLUP;
  else
    return INPUT;
}

void sleep(uint16_t ms)
{
  // Мы не можем уснуть, если есть необработанное прерывание от DS3231
  if(rtcAlarmFlag)
    return;
    
  uint16_t period;
  // Выключаем модуль RC522
  digitalWrite(RC522_RST,LOW);
  // Выключаем светодиод
  digitalWrite(LED,LOW);
  // Выключаем буззер
  digitalWrite(BUZ,LOW);
  // Сбрасываем вотчдог и засыпаем
  Watchdog.reset();
  period = Watchdog.sleep(ms);
  workTimer += period;
  // Используем рекурсию, если время проведённое во сне меньше заданного
  if(ms > period)
    sleep(ms - period);
}

void setMode(uint8_t md)
{
  mode = md;
  workTimer = 0;

  // Перенастраиваем режим в соответствии с настройками
  if(settings & SETTINGS_ALWAYS_WAIT)
    mode = MODE_WAIT;
  else if(settings & SETTINGS_ALWAYS_ACTIVE)
    mode = MODE_ACTIVE;
}

void eepromWrite(uint16_t adr, uint8_t val)
{
  for(uint16_t i = 0; i < 3; i++)
    EEPROM.write(adr + i, val);
}

uint8_t eepromRead(uint16_t adr)
{
  uint8_t val1 = EEPROM.read(adr);
  uint8_t val2 = EEPROM.read(adr + 1);
  uint8_t val3 = EEPROM.read(adr + 2);
  
  if(val1 == val2 || val1 == val3)
  {
    return val1;
  }
  else if(val2 == val3)
  {
    return val2;
  }
  else
  {
    BEEP_EEPROM_ERROR;
    return 0;
  }
}

void writeCardNumToLog(uint16_t num)
{
  if(num > MAX_CARD_NUM)
  {
    BEEP_EEPROM_ERROR;
    return;
  }
    
  uint16_t byteAdr = num/8;
  uint16_t bitAdr = num%8;
  uint8_t eepromByte = EEPROM.read(byteAdr);
  bitSet(eepromByte, bitAdr);
  EEPROM.write(byteAdr, eepromByte);
}

void clearMarkLog()
{
  uint16_t endAdr = (CARD_PAGE_MAX - 5)*4;
  for (uint16_t a = 0; a <= endAdr; a++)
  {
    Watchdog.reset();
    EEPROM.write(a,0);
    delay(2);
  }
}

void beep(uint16_t ms, uint8_t n)
{
  for (uint8_t i = 0; i < n; i++)
  {
    Watchdog.reset();
    
    digitalWrite(LED, HIGH);
    #ifdef PIEZO_FREQ
      tone(BUZ, PIEZO_FREQ, ms);
    #else
      digitalWrite (BUZ, HIGH);
    #endif
    
    delay(ms);
    Watchdog.reset();
    
    digitalWrite(LED, LOW);
    digitalWrite(BUZ, LOW);
    
    if (i < n - 1)
    {
      delay(ms);
      Watchdog.reset();
    }
  }
}

uint32_t readVcc(uint32_t refConst)
{
  // Включаем АЦП
  ADCSRA |=  bit(ADEN); 
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  // Ждём стабилиазции опорного источника
  delay(5);
  // Начинаем измерение
  ADCSRA |= _BV(ADSC);
  while(bit_is_set(ADCSRA, ADSC));

  // Читаем обязательно сначала младший байт, затем старший
  uint8_t low  = ADCL;
  uint8_t high = ADCH;

  uint32_t result = (high << 8) | low;

  result = refConst / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  // Выключаем АЦП
  ADCSRA ^= bit(ADEN);
  
  return result;
}

bool voltage()
{
  const uint32_t refConst = 1125300L; //voltage constanta
  uint32_t value = 0;
  bool result = false;

  Watchdog.reset();

  digitalWrite(LED, HIGH);
  delay(5000);

  for (uint8_t i = 0; i < 10; i++)
    value += readVcc(refConst);

  value /= 10;

  digitalWrite(LED, LOW);
  delay(250);
  
  Watchdog.reset();

  if(value < 3100)
  {
    BEEP_LOW_BATTERY;
    result = false;
  }
  else
  {
    BEEP_BATTERY_OK;
    result = true;
  }

  return result;
}

bool cardPageRead(uint8_t pageAdr, byte *data, byte *size)
{
  if(pageAdr < 3)
    return false;
    
  MFRC522::StatusCode status;
  // Адрес читаемого блока
  byte blockAddr = pageAdr-3 + ((pageAdr-3)/3);
  // Адрес блока ключей
  byte trailerBlock = blockAddr + (3-blockAddr%4);

  // Авторизация по ключу А
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  
  if(status != MFRC522::STATUS_OK)
    return false;
  
  status = mfrc522.MIFARE_Read(blockAddr, data, size);
  
  if(status != MFRC522::STATUS_OK)
    return false;
 
  return true; 
}

bool cardPageWrite(uint8_t pageAdr, byte *data, byte size)
{
  MFRC522::StatusCode status;
  
  byte blockAddr = pageAdr-3 + ((pageAdr-3)/3);
  byte trailerBlock = blockAddr + (3-blockAddr%4);

  // Авторизация по ключу А
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  
  if(status != MFRC522::STATUS_OK)
    return false;

  // Записываем данные в блок
  status = mfrc522.MIFARE_Write(blockAddr, data, size);
  
  if(status != MFRC522::STATUS_OK)
    return false;

  return true;
}

void rfid()
{
  byte pageData[18];
  byte dataSize;
  byte masterCardData[16];
  bool result;
  bool sleepBtwCard = false;

  // Включаем SPI и RC522. Ищем карту вблизи. Если не находим выходим из функции чтения чипов
  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_AntennaOff();
  mfrc522.PCD_SetAntennaGain(RC522_ANTENNA_GAIN);
  mfrc522.PCD_AntennaOn();
  
  delay(5);
  
  if(mfrc522.PICC_IsNewCardPresent())
  {
    if(mfrc522.PICC_ReadCardSerial())
    {
      //Читаем блок информации
      dataSize = sizeof(pageData);
      if(cardPageRead(CARD_PAGE_INFO, pageData, &dataSize))
      {
        // Проверяем тип чипа
        if(pageData[2] == 0xFF)
        {
          // Мастер-чип

          // Переходим в активный режим, если это не карточка чтения информации
          if(pageData[1] != MASTER_CARD_GET_INFO)
            setMode(MODE_ACTIVE);
          
          // Копируем информацию о мастер-чипе
          memcpy(masterCardData, pageData, 4);
          // Читаем данные с мастер-чипа
          result = true;
          
          dataSize = sizeof(pageData);
          result &= cardPageRead(CARD_PAGE_INFO + 1, pageData, &dataSize);
          if(result)
            memcpy(masterCardData + 4, pageData, 4);
            
          dataSize = sizeof(pageData);
          result &= cardPageRead(CARD_PAGE_INFO + 2, pageData, &dataSize);
          if(result)
            memcpy(masterCardData + 8, pageData, 4);
          
          dataSize = sizeof(pageData);
          result &= cardPageRead(CARD_PAGE_INFO + 3, pageData, &dataSize);
          if(result)
            memcpy(masterCardData + 12, pageData, 4);

          if(result)
          {
            // Обрабатываем мастер-чип

            // Проверяем пароль
            if( (pass[0] == masterCardData[4]) &&
                (pass[1] == masterCardData[5]) &&
                (pass[2] == masterCardData[6]) )
            {
              switch(masterCardData[1])
              {
                case MASTER_CARD_SET_TIME:
                  processTimeMasterCard(masterCardData, sizeof(masterCardData));
                  break;
                case MASTER_CARD_SET_NUMBER:
                  processStationMasterCard(masterCardData, sizeof(masterCardData));
                  break;
                case MASTER_CARD_SLEEP:
                  processSleepMasterCard(masterCardData, sizeof(masterCardData));
                  break;
                case MASTER_CARD_READ_DUMP:
                  processDumpMasterCard(masterCardData, sizeof(masterCardData));
                  break;
                case MASTER_CARD_SET_PASS:
                  processPassMasterCard(masterCardData, sizeof(masterCardData));
                  break;
                case MASTER_CARD_GET_INFO:
                  processGetInfoMasterCard(masterCardData, sizeof(masterCardData));
              }

              sleepBtwCard = true;
            }
            else
              BEEP_PASS_ERROR;
          }
        } // Конец обработки мастер-чипа
        else
        {
          // Переходим в активный режим
          setMode(MODE_ACTIVE);
          // Обработка чипа участника
          switch(stationNum)
          {
            case CLEAR_STATION_NUM:
              clearParticipantCard();
              sleepBtwCard = true;
              break;
            case CHECK_STATION_NUM:
              checkParticipantCard();
              break;
            default:
              processParticipantCard((((uint16_t)pageData[0])<<8) + pageData[1]);
              break;
          }
        }
      } 
    }
  }
  // Завершаем работу с карточкой
  mfrc522.PICC_HaltA();
  // Завершаем работу с SPI
  SPI.end();
  // Переводим RC522 в хард-ресет
  digitalWrite(RC522_RST,LOW);

  // Дополнительная задеркжа между чтением чипов
  // Нужна, чтобы спокойно можно было убрать мастер-чип после обработки
  if(sleepBtwCard)
    sleep(SLEEP_BETWEEN_MASTER_CARD);
}

void processTimeMasterCard(byte *data, byte dataSize)
{
  if(dataSize < 16)
  {
    BEEP_MASTER_CARD_TIME_ERROR;
    return;
  }

  // Внимание: часы настраиваются на время UTC
  memset(&t, 0, sizeof(t));

  t.mon = data[8];
  t.year = data[9]+2000;
  t.mday = data[10];
  t.hour = data[12];
  t.min = data[13];
  t.sec = data[14];

  DS3231_set(t);
  memset(&t, 0, sizeof(t));
  DS3231_get(&t);

  if(t.year < 2017)
    BEEP_TIME_ERROR;
  
  BEEP_MASTER_CARD_TIME_OK;
}

void processStationMasterCard(byte *data, byte dataSize)
{
  if(dataSize < 16)
  {
    BEEP_MASTER_CARD_STATION_ERROR;
    return;
  }

  uint8_t newNum = data[8];

  if(newNum > 0)
  {
    if(stationNum != newNum)
    {
      stationNum = newNum;
      eepromWrite(EEPROM_STATION_NUM_ADDR, stationNum);

      BEEP_MASTER_CARD_STATION_WRITTEN;
    }
    else
      BEEP_MASTER_CARD_STATION_OK;
  }
  else
    BEEP_MASTER_CARD_STATION_ERROR;
}

void processSleepMasterCard(byte *data, byte dataSize)
{
  if(dataSize < 16)
  {
    BEEP_MASTER_CARD_SLEEP_ERROR;
    return;
  }

  // вне зависимости от настроек, всегда засыпаем
  // поэтому не используется setMode
  mode = MODE_SLEEP;
  
  if(settings & SETTINGS_CLEAR_ON_SLEEP)
  {
    settings = DEFAULT_SETTINGS;
    eepromWrite(EEPROM_SETTINGS_ADDR,settings);
  }

  // Настраиваем будильник в DS3231
  uint8_t flags[5] = {0,0,0,0,0};
  memset(&t, 0, sizeof(t));
  alarmMonth = t.mon = data[8];
  alarmYear = t.year = ((int16_t)data[9]) + 2000;
  t.mday = data[10];
  t.hour = data[12];
  t.min = data[13];
  t.sec = data[14];

  DS3231_clear_a1f();
  DS3231_set_a1(t.sec, t.min, t.hour, t.mday, flags);

  clearMarkLog();

  BEEP_MASTER_CARD_SLEEP_OK;
}

void processDumpMasterCard(byte *data, byte dataSize)
{
  if(dataSize < 16)
  {
    BEEP_MASTER_CARD_DUMP_ERROR;
    return;
  }
    
  uint8_t dataEeprom[16];
  uint16_t eepromAdr = 0;
  bool result = true;

  memset(dataEeprom, 0, sizeof(dataEeprom));

  for(uint8_t page = CARD_PAGE_INIT_TIME; page <= CARD_PAGE_MAX; page++)
  {
    Watchdog.reset();
    delay(50);
    
    digitalWrite(LED,HIGH);
    
    for(uint8_t m = 0; m < 4; m++)
    {
      dataEeprom[m] = EEPROM.read(eepromAdr);
      eepromAdr++;
    }

    result &= cardPageWrite(page, dataEeprom, sizeof(dataEeprom));
    
    digitalWrite(LED, LOW);
  }

  if(result)
    BEEP_MASTER_CARD_DUMP_OK;
  else
    BEEP_MASTER_CARD_DUMP_ERROR;
    
  return;
}

void processPassMasterCard(byte *data, byte dataSize)
{
  if(dataSize < 16)
  {
    BEEP_MASTER_CARD_PASS_ERROR;
    return;
  }
    
  for(uint8_t i = 0; i < 3; i++)
  {
    pass[i] = data[i + 8];
    eepromWrite(EEPROM_PASS_ADDR + i*3, pass[i]);
  }
  
  settings = data[11];
  eepromWrite(EEPROM_SETTINGS_ADDR, settings);

  BEEP_MASTER_CARD_PASS_OK;
}

void processGetInfoMasterCard(byte *data, byte dataSize)
{
  if(dataSize < 16)
  {
    BEEP_MASTER_CARD_GET_INFO_ERROR;
    return;
  }  

  byte pageData[16];
  memset(pageData, 0, sizeof(pageData));
  // Проверяем батарейки
  bool batteryOk = voltage();
  // Получаем текущую дату и время
  DS3231_get(&t);
  // Записываем версию прошивки
  pageData[0] = FIRMWARE_VERSION;
  bool result = cardPageWrite(CARD_PAGE_START, pageData, sizeof(pageData));
  // Записываем информацию о станции
  memset(pageData, 0, sizeof(pageData));
  pageData[0] = stationNum;
  pageData[1] = settings;
  pageData[2] = batteryOk;
  pageData[3] = mode;
  result &= cardPageWrite(CARD_PAGE_START + 1, pageData, sizeof(pageData));
  // Записываем текущее время станции
  memset(pageData, 0, sizeof(pageData));
  pageData[0] = (t.unixtime & 0xFF000000)>>24;
  pageData[1] = (t.unixtime & 0x00FF0000)>>16;
  pageData[2] = (t.unixtime & 0x0000FF00)>>8;
  pageData[3] = (t.unixtime & 0x000000FF);
  result &= cardPageWrite(CARD_PAGE_START + 2, pageData, sizeof(pageData));
  // Записываем время пробуждения станции
  memset(&t, 0, sizeof(t));
  t.sec = bcdtodec(DS3231_get_addr(0x07));
  t.min = bcdtodec(DS3231_get_addr(0x08));
  t.hour = bcdtodec(DS3231_get_addr(0x09));
  t.mday = bcdtodec(DS3231_get_addr(0x0A));
  t.mon = alarmMonth;
  t.year = alarmYear;
  t.unixtime = get_unixtime(t);
  pageData[0] = (t.unixtime & 0xFF000000)>>24;
  pageData[1] = (t.unixtime & 0x00FF0000)>>16;
  pageData[2] = (t.unixtime & 0x0000FF00)>>8;
  pageData[3] = (t.unixtime & 0x000000FF);
  result &= cardPageWrite(CARD_PAGE_START + 3, pageData, sizeof(pageData));
  // Чтобы отличить звук заряженной батареи от звука об ошибке
  Watchdog.reset();
  delay(1000);

  if(result)
    BEEP_MASTER_CARD_GET_INFO_OK;
  else
    BEEP_MASTER_CARD_GET_INFO_ERROR;
}

void processParticipantCard(uint16_t cardNum)
{
  uint8_t lastNum = 0;
  uint8_t newPage = 0;
  bool checkOk = false;

  if(cardNum)
  {
    //Ищем последнюю пустую страницу в чипе для записи
    findNewPage(&newPage, &lastNum);
  
    if(newPage >= CARD_PAGE_START && newPage <= CARD_PAGE_MAX)
    {
      if(lastNum != stationNum)
      {
        /*
         * если включена функция старта-финиша. То станция старта принимает только пустые чипы
         * все остальные станции принимают только не пустые чипы
         * после станции финиша на чип нельзя записать отметку
         */
        checkOk = true;
        if(settings & SETTINGS_CHECK_START_FINISH)
        {
          if(newPage == CARD_PAGE_START && stationNum != START_STATION_NUM)
            checkOk = false;
          else if(stationNum == START_STATION_NUM && newPage != CARD_PAGE_START)
            checkOk = false;
          else if(lastNum == FINISH_STATION_NUM)
            checkOk = false;
          else if(stationNum == FINISH_STATION_NUM && newPage == CARD_PAGE_START)
            checkOk = false;
        }

        if(settings & SETTINGS_CHECK_CARD_TIME)
        {
          checkOk = !doesCardExpire();
        }

        // Записываем отметку
        if(checkOk)
        {
          if(writeMarkToParticipantCard(newPage))
          {
            // Записывааем номер чипа во внутреннюю память
            writeCardNumToLog(cardNum);
              
            BEEP_CARD_MARK_WRITTEN;
          }
        }
      }
      else
      {
        checkOk = true;
        BEEP_CARD_MARK_OK;
      }
    }
  }

  if(!checkOk)
    BEEP_CARD_MARK_ERROR;
}

void findNewPage(uint8_t *newPage, uint8_t *lastNum)
{
  uint8_t startPage = CARD_PAGE_START;
  uint8_t endPage = CARD_PAGE_MAX;
  uint8_t page;
  byte pageData[18];
  byte dataSize;
  byte num;

  *newPage = 0;
  *lastNum = 0;

  while(startPage < endPage)
  {   
    page = (startPage + endPage)/2;

    dataSize = sizeof(pageData);
    if(!cardPageRead(page, pageData, &dataSize))
      return;

    num = pageData[0];
     
    if(num == 0)
      endPage = page;
    else
      startPage = (startPage != page)? page : page + 1;
  }

  if(num > 0)
    page++;

  *newPage = page;
  *lastNum = num;
}

bool writeMarkToParticipantCard(uint8_t newPage)
{
  byte pageData[16];
  byte dataSize = sizeof(pageData);
  
  // Читаем текущее время
  DS3231_get(&t);

  pageData[0] = stationNum;
  pageData[1] = (t.unixtime & 0x00FF0000)>>16;
  pageData[2] = (t.unixtime & 0x0000FF00)>>8;
  pageData[3] = (t.unixtime & 0x000000FF);
      
  return cardPageWrite(newPage, pageData, dataSize);
}

/**
 * Функция очистки карточки участника
 */
void clearParticipantCard()
{
  byte pageData[16];
  byte dataSize = sizeof(pageData);
  bool result = true;

  memset(pageData, 0, dataSize);

  for(uint8_t page = CARD_PAGE_INIT_TIME; page <= CARD_PAGE_MAX; page++)
  {
    Watchdog.reset();
    delay(50);
    
    digitalWrite(LED,HIGH);
    
    result &= cardPageWrite(page, pageData, dataSize);
    
    digitalWrite(LED,LOW);
  }

  if(result)
  {
    DS3231_get(&t);
    
    pageData[0] = (t.unixtime&0xFF000000)>>24;
    pageData[1] = (t.unixtime&0x00FF0000)>>16;
    pageData[2] = (t.unixtime&0x0000FF00)>>8;
    pageData[3] = (t.unixtime&0x000000FF);

    result &= cardPageWrite(CARD_PAGE_INIT_TIME, pageData, dataSize);
  }

  if(result)
    BEEP_CARD_CLEAR_OK;
  else
    BEEP_CARD_CLEAR_ERROR;
}

void checkParticipantCard()
{
  byte pageData[18];
  byte dataSize = sizeof(pageData);
  uint16_t cardNum = 0;
  uint8_t newPage = 0;
  uint8_t lastNum = 0;
  bool result = false;
  
  if(cardPageRead(CARD_PAGE_INFO, pageData, &dataSize))
  {
    // Проверяем номер чипа 
    cardNum = (((uint16_t)pageData[0])<<8) + pageData[1];
    if(cardNum > 0 && pageData[2] != 0xFF)
    {
      // Проверяем количество отметок на чипе
      findNewPage(&newPage, &lastNum);
      if(newPage == CARD_PAGE_START && lastNum == 0)
      {
        result = true;
        // Проверяем время инициализации чипа
        if(settings & SETTINGS_CHECK_CARD_TIME)
          result = !doesCardExpire();
      }
    }
  }

  if(result)
    BEEP_CARD_CHECK_OK;
  else
    BEEP_CARD_CHECK_ERROR;
}

bool doesCardExpire()
{
  byte pageData[18];
  byte dataSize = sizeof(pageData);
  uint32_t cardTime = 0;
  bool result = true;
  
  if(cardPageRead(CARD_PAGE_INIT_TIME, pageData, &dataSize))
  {
    DS3231_get(&t);

    cardTime = (((uint32_t)pageData[0]) & 0xFF000000)<<24;
    cardTime |= (((uint32_t)pageData[1]) & 0x00FF0000)<<16;
    cardTime |= (((uint32_t)pageData[2]) & 0x0000FF00)<<8;
    cardTime |= (((uint32_t)pageData[3]) & 0x000000FF);

    if(t.unixtime - cardTime >= CARD_EXPIRE_TIME)
      result = true;
    else
      result = false;
  }

  return result;
}

void rtcAlarmIrq()
{
  // Мы не можем обработать здесь прерывание
  // Потому что это повесит процессор из-за операций с I2C
  // Поэтому устанвливаем флаг и обрабатываем прерывание в основном цикле
  rtcAlarmFlag = 1;
}
