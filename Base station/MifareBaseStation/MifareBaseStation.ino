
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Wire.h>
#include <ds3231.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

//Константы и перменные

const uint8_t vers = 103; //version of software
const uint16_t eepromMaxChip = 4000; //16Kb, default in ds3231 - 4Kb

const uint8_t LED = 4; // led diod pin
const uint8_t BUZ = 3; // buzzer pin
const uint8_t VCC_C = 5; // Pin for powering of the clock during their reading
const uint8_t RST_PIN = 9; //Reset pin of MFRC522, LOW for sleep
const uint8_t SS_PIN = 10; //SS pin of RFID
const uint16_t eepromAdrStantion = 800;//start address for storing number of the stantion
const uint16_t eepromAdrSleep = 900;//start address for storing sleep state
const uint16_t eepromPass = 850;//start address for storing password and settings

const uint8_t pageCC = 3;
const uint8_t blockInfo = 4; //block for information about participant and last stantion
const uint8_t pagePass = 5;

const uint8_t ntag214 = 4;
const uint8_t ntagValue = 50;

const uint8_t startStantion = 240; //number of start stantion
const uint8_t finishStantion = 245;//number of finish stantion
const uint8_t clearStantion = 249; //number of clear station
const uint8_t checkStantion = 248;
const uint16_t maxNumChip = 1500;//for EEPROM write. If you exceed, the mark will be made, but the information in EEPROM will not be recorded

//master chips number in the first three bytes of the chip in the first page
const uint8_t timeMaster = 250;
const uint8_t numberMaster = 251;
const uint8_t sleepMaster = 252;
const uint8_t dumpMaster = 253;
const uint8_t passMaster = 254;


/*
 * После запуска или перезагрузки станция считывает показание часов
 * Если они сбиты, то издает три коротких звуковых сигнала
 * Запоминает промежуточное время temph, чтобы вовремя перейти в режим ожиданиния при бездействии
 * 
 * Далее присходит счтиывание из EEPROM памяти настроек станций:
 * - номера станции
 * - состояние сна
 * - паролей мастер-чипа
 * - байта настроек работы станции
 * 
 * Затем станция выжидает 5 секунд и после длинного сигнала выходит в цикл loop
 */

uint8_t pass[3] = {0,0,0}; //pass for reading master chip
uint8_t setting = 0;

uint8_t stantion = 0; //number of station. default 0, can not be written to the chip
boolean startFinish = false;
boolean work = false; // Default station in standby mode, true - the operating mode
boolean night = false; // When triggered watchdog if the station does not sleep, there is a reboot
boolean deepsleep = false; //On / off stantion. In off regime sleep 8 s
boolean checkTimeInit = false; //реагирование станций на чипы с просроченой инициализацией
uint32_t maxTimeInit = 2500000UL; //

uint32_t loopCount = 0; //86400, 691200 - 6, 48 hour work regime
uint32_t maxCount = 86400UL; //loops before switching to standby mode

uint8_t lastCleanChip0 = 0;
uint8_t lastCleanChip1 = 0;
boolean lastChipClean = false;

MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
struct ts t; //time


