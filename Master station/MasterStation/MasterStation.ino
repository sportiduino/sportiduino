#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

const byte vers = 102; //version of software

const byte LED = 4;
const byte BUZ = 3;
const byte RST_PIN = 9;
const byte SS_PIN = 10;

//password for master key
uint8_t pass[] = {0,0,0};
uint8_t stantionConfig = 0;
uint8_t masterConfig = 0; //1 - автоматическое считывание чипов, 0 - только по запросу
const uint16_t eepromPass = 850;
const uint16_t eepromMaster = 870;

const byte pageCC = 3;
const byte pageInit = 4;
const byte pagePass = 5;
const byte pageInfo1 = 6;
const byte pageInfo2 = 7;

uint8_t ntagValue = 130;
uint8_t ntagType = 213;

uint8_t function = 0;

const uint8_t timeOut = 10;
const uint8_t packetSize = 32;
const uint8_t dataSize = 30;

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::StatusCode status;

uint8_t serialBuffer[packetSize];
uint8_t dataBuffer[dataSize];

void setup() {

  Serial.begin(9600);
  Serial.setTimeout(timeOut); 

  pass[0] = eepromread(eepromPass);
  pass[1] = eepromread(eepromPass+3);
  pass[2] = eepromread(eepromPass+6);
  stantionConfig = eepromread(eepromPass+9);
  masterConfig = eepromread(eepromMaster);
  
}

void loop() {
  if (masterConfig==1){
    readChip();
  }
  
  if (Serial.available() > 0){
  
    clearBuffer();
    
    Serial.readBytes(serialBuffer, packetSize);
    
    uint8_t sumAdr = serialBuffer[2]+3;
    if (sumAdr>28) sumAdr=31;

    
       
    for (uint8_t i=0;i<sumAdr-1;i++){
      dataBuffer[i]=serialBuffer[i+1];
    }

    

    uint8_t sum = checkSum(dataBuffer,sumAdr-1);
   
    
    if (serialBuffer[0]!=0xFE ||
        serialBuffer[sumAdr]!=sum){
      signalError(0x01);
    }
    else{
      findFunc();
    }
   
  }
}


uint8_t checkSum(const uint8_t *addr, uint8_t len) {
  uint8_t sum = 0;
  for (uint8_t i=0;i<len;i++){
    sum+=addr[i];
  }
  return sum;
}

uint8_t dataCount = 2;
uint8_t packetCount = 0;

/*
 * 
 */
void addData(uint8_t data,uint8_t func){
  
   dataBuffer[dataCount] = data;
   
   if (dataCount == dataSize-1){
     sendData(func,packetCount+32);
     packetCount++;
   }
   else{
     dataCount++;
   }
  
}

/*
 * 
 */
void sendData(uint8_t func, uint8_t leng){
  
  Serial.write(0xFE);
  
  dataBuffer[0] = func;
  dataBuffer[1] = leng-2;

  uint8_t trueleng = leng;
  if (leng>30)trueleng=30;
  
  for (uint8_t w  = 0;w<trueleng;w++){
    Serial.write(dataBuffer[w]);
  }
  Serial.write(checkSum(dataBuffer,trueleng));

  clearBuffer();  
}



uint8_t dump[16];

//MFRC522::StatusCode MFRC522::MIFARE_Read
bool ntagWrite (uint8_t *dataBlock, uint8_t pageAdr){

  const uint8_t sizePageNtag = 4;
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Ultralight_Write(pageAdr, dataBlock, sizePageNtag);
  if (status != MFRC522::STATUS_OK) {
    signalError(0x02);
    return false;
  }

  uint8_t buffer[18];
  uint8_t size = sizeof(buffer);

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(pageAdr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    signalError(0x03);
    return false;
  }
 
  for (uint8_t i = 0; i < 4; i++) {
    dump[i]=buffer[i];
  }

  if (dump[0]==dataBlock[0] && dump[1]==dataBlock[1] && dump[2]==dataBlock[2] && dump[3]==dataBlock[3]){
    return true;
  }
  else{
    return false;
  }
}

/*
 * 
 */
bool ntagRead (uint8_t pageAdr){
  uint8_t buffer[18];
  uint8_t size = sizeof(buffer);

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(pageAdr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    signalError(0x03);
    return false;
  }
  
  for (uint8_t i = 0; i < 16; i++) {
    dump[i]=buffer[i];
  }
  return true;
}

/*
 * 
 */
void flash(int ms, byte n){
  
  pinMode (LED, OUTPUT);
  pinMode (BUZ, OUTPUT);

  for (byte i = 0; i<n; i++){
    digitalWrite (LED, HIGH);
    tone (BUZ,4000,ms);
    delay (ms);
    digitalWrite (LED, LOW);
    if ((n-i)!=0){
      delay(ms);
    }
  }
  
} //end of flash

//write to EEPROM 3 value from adress to adress+2
void eepromwrite (uint16_t adr, uint8_t val) {
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
    signalError(0x04);
    return 0;
  }

} // end of eepromstantion

/*
 * 
 */
