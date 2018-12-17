// --------------- NRF24l01
#define HW_YU
// --------------- NRF24l01 ---

#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

const byte vers = 103; //version of software





// --------------- NRF24l01
bool rf24_online = false;
#ifdef HW_YU


struct strDataSendNRF
{ 
byte pageInit_0;
byte pageInit_1;
byte pageInit_2;
byte pageInit_3;

byte pagePass_0;
byte pagePass_1;
byte pagePass_2;
byte pagePass_3;

byte pageInfo1_0;
byte pageInfo1_1;
byte pageInfo1_2;
byte pageInfo1_3;

byte pageInfo2_0;
byte pageInfo2_1;
byte pageInfo2_2;
byte pageInfo2_3;
};
strDataSendNRF sendNRF;

 
const uint8_t LED = 4; // led diod pin
const uint8_t BUZ = 3; //pd3 buzzer pin
const uint8_t VCC_C = 5; //pc3 my
const uint8_t RST_PIN = 9; //Reset pin of MFRC522, LOW for sleep
const uint8_t SS_PIN = 10; //SS pin of RFID 

#define RF24_CSN    8 //  CS  на RF module
#define RF24_CE     2 //  CE  на RF module 
#define VCC_NRF   7   //  Vcc  на RF module 

#include "nRF24L01.h"
#include "RF24.h"


const uint8_t nCanalNRF24l01 = 112;      // Установка канала вещания;
byte addresses[][6] = {"1Master","2Base"};
RF24 radio(RF24_CE , RF24_CSN);

#else

// --------------- NRF24l01 ---


const uint8_t LED = 4; // led diod pin
const uint8_t BUZ = 3; // buzzer pin
const uint8_t VCC_C = 5; // Pin for powering of the clock during their reading
const uint8_t RST_PIN = 9; //Reset pin of MFRC522, LOW for sleep
const uint8_t SS_PIN = 10; //SS pin of RFID

// --------------- NRF24l01 
#endif
// --------------- NRF24l01 ---



//password for master key
uint8_t pass[] = {0,0,0};
uint8_t stantionConfig = 0;
const uint16_t eepromPass = 850;
const uint16_t eepromMaster = 870;

const byte pageCC = 3;
const byte pageInit = 4;
const byte pagePass = 5;
const byte pageInfo1 = 6;
const byte pageInfo2 = 7;

const uint8_t START_BYTE = 0xfe;

enum Error {
  ERROR_COM         = 0x01,
  ERROR_CARD_WRITE  = 0x02,
  ERROR_CARD_READ   = 0x03,
  ERROR_EEPROM_READ = 0x04
};


uint8_t ntagValue = 50;
uint8_t ntagType = 214;

uint8_t function = 0;

const uint8_t timeOut = 10;
const uint8_t packetSize = 32;
const uint8_t dataSize = 30;


MFRC522::MIFARE_Key key;
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::StatusCode status;

uint8_t serialBuffer[packetSize];
uint8_t dataBuffer[dataSize];

void setup() {
   // --------------- NRF24l01 
    pinMode(VCC_NRF, INPUT);
    rf24_online = false;
   // --------------- NRF24l01 ---
   
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.begin(9600);


  pass[0] = eepromread(eepromPass);
  pass[1] = eepromread(eepromPass+3);
  pass[2] = eepromread(eepromPass+6);
  stantionConfig = eepromread(eepromPass+9);
}

void loop() {
 //  Serial.print( "rf24_online"); Serial.println( rf24_online);
 //    Serial.print( "digitalRead(VCC_NRF)"); Serial.println( digitalRead(VCC_NRF));
// --------------- NRF24l01 
 if (digitalRead(VCC_NRF) and !rf24_online) { setupNRF();  }
 if (!digitalRead(VCC_NRF)) { rf24_online = false;}
// --------------- NRF24l01 ---
  static uint32_t lastReadCardTime = 0;
  
  if (Serial.available() > 0) {
    clearBuffer();
    
    Serial.readBytes(serialBuffer, packetSize);
    
    uint8_t sumAdr = serialBuffer[2] + 3;
    if (sumAdr > 28) {
      sumAdr = 31;
    }
       
    for (uint8_t i = 0; i < sumAdr - 1; i++) {
      dataBuffer[i]=serialBuffer[i+1];
    }

    uint8_t sum = checkSum(dataBuffer, sumAdr - 1);
    
    if (serialBuffer[0] != START_BYTE ||
        serialBuffer[sumAdr] != sum) {
      signalError(ERROR_COM);
    }
  else {
    findFunc();
   }
  }
}