void setup () {

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  //We read the time. if huddled hours, signaling
  
  pinMode(VCC_C, OUTPUT);
  digitalWrite(VCC_C, HIGH);
  delay(1);
  DS3231_init(DS3231_INTCN);
  DS3231_get(&t);
  digitalWrite(VCC_C, LOW);
  
  
  if (t.year < 2017) {
    //Error: watch wrong
    beep(50, 3);
  }

  set_sleep_mode (SLEEP_MODE_PWR_DOWN); // the most power save sleep mode
  
  stantion = eepromread(eepromAdrStantion); //Read the station number from the EEPROM
  deepsleep = eepromread(eepromAdrSleep); //Read on/off mark from EEPROM
  for (uint8_t i=0; i<3;i++){
    pass[i]=eepromread(eepromPass + i*3);
  }
  setting = eepromread(eepromPass + 9);
  if (pass[0]==255){
    pass[0]=pass[1]=pass[2]=setting=0;
  }

  uint8_t timeidl = setting&0b00000011;
  if (timeidl == 0b00000000) maxCount = 86400UL;
  if (timeidl == 0b00000001) maxCount = 691200UL;
  if (timeidl == 0b00000010) maxCount = 0;
  if (timeidl == 0b00000011) maxCount = 1;
  
  
  uint8_t set2 = setting&0b00000100;
  if (set2 == 0b00000100) startFinish = true;
  else startFinish = false;
  
  uint8_t set3 = setting&0b00001000;
  if (set3 == true) checkTimeInit = true;
 
  delay(5000); //is necessary to to reflash station. in sleep mode it is not possible.
  beep(800, 1 ); //The signal at system startup or reboote
}//end of setup


/*
 * В цикле работы станции. Сначала включается вотчдог на 1 секунду.
 * Если какой-либо процесс затянется на время большее, то приозойдёт перезагрузка станции
 * 
 * Затем проверяется время. Не пора ли переключаться из рабочего режима в режим ожидания при бездействии
 * 
 * Далее вызывается функция работы с чипами - rfid()
 * 
 * После опроса чипа станция проверяет режим работы - ожидания или рабочий. И соответственно входит в сон на разное время
 * В настройках можно настроить worktime таким образом, чтобы станция была постоянно в одном режиме.
 * 
 * Если станция в режиме сна, то происходит её усыпление на 24 секунды.
 * 
 */
void loop ()
{
  
  //enable watch-dog for 1 s
  wdt_enable(WDTO_1S);

  if (work){
    loopCount ++;
    if (loopCount > maxCount) {
      work = false;
      loopCount = 0;
    }
  }

  
  //Look the chip, If founded, writing it
  rfid();
 
  if (maxCount == 0) work = false;
  if (maxCount == 1) work = true;
  
  //Leaving to sleep for 250 ms or 1000 ms in the case of inactivity for more than 6 hours
  sleep(work);
  

  //Power down regime: sleep 24 s
  if (deepsleep) {
    for (uint8_t i = 0; i<3; i++){
      sleep8s();
    }
  }

} // end of loop


/*
 * Фунцкия программной перезагрузки
 */
void(* resetFunc) (void) = 0; // Reset MC function

/*
 * Настройка перервания по вотчдогу. 
 * Если прерывание вызвано не сном, то станция перезагружается
 */
// watchdog interrupt
ISR (WDT_vect)
{
  wdt_disable();  // disable watchdog
  if (!night) {
    resetFunc();
  }

}  // end of WDT_vect


/*
 * Вход в сон на 250 или 1000 мс. Сначала ставится флаг для прерывания, что оно приоисходит
 * в следствии сна. Затем присходит выставление всех пинов на OUTPUT и все оин кладустся на землю
 * Выключается АДЦ для большей экономии энергии. Затем настраиваются биты перехода в сон и засыпление станции
 * После просыпления снимается флаг сна для обработки прерываний
 */
//Entrance to sleep for 250 or 1000 ms
void sleep(boolean light) {
  night = true; //for correct work of watch-dog inerrupt

  //Low all pin for power save
  for (uint8_t i = 0; i <= A5; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite (i, LOW);  //
  }

  // disable ADC
  ADCSRA = 0;
  if (light) {
    MCUSR = 0;
    // allow changes, disable reset
    WDTCSR = bit (WDCE) | bit (WDE);
    // set interrupt mode and an interval
    WDTCSR = bit (WDIE) | bit (WDP2);    // set WDIE, and 0.25 second delay
    wdt_reset();  // pat the dog
  }
  if (!light) {
    WDTCSR = bit (WDCE) | bit (WDE);
    // set interrupt mode and an interval
    WDTCSR = bit (WDIE) | bit (WDP2) | bit (WDP1) ;    // set WDIE, and 1 second delay
    wdt_reset();  // pat the dog
  }

  noInterrupts ();           // timed sequence follows
  sleep_enable();

  // turn off brown-out enable in software
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS);
  interrupts ();             // guarantees next instruction executed
  sleep_cpu ();

  // cancel sleep as a precaution
  sleep_disable();
  night = false;

} // end of sleep()


