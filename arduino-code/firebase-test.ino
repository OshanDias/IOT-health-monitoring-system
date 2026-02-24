#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

#define WIFI_SSID "Your_WiFi_Name"
#define WIFI_PASSWORD "Your_Password"
#define FIREBASE_HOST "your-project.firebaseio.com"
#define FIREBASE_AUTH "your_database_secret"

FirebaseData firebaseData;

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

void loop() {
  if (Firebase.setInt(firebaseData, "/test_value", 100)) {
    Serial.println("Firebase updated!");
  } else {
    Serial.println("Firebase Error: " + firebaseData.errorReason());
  }
  delay(5000);
}## Basic Arduino sketch 