uint8_t checkSum(const uint8_t *addr, uint8_t len) {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < len; i++) {
    sum += addr[i];
  }
  return sum;
}

uint8_t dataCount = 2;
uint8_t packetCount = 0;

/*
 * 
 */
void addData(uint8_t data, uint8_t func) {
  
   dataBuffer[dataCount] = data;
   
   if (dataCount == dataSize - 1) {
     sendData(func, packetCount + 32);
     packetCount++;
   }
   else {
     dataCount++;
   }
  
}

/*
 * 
 */
void sendData(uint8_t func, uint8_t leng){
  
  Serial.write(START_BYTE);
  
  dataBuffer[0] = func;
  dataBuffer[1] = leng - 2;

  uint8_t trueleng = leng;
  if (leng > 30) {
    trueleng = 30;
  }
  
  for (uint8_t w = 0; w < trueleng; w++) {
    Serial.write(dataBuffer[w]);
  }
  Serial.write(checkSum(dataBuffer, trueleng));

  clearBuffer();  
}



uint8_t dump[16];

bool ntagWrite (uint8_t *data, uint8_t pageAdr) {
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
    signalError(ERROR_CARD_WRITE);
    return false;
  }

  // Read data from the block (again, should now be what we have written)
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    signalError(ERROR_CARD_READ);
    return false;
  }

  for (uint8_t i = 0; i < 4; i++) {
    dump[i] = buffer[i];
  }

  if (dump[0] == dataBlock[0] &&
      dump[1] == dataBlock[1] &&
      dump[2] == dataBlock[2] &&
      dump[3] == dataBlock[3]) {
    return true;
  }
  else {
    return false;
  }
}


/*
 * 
 */


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
    signalError(ERROR_CARD_READ);
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
 * 
 */
void beep(int ms, byte n) {
  pinMode(LED, OUTPUT);
  pinMode(BUZ, OUTPUT);

  for (byte i = 0; i < n; i++){
    digitalWrite(LED, HIGH);
    tone(BUZ, 4000, ms);
    delay(ms);
    digitalWrite(LED, LOW);
    if (i < n - 1) {
      delay(ms);
    }
  }
} //end of beep

//write to EEPROM 3 value from adress to adress+2
void eepromwrite(uint16_t adr, uint8_t val) {
  for (uint8_t i = 0; i < 3; i++) {
    EEPROM.write(adr + i, val);
  }
}

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
    signalError(ERROR_EEPROM_READ);
    return 0;
  }

} // end of eepromstantion

/*
 * 
 */
void findFunc() {
  switch (serialBuffer[1]) {
    case 0x41:
      writeMasterTime();
      break;
    case 0x42:
      writeMasterNum();
      break;
    case 0x43:
      writeMasterPass();
      break;
    case 0x44:
      writeInit();
      break;
    case 0x45:
      writeInfo();
      break;
    case 0x46:
      getVersion();
      break;
    case 0x47:
      writeMasterLog();
      break;
    case 0x48:
      readLog();
      break;
    case 0x4B:
      readCard();
      break;
    case 0x4C:
      readRawCard();
      break;
    case 0x4E:
      writeMasterSleep();
      break;
    case 0x58:
      signalError(0);
      break;
    case 0x59:
      signalOK();
// --------------- NRF24l01
      if (digitalRead(VCC_NRF) and !rf24_online) { setupNRF();  }
// --------------- NRF24l01 ---
      break;
  }
}

/*
 * 
 */