/*
 * Вход в сон на 8000 мс. Сначала ставится флаг для прерывания, что оно приоисходит
 * в следствии сна. Затем присходит выставление всех пинов на OUTPUT и все оин кладутся на землю
 * Выключается АЦП для большей экономии энергии. Затем настраиваются биты перехода в сон и засыпление станции
 * После просыпления снимается флаг сна для обработки прерываний
 */
void sleep8s() {
  night = true; //for correct work of watch-dog inerrupt

  //Low all pin for power save
  for (uint8_t i = 0; i <= A5; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite (i, LOW);  //
  }
 
  // disable ADC
  ADCSRA = 0;
  MCUSR = 0;
  // allow changes, disable reset
  WDTCSR = bit (WDCE) | bit (WDE);
  // set interrupt mode and an interval
  WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);    // set WDIE, and 8 second delay
  wdt_reset();  // pat the dog

  noInterrupts ();           // timed sequence follows
  sleep_enable();

  // turn off brown-out enable in software
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS);
  interrupts ();             // guarantees next instruction executed
  sleep_cpu ();

  // cancel sleep as a precaution
  sleep_disable();
  night = false;

} // end of sleep8s()


/*
 * Функция записи во внутреннюю память микроконтроллера
 * Запись приосходит с мажоритарным резервированием в три подряд ячейки
 */

//write to EEPROM 3 value from adress to adress+2
void eepromwrite (uint16_t adr, uint8_t val) {
  for (uint8_t i = 0; i < 3; i++) {
    EEPROM.write(adr + i, val);
  }
}

/*
 * Считывание ячейки из внутренней памяти МК с учетом мажоритарного резервирования
 */

//Getting info from the EEPROM
uint8_t eepromread(uint16_t adr) {
  if (EEPROM.read(adr) == EEPROM.read(adr + 1) ||
      EEPROM.read(adr) == EEPROM.read(adr + 2)) {
    return EEPROM.read(adr);
  }
  else if (EEPROM.read(adr + 1) == EEPROM.read(adr + 2)) {
    return EEPROM.read(adr + 1);
  }
  else {
    beep(50, 3);
    return 0;
  }

} // end of eepromstantion

/*
 * Запись ячейки соответствующей номеру чипа во внутреннюю память. Только факт отметки.
 */

// write number of chip to eeprom
void writeNumEeprom (uint16_t num){
  uint16_t byteAdr = num/8;
  uint16_t bitAdr = num%8;
  uint8_t eepromByte = EEPROM.read(byteAdr);
  bitSet(eepromByte, bitAdr);
  EEPROM.write(byteAdr, eepromByte);
}

/*
 * Очистка внутренней памяти чипа
 * адреса 0 - 750
 */

//clean eeprom from number chip
void cleanEeprom (){
  for (uint16_t a = 0; a <750; a++){
    if (a%100==0) wdt_reset();
    EEPROM.write(a,0);
    delay(5);
  }
}


/*
 * Выдача сигнала. Принимает продолжительность сигнала и число сигналов подряд
 * В ходе работы сбрасывает вотчдог, чтобы не произошла перезагрузка
 */

// led and buzzer signal
void beep(uint16_t ms, uint8_t n) {

  pinMode (LED, OUTPUT);
  pinMode (BUZ, OUTPUT);

  for (uint8_t i = 0; i < n; i++) {
    digitalWrite (LED, HIGH);
    tone (BUZ, 4000, ms);
    delay (ms);
    wdt_reset();
    digitalWrite (LED, LOW);
    if (i < n - 1) {
      delay(ms);
      wdt_reset();
    }
  }

} //end of beep

