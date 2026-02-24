## Basic Arduino Sketch
#include <Wire.h>
#include <Adafruit_MLX90614.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void setup() {
  Serial.begin(115200);
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX sensor!");
    while (1);
  };
}

void loop() {
  Serial.print("Ambient: "); Serial.print(mlx.readAmbientTempC()); Serial.print("°C ");
  Serial.print("| Object: "); Serial.print(mlx.readObjectTempC()); Serial.println("°C");
  delay(1000); // Read every second [cite: 405]
}
