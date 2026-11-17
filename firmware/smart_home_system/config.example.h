/*
  config.example.h  --  copy this file to "config.h" (same folder) and edit.
  config.h is git-ignored so your real card UID is never committed.
*/
#ifndef CONFIG_H
#define CONFIG_H

// --- RFID: UID of the card/tag allowed to open the door -------------------
// Find yours by running firmware/tests/rfid_only_test and scanning the card
// (the Serial Monitor prints "Scanned UID: XX XX XX XX").
const byte authorizedUID[4] = {0xDE, 0xAD, 0xBE, 0xEF};

// --- Servo (door lock) angles in degrees ----------------------------------
const int LOCK_ANGLE   = 0;
const int UNLOCK_ANGLE = 90;

// --- Fan: turns ON at or above this temperature (deg C) -------------------
const float TEMP_THRESHOLD = 31.0;

// --- Pump: HC-SR04 measures distance from sensor DOWN to the water surface -
// Larger distance = lower water.
const float LOW_THRESHOLD_CM  = 13.0;  // distance >= this -> tank LOW  -> pump ON
const float FULL_THRESHOLD_CM = 3.0;   // distance <= this -> tank FULL -> pump OFF

// --- LCD I2C address (find with firmware/tests/i2c_scanner; usually 0x27/0x3F)
#define LCD_I2C_ADDR 0x27

#endif
