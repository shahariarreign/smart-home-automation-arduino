/*
  LCD Block Fill Test
  Fills both rows with solid blocks (character 255) at max brightness.
  If you see NOTHING even with correct contrast, the backpack-to-LCD
  solder joints are the problem, not code or contrast.
*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  lcd.init();
  lcd.backlight();
  for (int row = 0; row < 2; row++) {
    lcd.setCursor(0, row);
    for (int col = 0; col < 16; col++) {
      lcd.write(255); // solid block character
    }
  }
}

void loop() {}