/*
 * Функция считывания напряжения питания МК. 
 * Сравнение происходит по внутреннему источнику опроного напряжения в 1.1 В
 */

// voltmeter on
uint32_t  readVcc(uint32_t constanta) {
  ADCSRA |=  bit (ADEN);   // turn ADC on
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);


  delay(75); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  uint32_t result = (high << 8) | low;

  result = constanta / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

/*
 * Измерение напряжения. Включает диод на 5 секунд. Затем происходит измерение.
 * Если напряжение меньше 3.1 В, то станция выдает три длинные сигнала. Если больше, то один.
 */

//measuring voltage of battery and make signal.
void voltage() {
  wdt_enable(WDTO_8S);
  wdt_reset();

  const uint32_t constant = 1125300L; //voltage constanta

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  delay(5000);

  uint32_t value = 0;
  for (uint8_t i = 0; i < 10; i++) {
    value += readVcc(constant);
  }
  value /= 10;


  if (value < 3100) {
    beep(1000, 3);
  }
  else {
    beep(1000, 1);
  }
  digitalWrite(LED, LOW);
  wdt_reset();

}//end of voltage

uint8_t dump[16];

//MFRC522::StatusCode MFRC522::MIFARE_Read
bool ntagWrite (uint8_t *data, uint8_t pageAdr){
  byte blockAddr = pageAdr-3 + ((pageAdr-3)/3);

  byte dataBlock[16];
  dataBlock[0]=data[0];
  dataBlock[1]=data[1];
  dataBlock[2]=data[2];
  dataBlock[3]=data[3];
  
  byte buffer[18];
  byte size = sizeof(buffer);
  byte trailerBlock = blockAddr + (3-blockAddr%4);
  
  // Authenticate using key A
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
      return false;
  }

  // Write data to the block
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
     
     return false;
  }
  
  // Read data from the block (again, should now be what we have written)
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
      
      return false;
  }
 
  for (uint8_t i = 0; i < 4; i++) {
    dump[i]=buffer[i];
  }

  if (dump[0]==dataBlock[0]){
    return true;
  }
  else{
    return false;
  }
    
}

bool ntagRead (uint8_t pageAdr){
  byte blockAddr = pageAdr-3 + ((pageAdr-3)/3);
  
  byte buffer[18];
  byte size = sizeof(buffer);
  byte trailerBlock = blockAddr + (3-blockAddr%4);
   // Authenticate using key A
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
      return false;
  }

 
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
      
      return false;
  }
 
  byte count = 0;
  for (byte i = 0; i < 16; i++) {
      dump[i]=buffer[i];
      // Compare buffer (= what we've read) with dataBlock (= what we've written)
 }

 if (pageAdr==3){
      dump[2]=0;
 }
 
 return true;
   
}



/*
 * Проверка не прошли более чем время заданное в worktime с момента последней успешной записи чипа.
 */

//get time
void getTime(){
  
  digitalWrite(VCC_C, HIGH);
  delay(1);
  
  DS3231_get(&t);
  digitalWrite(VCC_C, LOW);

}

/*
 * Основная функция работы с чипами
 */

uint8_t tempDump[4] = {255,0,0,0};

