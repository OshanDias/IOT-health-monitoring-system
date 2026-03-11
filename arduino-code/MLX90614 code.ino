#include <Wire.h>
#include <Adafruit_MLX90614.h> // the MLX90614 libriry

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void setup() {
  Serial.begin(115200); // baud rate
  Wire.begin(D2, D1);

  Serial.println("MLX test");

  if (!mlx.begin()) {
    Serial.println("Sensor not found");
    while (1);
  }

  Serial.println("Sensor OK");
}

void loop() {
  Serial.print("Temp = ");
  Serial.println(mlx.readObjectTempC());
  delay(2000);
}
