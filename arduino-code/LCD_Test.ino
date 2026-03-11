#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

void setup() {
  Wire.begin(D2, D1);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("HR:");
  lcd.print("Sp:");
  lcd.print("temp:");
}

void loop() {

}