void writeMasterTime() {

// --------------- NRF24l01
if(rf24_online){
sendNRF.pageInit_0 = 0;
sendNRF.pageInit_1 = 250;
sendNRF.pageInit_2 = 255;
sendNRF.pageInit_3 = vers;
sendNRF.pagePass_0 = pass[0];
sendNRF.pagePass_1 = pass[1];
sendNRF.pagePass_2 = pass[2];
sendNRF.pagePass_3 = 0;
sendNRF.pageInfo1_0 = dataBuffer[3];
sendNRF.pageInfo1_1 = dataBuffer[2];
sendNRF.pageInfo1_2 = dataBuffer[4];
sendNRF.pageInfo1_3 = 0;
sendNRF.pageInfo2_0 = dataBuffer[5];
sendNRF.pageInfo2_1 = dataBuffer[6];
sendNRF.pageInfo2_2 = dataBuffer[7];
sendNRF.pageInfo2_3 = dataBuffer[0];
TransmitNRF();
}
// --------------- NRF24l01 ---
  
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  
  byte dataBlock[4] = {0, 250, 255, vers};
  if(!ntagWrite(dataBlock, pageInit)) {
    return;
  }


  byte dataBlock2[] = {pass[0], pass[1], pass[2], 0};
  if(!ntagWrite(dataBlock2, pagePass)) {
    return;
  }

  byte dataBlock3[] = {dataBuffer[3], dataBuffer[2], dataBuffer[4], 0};
  if(!ntagWrite(dataBlock3, pageInfo1)){
    return;
  }

  byte dataBlock4[] = {dataBuffer[5], dataBuffer[6], dataBuffer[7], 0};
  if(!ntagWrite(dataBlock4, pageInfo2)) {
    return;
  }

  beep(500,3);
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  SPI.end();
}

/*
 * 
 */
void  writeMasterNum(){
  // --------------- NRF24l01
if(rf24_online){
sendNRF.pageInit_0 = 0;
sendNRF.pageInit_1 = 251;
sendNRF.pageInit_2 = 255;
sendNRF.pageInit_3 = vers;
sendNRF.pagePass_0 = pass[0];
sendNRF.pagePass_1 = pass[1];
sendNRF.pagePass_2 = pass[2];
sendNRF.pagePass_3 = 0;
sendNRF.pageInfo1_0 = dataBuffer[2];
sendNRF.pageInfo1_1 = 0;
sendNRF.pageInfo1_2 = 0;
sendNRF.pageInfo1_3 = 0;
TransmitNRF();
}
// --------------- NRF24l01 ---
  SPI.begin();          // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  byte dataBlock[4] = {0, 251, 255, vers};
  if(!ntagWrite(dataBlock, pageInit)) {
    return;
  }


  byte dataBlock2[] = {pass[0], pass[1], pass[2], 0};
  if(!ntagWrite(dataBlock2, pagePass)){
    return;
  }

  byte dataBlock3[] = {dataBuffer[2], 0, 0, 0};
  if(!ntagWrite(dataBlock3, pageInfo1)) {
    return;
  }

  signalOK();
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  SPI.end();
}

/*
 * 
 */
void writeMasterPass() {
    // --------------- NRF24l01
if(rf24_online){
sendNRF.pageInit_0 = 0;
sendNRF.pageInit_1 = 254;
sendNRF.pageInit_2 = 255;
sendNRF.pageInit_3 = vers;
sendNRF.pagePass_0 = dataBuffer[5];
sendNRF.pagePass_1 = dataBuffer[6];
sendNRF.pagePass_2 = dataBuffer[7];
sendNRF.pagePass_3 = 0;
sendNRF.pageInfo1_0 = dataBuffer[2];
sendNRF.pageInfo1_1 = dataBuffer[3];
sendNRF.pageInfo1_2 = dataBuffer[4];
sendNRF.pageInfo1_3 = stantionConfig;
TransmitNRF();
}
// --------------- NRF24l01 ---

  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  byte dataBlock[4] = {0,254,255,vers};
  if(!ntagWrite(dataBlock, pageInit)) {
    return;
  }


  byte dataBlock2[] = {dataBuffer[5],dataBuffer[6],dataBuffer[7],0};
  if(!ntagWrite(dataBlock2, pagePass)) {
    return;
  }

  pass[0] = dataBuffer[2];
  pass[1] = dataBuffer[3];
  pass[2] = dataBuffer[4];
  stantionConfig = dataBuffer[8];  
  
  eepromwrite(eepromPass, dataBuffer[2]);
  eepromwrite(eepromPass+3, dataBuffer[3]);
  eepromwrite(eepromPass+6, dataBuffer[4]);
  eepromwrite(eepromPass+9, dataBuffer[8]);  
  
  byte dataBlock3[] = {pass[0],pass[1],pass[2],stantionConfig};
  if(!ntagWrite(dataBlock3, pageInfo1)) {
    return;
  }
  
  signalOK();
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  SPI.end();
}