void findFunc(){
  switch (serialBuffer[1]){
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
    case 0x49:
      updConfig();
      break;
    case 0x4B:
      readChip();
      break;
    case 0x4C:
      readRawChip();
      break;
    case 0x4E:
      writeMasterSleep();
      break;
    case 0x58:
      signalError(0);
      break;
    case 0x59:
      signalOK();
      break;
      
  }
  
}

/*
 * 
 */
void writeMasterTime(){
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
  
  byte dataBlock[4]    = {0,250,255,vers};
  if(!ntagWrite(dataBlock,pageInit)){
    return;
  }


  byte dataBlock2[] = {pass[0],pass[1],pass[2],0};
  if(!ntagWrite(dataBlock2,pagePass)){
    return;
  }

  byte dataBlock3[] = {dataBuffer[3],dataBuffer[2],dataBuffer[4],0};
  if(!ntagWrite(dataBlock3,pageInfo1)){
    return;
  }

  byte dataBlock4[] = {dataBuffer[5],dataBuffer[6],dataBuffer[7],0};
  if(!ntagWrite(dataBlock4,pageInfo2)){
    return;
  }
  

  flash(500,3);

  SPI.end();
}

/*
 * 
 */
void  writeMasterNum(){
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

  byte dataBlock[4]    = {0,251,255,vers};
  if(!ntagWrite(dataBlock,pageInit)){
    return;
  }


  byte dataBlock2[] = {pass[0],pass[1],pass[2],0};
  if(!ntagWrite(dataBlock2,pagePass)){
    return;
  }

  byte dataBlock3[] = {dataBuffer[2],0,0,0};
  if(!ntagWrite(dataBlock3,pageInfo1)){
    return;
  }

  signalOK();

  SPI.end();
}

/*
 * 
 */
void writeMasterPass(){
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

  byte dataBlock[4]    = {0,254,255,vers};
  if(!ntagWrite(dataBlock,pageInit)){
    return;
  }


  byte dataBlock2[] = {dataBuffer[5],dataBuffer[6],dataBuffer[7],0};
  if(!ntagWrite(dataBlock2,pagePass)){
    return;
  }

  pass[0] = dataBuffer[2];
  pass[1] = dataBuffer[3];
  pass[2] = dataBuffer[4];
  stantionConfig = dataBuffer[8];  
  
  eepromwrite(eepromPass,dataBuffer[2]);
  eepromwrite(eepromPass+3,dataBuffer[3]);
  eepromwrite(eepromPass+6,dataBuffer[4]);
  eepromwrite(eepromPass+9,dataBuffer[8]);  
  
  byte dataBlock3[] = {pass[0],pass[1],pass[2],stantionConfig};
  if(!ntagWrite(dataBlock3,pageInfo1)){
    return;
  }
  
  signalOK();

  SPI.end();
}

/*
 * 
 */
void writeInit(){
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

  if(!ntagRead(pageCC)){
    return;
  }

  
  if (dump[2]==0x3E) {
    ntagValue = 130;
    ntagType = 5;
  }
  else if (dump[2]==0x6D){
    ntagValue = 216;
    ntagType = 6;
  }
  else {
    ntagValue = 40;
    ntagType = 3;
  }
  
  
  
  byte Wbuff[] = {255,255,255,255};
  
  for (byte page=4; page < ntagValue;page++){
    if (!ntagWrite(Wbuff,page)){
      return;
    }
  }

  byte Wbuff2[] = {0,0,0,0};
  
  for (byte page=4; page < ntagValue;page++){
    if (!ntagWrite(Wbuff2,page)){
      return;
    }
  }

  byte dataBlock[4]    = {dataBuffer[2],dataBuffer[3],ntagType,vers};
  if(!ntagWrite(dataBlock,pageInit)){
     return;
  }


  byte dataBlock2[] = {dataBuffer[4],dataBuffer[5],dataBuffer[6],dataBuffer[7]};
  if(!ntagWrite(dataBlock2,pagePass)){
    return;
  }

  byte dataBlock3[] = {dataBuffer[8],dataBuffer[9],dataBuffer[10],dataBuffer[11]};
  if(!ntagWrite(dataBlock3,pageInfo1)){
    return;
  }

  byte dataBlock4[] = {dataBuffer[12],dataBuffer[13],dataBuffer[14],dataBuffer[15]};
  if(!ntagWrite(dataBlock4,pageInfo2)){
    return;
  }
  
  signalOK();

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

  byte dataBlock[4]    = {dataBuffer[2],dataBuffer[3],dataBuffer[4],dataBuffer[5]};
  if(!ntagWrite(dataBlock,pageInfo1)){
    return;
  }


  byte dataBlock2[] = {dataBuffer[6],dataBuffer[7],dataBuffer[8],dataBuffer[9]};
  if(!ntagWrite(dataBlock2,pageInfo2)){
    return;
  }

  signalOK();


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

  if(!ntagRead(pageCC)){
    return;
  }

  uint8_t ntagValue=130;

  if (dump[2]!=0x3E) {
    return;
  }

  byte Wbuff[] = {255,255,255,255};
  
  for (byte page=4; page < ntagValue;page++){
    if (!ntagWrite(Wbuff,page)){
      return;
    }
  }

  byte Wbuff2[] = {0,0,0,0};
   
  for (byte page=4; page < ntagValue;page++){
    if (!ntagWrite(Wbuff2,page)){
      return;
    }
  }

  byte dataBlock[4]    = {0, 253, 255,vers};
  if(!ntagWrite(dataBlock,pageInit)){
    return;
  }


  byte dataBlock2[] = {pass[0], pass[1], pass[2],0};
  if(!ntagWrite(dataBlock2,pagePass)){
    return;
  }

  signalOK();


  SPI.end();
}

