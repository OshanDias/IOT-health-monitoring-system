#include <Wire.h>  //Include I2C communication library (used for ESP8266 I2C pins
#include <LiquidCrystal_I2C.h> // Include library to control I2C LCD display

LiquidCrystal_I2C lcd(0x27,16,2); // Create an LCD object with I2C address 0x27, 16 columns and 2 rows

void setup() {
  Wire.begin(D2, D1);
  // Initialize I2C communication
  // D2 = SDA (data line)
  // D1 = SCL (clock line)
  
  lcd.init();
  lcd.backlight(); // Turn ON the LCD backlight
  lcd.setCursor(0,0);
  lcd.print("HR:");
  lcd.print("Sp:");
  lcd.print("temp:");
}

void loop() {

}