/*
 * 
 */
void writeInit() {
  SPI.begin();          // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  if(!ntagRead(pageCC)) {
    return;
  }

  byte Wbuff[] = {255,255,255,255};
  
  for (byte page = 4; page < ntagValue; page++) {
    if (!ntagWrite(Wbuff,page)) {
      return;
    }
  }

  byte Wbuff2[] = {0,0,0,0};
  
  for (byte page=4; page < ntagValue; page++) {
    if (!ntagWrite(Wbuff2, page)) {
      return;
    }
  }

  byte dataBlock[4] = {dataBuffer[2],dataBuffer[3],ntagType,vers};
  if(!ntagWrite(dataBlock, pageInit)) {
     return;
  }


  byte dataBlock2[] = {dataBuffer[4],dataBuffer[5],dataBuffer[6],dataBuffer[7]};
  if(!ntagWrite(dataBlock2, pagePass)) {
    return;
  }

  byte dataBlock3[] = {dataBuffer[8],dataBuffer[9],dataBuffer[10],dataBuffer[11]};
  if(!ntagWrite(dataBlock3, pageInfo1)) {
    return;
  }

  byte dataBlock4[] = {dataBuffer[12],dataBuffer[13],dataBuffer[14],dataBuffer[15]};
  if(!ntagWrite(dataBlock4, pageInfo2)) {
    return;
  }
  
  signalOK();
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  SPI.end();
}

/*
 * 
 */
void writeInfo(){
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  byte dataBlock[4] = {dataBuffer[2],dataBuffer[3],dataBuffer[4],dataBuffer[5]};
  if(!ntagWrite(dataBlock, pageInfo1)) {
    return;
  }


  byte dataBlock2[] = {dataBuffer[6],dataBuffer[7],dataBuffer[8],dataBuffer[9]};
  if(!ntagWrite(dataBlock2, pageInfo2)) {
    return;
  }

  signalOK();
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();

  SPI.end();
}

/*
 * 
 */
void writeMasterLog(){


  
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  byte Wbuff[] = {255,255,255,255};
  
  for (byte page=4; page < ntagValue; page++){
    if (!ntagWrite(Wbuff, page)) {
      return;
    }
  }

  byte Wbuff2[] = {0,0,0,0};
   
  for (byte page=4; page < ntagValue; page++){
    if (!ntagWrite(Wbuff2, page)) {
      return;
    }
  }

  byte dataBlock[4] = {0, 253, 255, vers};
  if(!ntagWrite(dataBlock, pageInit)) {
    return;
  }

  byte dataBlock2[] = {pass[0], pass[1], pass[2], 0};
  if(!ntagWrite(dataBlock2, pagePass)) {
    return;
  }

  signalOK();

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  SPI.end();
}

/*
 * 
 */
