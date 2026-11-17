/*
  RFID Door Lock - Standalone Test
  No LCD, no water/temp sensors - isolates RFID + servo only.
  Wiring:
    RFID: SDA/SS->10, SCK->13, MOSI->11, MISO->12, RST->9, 3.3V, GND
    Servo: Signal->6, 5V, GND
  Open Serial Monitor at 9600 baud.
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define RFID_SS   10
#define RFID_RST  9
MFRC522 rfid(RFID_SS, RFID_RST);
byte authorizedUID[4] = {0xDE, 0xAD, 0xBE, 0xEF}; // <-- replace with YOUR card's UID

#define SERVO_PIN 6
Servo doorServo;
const int LOCK_ANGLE   = 0;
const int UNLOCK_ANGLE = 90;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  Serial.print("RFID version check: ");
  rfid.PCD_DumpVersionToSerial();

  doorServo.attach(SERVO_PIN);
  doorServo.write(LOCK_ANGLE);

  Serial.println("Ready. Scan a card...");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  bool match = (rfid.uid.size == 4);
  Serial.print("Scanned UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
    if (match && rfid.uid.uidByte[i] != authorizedUID[i]) match = false;
  }
  Serial.println();

  if (match) {
    Serial.println("Access Granted -> door unlocking");
    doorServo.write(UNLOCK_ANGLE);
    delay(3000);
    doorServo.write(LOCK_ANGLE);
    Serial.println("Door locked again");
  } else {
    Serial.println("Access Denied");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