//Writing the chip
void rfid() {
  //инициализируем переменные
  uint8_t lastNum = 0; //last writed number of stantion in memory of chip
  uint8_t newPage = 0;
  uint16_t chipNum = 0; //number of chip from 1-st block
  uint16_t partNumFirst =0; //part of number
  uint16_t partNumSecond = 0; //part of number

  
  //включаем SPI ищем карту вблизи. Если не находим выходим из функции чтения чипов
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();    // Init MFRC522
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  getTime();
 
  //просываемся, если станция была в спяящем режиме. Измеряем напряжение на батарее
  if (deepsleep == true) {
    deepsleep = false;
    eepromwrite (eepromAdrSleep, 0);
    voltage();
    resetFunc();
  }
   
  //читаем блок информации
  if(!ntagRead(blockInfo)){
    return;
  }
  
  //в первых трех байтах находятся нули для обычных чипов и заданные числа для мастер-чипов
  uint8_t info = 0;
  if (dump[2]==255) {
    uint8_t tempdump[16];
    tempdump[0]=dump[0];
    tempdump[1]=dump[1];
    tempdump[2]=dump[2];
    tempdump[3]=dump[3];

    if(!ntagRead(blockInfo+1)){
      return;
    }

    tempdump[4]=dump[0];
    tempdump[5]=dump[1];
    tempdump[6]=dump[2];
    tempdump[7]=dump[3];

    if(!ntagRead(blockInfo+2)){
      return;
    }

    tempdump[8]=dump[0];
    tempdump[9]=dump[1];
    tempdump[10]=dump[2];
    tempdump[11]=dump[3];

    if(!ntagRead(blockInfo+3)){
      return;
    }

    tempdump[12]=dump[0];
    tempdump[13]=dump[1];
    tempdump[14]=dump[2];
    tempdump[15]=dump[3];

    for (uint8_t h=0;h<16;h++){
      dump[h]=tempdump[h];
    }
    
    info = dump[1];
    //считываем пароль с мастер-чипа
    uint8_t chipPass[3];
    chipPass[0] = dump[4];
    chipPass[1] = dump[5];
    chipPass[2] = dump[6];

    //сверяем пароль. Если не подходит - пищим и выходим
    if ((pass[0] != chipPass[0])||
        (pass[1] != chipPass[1])||
        (pass[2] != chipPass[2])){
      beep(50,3);
      return;
    }
    //вызов функций соответствующим мастер-чипам
    if (info == timeMaster) timeChip();
    else if (info == numberMaster) stantionChip();
    else if (info == sleepMaster) sleepChip();
    else if (info == dumpMaster) dumpChip();
    else if (info == passMaster) passChip();
    return; 
  }

  if (stantion == clearStantion){
    clearChip();
    return;
  }

  if (stantion == checkStantion){
    checkChip();
    return;
  }
  
  if (checkTimeInit == true){
  
    uint32_t timeInit = dump[4];
    timeInit = timeInit <<8;
    timeInit += dump[5];
    timeInit = timeInit <<8;
    timeInit += dump[6];
    timeInit = timeInit <<8;
    timeInit += dump[7];

    if (t.year>2016 && (t.unixtime-timeInit)> maxTimeInit){
      return;
    }
  }
  

  tempDump[0] = 0;
  
  
  //считыввание номер с чипа. Склеивание старшего и младшего байта в номер.
  chipNum = (dump[0]<<8) + dump[1];
  if (chipNum==0) return;

  
  uint8_t ntagType = dump[2]%10;

  newPage = findNewPage(50);
 

  //ищем последнюю пустую страницу в чипе для записи
  
  //если поиск вышел неудачным, функция возвращает ноль и выходит
  
  if (newPage == 0) return;

  //во время поиска последнюю записанную страницу поместили в tempDump. Считываем из неё номер, чтобы убедится, что последняя записанная станция отличается 
  lastNum = tempDump[0];
  if (stantion == lastNum){
    beep(500,1);
    return;
  }
  
  //проверяем записан ли чип уже полностью и места на нём нет
  //check if memory of chip has finished
  uint8_t maxPage = 49;
  
  if(newPage > maxPage){
    return;
  }

  /*
  * если включена функция старта-финиша. То станция старта принимает только пустые чипы
  * все остальные станции принимают только не пустые чипы
  * после станции финиша на чип нельзя записать отметку
  */
  uint8_t startPage = 8;
  if (startFinish){
    //first section can be write just by start stantion
    if (newPage == startPage){
      if (stantion!=startStantion){
        return;
      }
    }
    //if chip has finished it isnt be written
    if (lastNum == finishStantion){
      return;
    }
    //if chip not empty it can be started
    if (stantion == startStantion){
      if (newPage!=startPage){
        return;
      }
    }
  }
  //Записываем отметку
  //write time mark
  if (!writeTime(newPage)){
    return;
  }
  //записывааем номер чипа во внутреннюю память и если есть внешняя память, записываем туда время
  //write num of chip to eeprom
  if ((chipNum<maxNumChip)&&(chipNum>0)){
    writeNumEeprom (chipNum);
  }

  //переходим в рабочий режим
  work = true;
  loopCount = 0;

  // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
  SPI.end();

} // end of rfid()