void readLog() {
  function = 0x61;
  clearBuffer();
  
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  if(!ntagRead(pageInit)) {
    return;
  }

  addData(0, function);
  addData(dump[0], function);

  for (uint8_t page = 5; page < ntagValue; page++) {
    if (!ntagRead(page)) {
      return;
    }

    for (uint8_t i = 0; i < 4; i++) {
      for (uint8_t y = 0; y < 8; y++) {
        uint8_t temp = dump[i];
        temp = temp >> y;
        if (temp%2 == 1) {
        
          uint16_t num = (page - 5)*32 + i*8 + y;
          uint8_t first = (num&0xFF00)>>8;
          uint8_t second = num&0x00FF; 
          addData(first, function);
          addData(second, function);
        }
      }
    }
  }

  sendData(function, dataCount);
  packetCount = 0;
  
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  SPI.end();
}


void readCard() {
  function = 0x63;
  clearBuffer();
  
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  if(!ntagRead(pageInit)){
    return;
  }

  addData(dump[0], function);
  addData(dump[1], function);

  if(!ntagRead(pagePass)) {
    return;
  }
  uint8_t timeInit = dump[0];
  uint32_t timeInit2 = dump[1];
  timeInit2 <<= 8;
  timeInit2 += dump[2];
  timeInit2 <<= 8;
  timeInit2 += dump[3];
  
  for (uint8_t page = 6; page < 8; page++) {
    if (!ntagRead(page)) {
      return;
    }

    for (uint8_t i = 0; i < 4; i++){
      addData(dump[i], function);
    }
  }

  for (uint8_t page = 8; page < ntagValue; page++) {
    if (!ntagRead(page)) {
      return;
    }
    
    if (dump[0] == 0) {
      break;
    }
    
    addData(dump[0], function);

    uint32_t time2 = dump[1];
    time2 <<= 8;
    time2 += dump[2];
    time2 <<= 8;
    time2 += dump[3];
    if (time2 > timeInit2) {
      addData(timeInit, function);
    }
    else {
      addData(timeInit+1, function);
    }
    for (uint8_t i = 1; i < 4; i++) {
      addData(dump[i], function);
    }
  }
  
  sendData(function, dataCount);
  packetCount = 0;
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  SPI.end();

  
}

/*
 * 
 */
void readRawCard() {
  function = 0x65;
  clearBuffer();
  
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  if(!ntagRead(pageCC)) {
    return;
  }
  
  for (uint8_t page = 4; page < ntagValue; page++) {
    if (!ntagRead(page)) {
      return;
    }

    addData(page, function);
    for (uint8_t i = 0; i < 4; i++) {
      addData(dump[i], function);
    }
  }
  
  sendData(function, dataCount);
  packetCount = 0;
  
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  SPI.end();
}


/*
 * 
 */
void writeMasterSleep() {

    // --------------- NRF24l01
if(rf24_online){
sendNRF.pageInit_0 = 0;
sendNRF.pageInit_1 = 252;
sendNRF.pageInit_2 = 255;
sendNRF.pageInit_3 = vers;
sendNRF.pagePass_0 = pass[0];
sendNRF.pagePass_1 = pass[1];
sendNRF.pagePass_2 = pass[2];
sendNRF.pagePass_3 = 0;
TransmitNRF();
}
// --------------- NRF24l01 ---
  
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  byte dataBlock[4] = {0, 252, 255, vers};
  if(!ntagWrite(dataBlock, pageInit)) {
    return;
  }

  byte dataBlock2[] = {pass[0], pass[1], pass[2], 0};
  if(!ntagWrite(dataBlock2, pagePass)) {
    return;
  }

  signalOK();
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  SPI.end();
}

/*
 * 
 */
void signalError(uint8_t num) {
  beep(50,3);
  if (num == 0) return;

  function = 0x78;

  clearBuffer();

  addData(num, function);
  sendData(function, dataCount);
  packetCount = 0;
}

/*
 * 
 */
void signalOK() {
  beep(200,1);
}

void getVersion() {
  function = 0x66;

  clearBuffer();

  addData(vers, function);
  sendData(function, dataCount);
  packetCount = 0;
}

void clearBuffer() {
  dataCount = 2;
  for(uint8_t j = 0; j < dataSize; j++) {
    dataBuffer[j] = 0; 
  }
  for(uint8_t k = 0; k < packetSize; k++) {
    serialBuffer[k] = 0;
  }
}





// --------------- NRF24l01

