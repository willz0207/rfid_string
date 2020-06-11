#include <MFRC522.h> //library responsible for communicating with the module RFID-RC522
#include <SPI.h> //library responsible for communicating of SPI bus


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


#define SS_PIN    21
#define RST_PIN   22
#define led       13
#define led1       4
MFRC522 mfrc522(SS_PIN, RST_PIN);

MFRC522::MIFARE_Key key;

long bayar = 5000;            // isikan tagihan merchant kelipatan 1000, max 255rb

bool notif = true;

long saldo;
int digit;

long OLDsaldo;
int OLDdigit;

void setup() {
    Serial.begin(9600);
    SPI.begin();        
    mfrc522.PCD_Init(); 
    
    pinMode(led1, OUTPUT);
    pinMode(led, OUTPUT);

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println("EMONEY Tanpa Server - Merchant");
    Serial.println();
    Serial.println("Peringatan : Data akan di simpan pada RFID Card pada sector #1 (blocks #4)");
    Serial.println();
    
    if (bayar > 255000){
      while(1){
        Serial.println("ERROR - Tagihan merchant lebih dari 255rb");
      }
    }

    Serial.print("Tagihan Merchant : ");
    Serial.println(bayar);
    Serial.println("Silahkan tap kartu");
    Serial.print("Setiap TAP, saldo berkurang sebesar ");
    Serial.println(bayar);
    digit = bayar/1000;
    //Serial.println(digit);
}

void loop() {
  if (notif){
    notif = false;
    Serial.println();
    
  }
  if ( ! mfrc522.PICC_IsNewCardPresent()){
      return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()){
      return;
  }
  
  // that is: sector #1, covering block #4 up to and including block #7
  byte sector         = 1;
  byte blockAddr      = 4;
  
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
//
    Serial.println("Current data in sector:");
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();

  // Baca Saldo yang ada dari RFID Card
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
      Serial.println("Gagal Baca Kartu RFID");
      //Serial.println(mfrc522.GetStatusCodeName(status));
      resetReader();
      return;
  }
  OLDdigit = buffer[0];
  OLDsaldo = OLDdigit;
  OLDsaldo *= 1000;
  Serial.print("Saldo Kartu Sebelumnya : ");
  Serial.println(OLDsaldo);
  Serial.println();

  // Kurangi Saldo sebesar tagihan merchant
  if (OLDdigit < digit){
    Serial.println("Saldo belum di kurangi, saldo tidak cukup, silahkan isi saldo terlebih dahulu");
    Serial.println("GAGAL bayar merchant");
    delay(500);
    digitalWrite(led1, HIGH);
    delay(1000);
    digitalWrite(led1, LOW);
    resetReader();
    return;
  }

  OLDdigit -= digit;
  
  byte dataBlock[]    = {
      //0,      1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15
      OLDdigit, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
      Serial.println("GAGAL Write Saldo pada Kartu RFID");
      //Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.println();

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
      Serial.println("Gagal Baca Kartu RFID");
      //Serial.println(mfrc522.GetStatusCodeName(status));
  }

  Serial.println();

  Serial.println("Mengurangi Saldo...");
  if (buffer[0] == dataBlock[0]){
    saldo = buffer[0];
    saldo *= 1000;
    //Serial.print("data digit ke 0 : ");
    //Serial.println(buffer[0]);
    Serial.print("======================>>>>>> Saldo kartu sekarang : ");
    Serial.println(saldo);
    delay(500);
    digitalWrite(led, HIGH);
    delay(500);
    digitalWrite(led, LOW);
    delay(1000);
    Serial.println("_________ Berhasil bayar Merchant ___________");
  }else{
    Serial.println("------------ GAGAL bayar Merchant --------------");
  }

  Serial.println();

  // Dump the sector data
  //Serial.println("Current data in sector:");
  //mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

  resetReader();
}

void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

void resetReader(){
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();

  notif = true;
}