/*
 * функция обработки мастер-чипа времени
 * С чипа считыввается новое время и устанавливается 
 * внутреннее время. Станция пикает 5 раз и перезагружается
 */
void timeChip() {
    
  t.mon = dump[8];
  t.year = dump[9]+2000;
  t.mday = dump[10];
  t.hour = dump[12];
  t.min = dump[13];
  t.sec = dump[14];

  uint8_t dataDump[4] ={0, 0, 0,0};
  
  if(!ntagWrite(dataDump,4)){
    beep(50,3);
    return;
  }
  
  digitalWrite(VCC_C,HIGH);
  delay(1);
  DS3231_set(t); //correct time
  digitalWrite(VCC_C,LOW);
  
  beep(300,5);
  resetFunc(); //reboot
}

/*
 * Функция установки нового номера станции
 * станция считывает чип, записывает новый номер
 * во внутреннюю память, пикает 5 раз и перезагружается
 */

void stantionChip(){
  
  uint8_t newnum = dump[8];
  uint8_t dataDump[4] ={0, 0, 0, 0};
  
  if(!ntagWrite(dataDump,4)){
    beep(50,3);
    return;
  }
    
  if ((stantion != newnum)&&(newnum!=0)){
    stantion = newnum;
    eepromwrite (eepromAdrStantion, newnum);
    beep(300,5);
    resetFunc(); //reboot
  }
  else{
    beep(50,3);
    return;
  }
}

/*
 * Функция обработки мастер-чипа сна. 
 * Станция стирает данные о пароле и настройках,
 * пикает три раза и входит в сон
 */
void sleepChip(){
  
  uint8_t dataDump[4] ={0, 0, 0, 0};
  
  if(!ntagWrite(dataDump,4)){
    beep(50,3);
    return;
  }
  for (uint8_t i = 0;i<3;i++){
    pass[i]=0;
    eepromwrite((eepromPass+i*3),0);
  }
  
  deepsleep = true;
  eepromwrite (eepromAdrSleep, 255); //write sleep mode to EEPROM in case of failures
  
  beep(100,3);

  cleanEeprom();

  resetFunc(); //reboot
}

/*
 * Функция записи дамп-чипа. Станция считывает все данные по чипам из внутренней памяти
 * и записывает их последовательно на дамп-чип. После чего один раз пикает и выходит.
 */
void dumpChip(){
  uint8_t dataEeprom[4];
  uint16_t eepromAdr = 0;

  uint8_t dataDump[4] = {stantion,0,0,0};
  
  if(!ntagWrite(dataDump,4)){
    beep(50,3);
    return;
  }

  for (uint8_t page = 5; page<50;page++){
  wdt_reset();  
    for (uint8_t m = 0; m<4;m++){
      dataEeprom[m]=EEPROM.read(eepromAdr);
      eepromAdr++;
    }

    if(!ntagWrite(dataEeprom,page)){
      beep(50,3);
      return;
    }
    
  }

  beep(500,1);
  return;
  
}

/*
 * Функция записи отметки в чип. 
 * Записывает номер и поседние 3 байта юникстайм в чип. Если удалось, пикает и выдает true
 */
