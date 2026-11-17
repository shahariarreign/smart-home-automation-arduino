/*
  Smart Home Automation and Security System - Arduino UNO
  Full system: RFID door lock, DHT11 temp + fan, HC-SR04 water level + pump, LCD status

  Wiring:
    RFID:    SDA/SS->10, SCK->13, MOSI->11, MISO->12, RST->9, 3.3V, GND
    Servo:   Signal->6 (Arduino), V+ -> external 5V adapter, GND -> shared with Arduino GND
    DHT11:   DATA->D2, 5V, GND
    Relay1 (fan):  IN->D3, 5V, GND  (load side: see docs/wiring.md)
    HC-SR04: Trig->D7, Echo->D8, 5V, GND
    Relay2 (pump): IN->D4, 5V, GND  (load side: see docs/wiring.md)
    LCD:     SDA->A4, SCL->A5, 5V, GND

  Libraries required:
    MFRC522 (GithubCommunity)
    DHT sensor library (Adafruit) + Adafruit Unified Sensor
    LiquidCrystal_I2C (Frank de Brabander)

  Logic:
    RFID:  authorized card -> unlock 3s -> relock
    Fan:   temp >= 31C -> ON, else OFF
    Pump:  distance >= 13cm (low water) -> ON, distance <= 3cm (full) -> OFF
    LCD row0: "Scan Your Card" normally, "Welcome Boss" / "Wait a minute." (+row1 "Who are you?") briefly on scan
    LCD row1: alternates every 2s between Temp/Fan status and Water/Pump status

  Open Serial Monitor at 9600 baud.
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "config.h"   // copy config.example.h -> config.h and edit (see README)

// ---------- RFID ----------
#define RFID_SS   10
#define RFID_RST  9
MFRC522 rfid(RFID_SS, RFID_RST);
// authorizedUID[] comes from config.h

// ---------- Servo (door) ----------
#define SERVO_PIN 6
Servo doorServo;
// LOCK_ANGLE / UNLOCK_ANGLE come from config.h

// ---------- DHT11 (temp) + Fan ----------
#define DHT_PIN  2
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);
#define FAN_RELAY 3
// TEMP_THRESHOLD comes from config.h
bool fanOn = false;
float lastTempC = 0;
unsigned long lastDHTRead = 0;
const unsigned long DHT_INTERVAL = 1000; // DHT11 needs >=1s between reads

// ---------- Ultrasonic (water level) + Pump ----------
#define TRIG_PIN 7
#define ECHO_PIN 8
#define PUMP_RELAY 4
// LOW_THRESHOLD_CM / FULL_THRESHOLD_CM come from config.h
bool pumpOn = false;
long lastDistance = 0;
unsigned long lastPumpCheck = 0;
const unsigned long PUMP_INTERVAL = 500;

// ---------- LCD ----------
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);
unsigned long lastLCDSwitch = 0;
const unsigned long LCD_SWITCH_INTERVAL = 2000;
bool showTempOnLCD = true;

void setup() {
  Serial.begin(9600);

  // RFID
  SPI.begin();
  rfid.PCD_Init();
  Serial.print("RFID version check: ");
  rfid.PCD_DumpVersionToSerial();

  // Servo
  doorServo.attach(SERVO_PIN);
  doorServo.write(LOCK_ANGLE);

  // DHT11 + Fan
  dht.begin();
  pinMode(FAN_RELAY, OUTPUT);
  digitalWrite(FAN_RELAY, LOW); // adjust LOW/HIGH to match your relay's active state

  // Ultrasonic + Pump
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PUMP_RELAY, OUTPUT);
  digitalWrite(PUMP_RELAY, LOW); // adjust LOW/HIGH to match your relay's active state

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Scan Your Card");

  Serial.println("System ready. Scan a card...");
}

void loop() {
  checkRFID();
  checkTemperature();
  checkWaterLevel();
  updateLCDRow1();
}

// ---------------- RFID / Door ----------------
void checkRFID() {
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

  lcd.setCursor(0, 0);
  lcd.print("                "); // clear row0 (16 spaces)
  lcd.setCursor(0, 0);

  if (match) {
    Serial.println("Access Granted -> door unlocking");
    lcd.print("Welcome Boss");
    doorServo.write(UNLOCK_ANGLE);
    delay(3000);
    doorServo.write(LOCK_ANGLE);
    Serial.println("Door locked again");
  } else {
    Serial.println("Access Denied");
    lcd.print("Wait a minute.");
    lcd.setCursor(0, 1);
    lcd.print("                "); // clear row1 (borrowed temporarily)
    lcd.setCursor(0, 1);
    lcd.print("Who are you?");
    delay(2000);
  }

  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print("Scan Your Card");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// ---------------- DHT11 / Fan ----------------
void checkTemperature() {
  unsigned long now = millis();
  if (now - lastDHTRead < DHT_INTERVAL) return;
  lastDHTRead = now;

  float tempC = dht.readTemperature();
  if (isnan(tempC)) {
    Serial.println("Failed to read from DHT11 - check wiring");
    return;
  }
  lastTempC = tempC;

  fanOn = (tempC >= TEMP_THRESHOLD);
  digitalWrite(FAN_RELAY, fanOn ? HIGH : LOW);

  Serial.print("Temp: ");
  Serial.print(tempC);
  Serial.print(" C | Fan: ");
  Serial.println(fanOn ? "ON" : "OFF");
}

// ---------------- Ultrasonic / Pump ----------------
void checkWaterLevel() {
  unsigned long now = millis();
  if (now - lastPumpCheck < PUMP_INTERVAL) return;
  lastPumpCheck = now;

  long distance = getDistanceCM();
  lastDistance = distance;

  if (distance >= LOW_THRESHOLD_CM) {
    pumpOn = true;
  } else if (distance <= FULL_THRESHOLD_CM) {
    pumpOn = false;
  }
  // else: between thresholds -> keep current pump state

  digitalWrite(PUMP_RELAY, pumpOn ? HIGH : LOW);

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm | Pump: ");
  Serial.println(pumpOn ? "ON" : "OFF");
}

long getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return duration * 0.034 / 2;
}

// ---------------- LCD row 1 (alternating status) ----------------
void updateLCDRow1() {
  unsigned long now = millis();
  if (now - lastLCDSwitch < LCD_SWITCH_INTERVAL) return;
  lastLCDSwitch = now;
  showTempOnLCD = !showTempOnLCD;

  lcd.setCursor(0, 1);
  lcd.print("                "); // clear row1
  lcd.setCursor(0, 1);

  if (showTempOnLCD) {
    lcd.print("T:");
    lcd.print(lastTempC, 1);
    lcd.print("C F:");
    lcd.print(fanOn ? "ON" : "OFF");
  } else {
    lcd.print("Dist:");
    lcd.print(lastDistance);
    lcd.print("cm P:");
    lcd.print(pumpOn ? "ON" : "OFF");
  }
}
