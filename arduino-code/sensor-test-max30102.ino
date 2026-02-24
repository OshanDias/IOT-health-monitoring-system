## Basic Arduino sketch
#include <Wire.h>
#include "MAX30105.h" // Common library for MAX30102

MAX30105 particleSensor;

void setup() {
  Serial.begin(115200);
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 not found!");
    while (1);
  }
  particleSensor.setup(); // Configure sensor with default settings
}

void loop() {
  // Read raw values
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  Serial.print("IR: "); Serial.print(irValue);
  Serial.print(" | Red: "); Serial.println(redValue);
  
  delay(100); 
}