bool writeTime(int newPage){

   uint32_t code = stantion;
   code = code<<24;
   code += (t.unixtime&0x00FFFFFF);

   uint8_t toWrite[4] = {0,0,0,0};
   toWrite[0]=(code&0xFF000000)>>24;
   toWrite[1]=(code&0x00FF0000)>>16;
   toWrite[2]=(code&0x0000FF00)>>8;
   toWrite[3]=(code&0x000000FF);
   
   uint8_t dataBlock2[4] = {toWrite[0],toWrite[1],toWrite[2],toWrite[3]};
   
   if (ntagWrite(dataBlock2,newPage)){
     beep(200, 1);
     return true;
   }
   else {
     return false;
   }
}

/*
 * Функция обработки мастер-чипа смены пароля. Станция считывает новый пароль и байт настроек. Записывает его в память.
 * Пикает два раза и перезагружается.
 */

void passChip(){
  
  for (uint8_t i = 0;i<3;i++){
    pass[i]=dump[i+8];
    eepromwrite((eepromPass+i*3),pass[i]);
  }
  setting = dump[11];
  eepromwrite(eepromPass+9,setting);

  uint8_t dataDump[4] ={0, 0, 0, 0};
  
  if(!ntagWrite(dataDump,4)){
    return;
  }
  
  beep(300,2);
  resetFunc(); //reboot
  
}


/*
 * функция поиска последней записанной страницы по алгоритму бинарного поиска.
 */

uint8_t findNewPage(uint8_t finishpage){
  uint8_t startpage = 8;
  uint8_t page = (finishpage+startpage)/2;

  while (1) {
    if (finishpage==startpage) {
      return (finishpage);
    }
       
    page = (finishpage + startpage)/2;
   
    if(!ntagRead(page)){
      for (uint8_t i = 0; i<4 ; i++) tempDump[i] = 0;
      return 0;
    }
     
    if (dump[0]==0){
      finishpage = (finishpage - startpage)/2 + startpage;
    }
    else {
      for (uint8_t i = 0; i<4 ; i++) tempDump[i] = dump[i];
      startpage = finishpage - (finishpage - startpage)/2;
    }
  } 
}

void clearChip(){
  
  
  if ((lastCleanChip0==dump[0])&&(lastCleanChip1==dump[1])&&lastChipClean){
    beep(500, 1 );
    return;
  }
  lastChipClean = false;
  lastCleanChip0 = dump[0];
  lastCleanChip1 = dump[1];
  
  pinMode (LED, OUTPUT);
  digitalWrite (LED,HIGH);
  
  byte Wbuff[] = {255,255,255,255};
  
  for (byte page = 8; page < ntagValue; page++) {
    wdt_reset();
    if (!ntagWrite(Wbuff,page)) {
      return;
    }
  }

  byte Wbuff2[] = {0,0,0,0};
  
  for (byte page = 8; page < ntagValue; page++) {
    wdt_reset();
    if (!ntagWrite(Wbuff2, page)) {
      return;
    }
  }

   uint32_t utime = t.unixtime;
   
   uint8_t dataBlock2[4] = {0,0,0,0};
   dataBlock2[0]=(utime&0xFF000000)>>24;
   dataBlock2[1]=(utime&0x00FF0000)>>16;
   dataBlock2[2]=(utime&0x0000FF00)>>8;
   dataBlock2[3]=(utime&0x000000FF);
  
  if(!ntagWrite(dataBlock2, pagePass)) {
    return;
  }
  digitalWrite (LED,LOW);
  beep(500, 1 );
  
  lastChipClean = true;
}

void checkChip(){
  
  uint32_t initTime = dump[4];
  initTime <<= 8;
  initTime += dump[5];
  initTime <<= 8;
  initTime += dump[6];
  initTime <<= 8;
  initTime += dump[7];

  if ((t.unixtime-initTime)> maxTimeInit){
    beep(200,3);
    return;
  }

  uint8_t firstPage = 8;

  for (byte i =0;i<7;i++){
    if(!ntagRead(firstPage)){
      return;
    }  
  }
  
  for (byte i=0; i<16;i++){
    if (dump[i]!=0){
      beep(200,3);
      return;
    }
  }

  beep(200,1);
  
}