bool rf24_findFunc(){   // Пока не используем
 
  switch (serialBuffer[1]) {
   

    case 0x47:// writeMasterLog();
sendNRF.pageInit_1=253;
      break;

		default:
			return true;
	}

beep(50, sendNRF.pageInit_1 - 249);
  radio.stopListening();                                    // Перестаем слушать эфир для начала передачи
 radio.setChannel( nCanalNRF24l01 );                       // !!!!Проверить влияет ли на стабильность передачи!!!!!!!!!!!!!! 
 radio.flush_tx();                          // чистим буфер отправки
   if (!radio.write( &sendNRF, sizeof(sendNRF) ))
     {
       beep(150, 3); //Serial.println(F("Sending Data - ERROR"));
     } 
      else {
           beep(30, 1); //Serial.println(F("Sending Data - Ok"));
       }

 radio.startListening();                                    // Переходим в режим прослушки эфира
 radio.flush_rx();                                          // Очищаем буфер приемника

 
	if(serialBuffer[1]==0x47){
		radio.startListening();                 // Start listening
		uint8_t i=0;
	  function = 0x61;
	  clearBuffer();
	  addData(0, function);
	  addData(dump[0], function);

	  for (uint8_t page = 5; page < ntagValue; page++) {
			i=0;
	    while(!radio.available()){
				delay(5);
				i++;
				if(i>100)
					return true;//timeout
			}
      radio.read(&dump,4);  // Read the payload
	    for (uint8_t i = 0; i < 4; i++) {
	      for (uint8_t y = 0; y < 8; y++) {
	        uint8_t temp = dump[i];
	        temp = temp >> y;
	        if (temp%2 == 1) {

	          uint16_t num = (page - 5)*32 + i*8 + y;
	          uint8_t first = (num&0xFF00)>>8;
	          uint8_t second = num&0x00FF;
	          addData(first, function);
	          addData(second, function);
	        }
	      }
	    }
	  }

	  sendData(function, dataCount);
	  packetCount = 0;
	 	radio.stopListening();                                        // First, stop listening so we can talk
	}
	return false;
}

void setupNRF(){
//    Serial.println( "setupNRF()");
    beep(50, 2); delay(200);beep(50, 1); delay(300);beep(50, 3); 
    SPI.begin();
    radio.begin();                           // Setup and configure rf radio
    radio.setChannel(nCanalNRF24l01);        // Set the channel
    radio.setRetries(15,15);                  // Optionally, increase the delay between retries. Want the number of auto-retries as high as possible (15)
    radio.setDataRate(RF24_250KBPS);           // Raise the data rate to reduce transmission distance and increase lossiness
    radio.setPALevel(RF24_PA_MIN);           // Set PA LOW for this demonstration. We want the radio to be as lossy as possible for this example.
    radio.setAutoAck(0);                     // Ensure autoACK is enabled 
    radio.setCRCLength(RF24_CRC_16);         // Set CRC length to 16-bit to ensure quality of data
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
//    radio.setPayloadSize(16);
    radio.startListening();                 // Start listening
    radio.flush_tx();
      rf24_online = true;
 
}


// Отправляем данные
void TransmitNRF() {

beep(50, sendNRF.pageInit_1 - 249);
   
 radio.stopListening();                                    // Перестаем слушать эфир для начала передачи
 radio.setChannel( nCanalNRF24l01 );                       // !!!!Проверить влияет ли на стабильность передачи!!!!!!!!!!!!!! 
 radio.flush_tx();                          // чистим буфер отправки
// if (!radio.write( &sendNRF, sizeof(sendNRF) ))
  if (!radio.write( &sendNRF, sizeof(sendNRF) ))
     {
      beep(150, 3);  //Serial.println(F("Sending Data - ERROR"));
     } 
      else {
       //beep(30, 1);  //Serial.println(F("Sending Data - OK"));
     }

 radio.startListening();                                    // Переходим в режим прослушки эфира
 radio.flush_rx();                                          // Очищаем буфер приемника

}

// --------------- NRF24l01 ---