/*
 * 
 */
void readLog(){
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

  if(!ntagRead(pageInit)){
    return;
  }

  addData(0,function);
  addData(dump[0],function);

  for (uint8_t page = 5; page<ntagValue;page++){
    
    if (!ntagRead(page)){
      return;
    }

    for (uint8_t i = 0;i<4;i++){
      for (uint8_t y=0;y<8;y++){
        uint8_t temp = dump[i];
        temp = temp >> y;
        if (temp%2 == 1){
        
          uint16_t num = (page-5)*32+i*8+y;
          uint8_t first = (num&0xFF00)>>8;
          uint8_t second = num&0x00FF; 
          addData(first,function);
          addData(second,function);
          
        }
      }

    }
  
  }

  sendData (function,dataCount);
  packetCount = 0;

  

  SPI.end();
}



void  readChip(){
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

  if(!ntagRead(pageCC)){
    return;
  }
  
  if (dump[2]==0x3E) ntagValue = 130;
  else if (dump[2]==0x6D) ntagValue = 226;
  else ntagValue=40;

  if(!ntagRead(pageInit)){
    return;
  }

  addData(dump[0],function);
  addData(dump[1],function);

  if(!ntagRead(pagePass)){
    return;
  }
  uint8_t timeInit = dump[0];
  uint32_t timeInit2 = dump[1];
  timeInit2<<=8;
  timeInit2+=dump[2];
  timeInit2<<=8;
  timeInit2+=dump[3];
  
  for (uint8_t page = 6; page<8;page++){
    if (!ntagRead(page)){
      return;
    }

    for (uint8_t i = 0;i<4;i++){
      addData(dump[i],function);
    }
  }

  for (uint8_t page = 8; page<ntagValue;page++){
    
    if (!ntagRead(page)){
      return;
    }
    
    if (dump[0]==0) break;
    
    addData(dump[0],function);

    uint32_t time2 = dump[1];
    time2<<=8;
    time2+=dump[2];
    time2<<=8;
    time2+=dump[3];
    if (time2>timeInit2){
      addData(timeInit,function);
    }
    else{
      addData(timeInit+1,function);
    }
    for (uint8_t i = 1;i<4;i++){
      addData(dump[i],function);
    }
  
  }
  
  sendData (function,dataCount);
  packetCount = 0;

  SPI.end();

  if (masterConfig==1) {
    flash(20,1);
    delay(2000);
  }
}

/*
 * 
 */
void readRawChip(){
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

  if(!ntagRead(pageCC)){
    return;
  }
  
  if (dump[2]==0x3E) ntagValue = 130;
  else if (dump[2]==0x6D) ntagValue = 224;
  else ntagValue = 40;

  for (uint8_t page = 4; page<ntagValue;page++){
    
    if (!ntagRead(page)){
      return;
    }

    addData(page,function);
    for (uint8_t i = 0;i<4;i++){
      addData(dump[i],function);
    }
  
  }
  
  sendData (function,dataCount);
  packetCount = 0;
  
  
  
  SPI.end();
}


/*
 * 
 */
void writeMasterSleep(){
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

  byte dataBlock[4]    = {0, 252, 255,vers};
  if(!ntagWrite(dataBlock,pageInit)){
    return;
  }


  byte dataBlock2[] = {pass[0], pass[1], pass[2],0};
  if(!ntagWrite(dataBlock2,pagePass)){
    return;
  }

  signalOK();

  SPI.end();
}

/*
 * 
 */
void signalError(uint8_t num){
  flash(100,3);
  if (num==0) return;

  function = 0x78;

  clearBuffer();

  addData(num,function);
  sendData (function,dataCount);
  packetCount = 0;
  
}

/*
 * 
 */
void signalOK(){
  flash(300,1);
}

void readFlash(){

}


void getVersion(){
  
  function = 0x66;

  clearBuffer();

  addData(vers,function);
  sendData (function,dataCount);
  packetCount = 0;

}

void updConfig(){
  function = 0x69;
  

  masterConfig = dataBuffer[2];
  eepromwrite(eepromMaster,masterConfig);

  clearBuffer();
  
  addData(masterConfig,function);
  sendData (function,dataCount);
  packetCount = 0;
}


void clearBuffer(){
  dataCount = 2;
  for(uint8_t j=0;j<dataSize;j++) dataBuffer[j]=0; 
  for(uint8_t k=0;k<packetSize;k++) serialBuffer[k]=0;
}

