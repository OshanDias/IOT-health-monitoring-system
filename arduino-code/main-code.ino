/*
 * SMART HEALTH MONITORING SYSTEM
 * NIBM HDSE Batch 25.2 - IoT Coursework
 * Final Version -
 */

// ========== LIBRARIES ==========

//  Wire.h handles I2C communication — needed for LCD, MAX30105, and MLX90614 sensors.
#include <Wire.h>

//  ESP8266WiFi.h lets the ESP8266 connect to WiFi networks.
#include <ESP8266WiFi.h>

//  FirebaseESP8266.h is the library to read/write to Firebase Realtime Database (older version, works well for ESP8266 with public DB).
#include <FirebaseESP8266.h>

//  LiquidCrystal_I2C.h controls the 16x2 I2C LCD display.
#include <LiquidCrystal_I2C.h>

//  MAX30105.h is SparkFun's library for the MAX30102/MAX30105 pulse oximeter sensor (heart rate + SpO2).
#include <MAX30105.h>

//  heartRate.h provides helper functions like checkForBeat() — but in this code we use the library's built-in getHeartRate() instead.
#include <heartRate.h>

// Adafruit_MLX90614.h library for the contactless infrared temperature sensor.
#include <Adafruit_MLX90614.h>

// ========== WIFI & FIREBASE ==========

//  Replace these with your actual WiFi credentials — the device needs internet to send data to Firebase.
#define WIFI_SSID "YOUR_WIFI_NAME"         // ⚠️ CHANGE THIS!
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD" // ⚠️ CHANGE THIS!

//  Your Firebase Realtime Database URL (without https://) — copy from Firebase console.
#define FIREBASE_HOST "health-monitor-iot-c3835-default-rtdb.asia-southeast1.firebasedatabase.app"

//  Empty string = no authentication → database must have public write rules (".write": true) for this to work.
#define FIREBASE_AUTH ""

// ========== PINS ==========

//  GPIO pins connected to LEDs and buzzer (D5–D8 are common on NodeMCU/ESP8266).
#define GREEN_LED  D5   // Normal status
#define YELLOW_LED D6   // Warning status
#define RED_LED    D7   // Critical/alert status
#define BUZZER     D8   // Audible alert

// ========== OBJECTS ==========

//  Create instance of the MAX30105 sensor object.
MAX30105 heartSensor;

// Create instance of the MLX90614 temperature sensor.
Adafruit_MLX90614 tempSensor = Adafruit_MLX90614();

// Create LCD object — address 0x27 is most common for I2C LCDs, 16 columns × 2 rows.
LiquidCrystal_I2C lcd(0x27, 16, 2);

//  FirebaseData object — used to handle all Firebase read/write operations.
FirebaseData firebaseData;

// ========== VARIABLES ==========

// Stores the latest calculated heart rate in beats per minute (BPM).
float heartRate = 0;

//  Stores the latest SpO2 percentage (this code uses a very simplified placeholder).
float spo2 = 0;

//  Stores body temperature in Celsius from MLX90614.
float bodyTemp = 0;

//  Current overall health status — updated every reading cycle.
String currentStatus = "normal";

// ========== THRESHOLDS ==========

// Comment: Structure (like a mini-class) to group all threshold values — easy to adjust later if needed.
struct {
  float hr_min = 50.0;           // Normal minimum heart rate
  float hr_max = 110.0;          // Normal maximum heart rate
  float hr_critical_min = 40.0;  // Dangerously low HR
  float hr_critical_max = 130.0; // Dangerously high HR
  float spo2_min = 92.0;         // Normal minimum SpO2
  float spo2_critical = 90.0;    // Critically low SpO2
  float temp_min = 36.0;         // Normal low body temp
  float temp_max = 37.5;         // Normal high body temp
  float temp_critical_min = 35.0;// Hypothermia range
  float temp_critical_max = 39.5;// Fever/hyperthermia range
} limits;

// ========== TIMING ==========

// Comment: Tracks the last time sensors were read (milliseconds since start).
unsigned long lastRead = 0;

// Comment: Tracks the last time data was sent to Firebase.
unsigned long lastLog = 0;

// ========== SETUP ==========

// Comment: Runs once when the ESP8266 powers on or resets.
void setup() {
  // Comment: Start serial communication at 115200 baud — for debugging via Arduino IDE Serial Monitor.
  Serial.begin(115200);
  
  // Comment: Print startup header to serial.
  Serial.println("\n=== HEALTH MONITOR ===");
 
  // Comment: Set LED and buzzer pins as outputs so we can control them.
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
 
  // Comment: Start I2C communication on custom pins D2 (SDA) and D1 (SCL) — common for NodeMCU.
  Wire.begin(D2, D1);
 
  // Comment: Initialize LCD, turn on backlight, show welcome message.
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Health Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);  // Comment: Pause 2 seconds so user can read it.
 
  // Comment: Try to initialize MAX30105 sensor over I2C.
  Serial.println("Init MAX30102...");
  if (!heartSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    // Comment: If sensor not found → show error on LCD and blink red LED forever (hardware fault).
    Serial.println("MAX30102 FAIL!");
    lcd.clear();
    lcd.print("Sensor Error!");
    while(1) {
      digitalWrite(RED_LED, !digitalRead(RED_LED));
      delay(200);
    }
  }
  // Comment: Configure sensor with default settings + low LED current (0x0A = small brightness).
  heartSensor.setup();
  heartSensor.setPulseAmplitudeRed(0x0A);
  heartSensor.setPulseAmplitudeIR(0x0A);
  Serial.println("MAX30102 OK");
 
  // Comment: Initialize temperature sensor.
  Serial.println("Init MLX90614...");
  if (!tempSensor.begin()) {
    // Comment: If temp sensor fails → same error handling as above.
    Serial.println("MLX90614 FAIL!");
    lcd.clear();
    lcd.print("Temp Sensor Err");
    while(1) {
      digitalWrite(RED_LED, !digitalRead(RED_LED));
      delay(200);
    }
  }
  Serial.println("MLX90614 OK");
 
  // Comment: Start WiFi connection process.
  lcd.clear();
  lcd.print("WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  // Comment: Wait up to 10 seconds (20 × 500ms) for connection.
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt++;
  }
 
  // Comment: Check connection result.
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK!");
    Serial.println(WiFi.localIP());  // Comment: Print IP address — useful for debugging.
   
    // Comment: Connect to Firebase (no auth = public DB).
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);  // Comment: Auto-reconnect if WiFi drops.
    Serial.println("Firebase OK!");
   
    lcd.setCursor(0, 1);
    lcd.print("Connected!");
  } else {
    // Comment: No WiFi → device runs locally (LCD + LEDs work, no cloud logging).
    Serial.println("\nNo WiFi - Local only");
    lcd.setCursor(0, 1);
    lcd.print("Local Mode");
  }
 
  delay(2000);
  lcd.clear();
  lcd.print("Ready!");
  delay(1000);  // Comment: Final pause before main loop.
}

// ========== LOOP ==========

// Comment: Main program loop — runs forever after setup().
void loop() {
  // Comment: Get current time since boot (in milliseconds).
  unsigned long now = millis();
 
  // Comment: Read sensors every 10 seconds (10000 ms).
  if (now - lastRead >= 10000) {
    lastRead = now;  // Comment: Update timestamp.
   
    readSensors();   // Comment: Get new values from sensors.
   
    // Comment: Only process good readings.
    if (validate()) {
      currentStatus = evaluate();   // Comment: Decide normal/warning/critical.
      updateLCD();                  // Comment: Show values on display.
      updateAlerts();               // Comment: Control LEDs + buzzer.
      printSerial();                // Comment: Debug output to Serial Monitor.
    }
  }
 
  // Comment: Send data to Firebase every 60 seconds — but only if WiFi is connected.
  if (now - lastLog >= 60000 && WiFi.status() == WL_CONNECTED) {
    lastLog = now;
    logFirebase();  // Comment: Upload latest reading.
  }
 
  delay(100);  // Comment: Small delay to prevent CPU hogging.
}

// ========== READ SENSORS ==========

// Comment: Collects raw data from both sensors.
void readSensors() {
  // Comment: Get current IR (infrared) reading — main signal for detecting finger/beat.
  long ir = heartSensor.getIR();
 
  if (ir > 50000) {  // Comment: Arbitrary threshold — means finger is probably placed (IR reflects back strongly).
    // Comment: Use library's built-in function to calculate BPM (based on beat detection inside library).
    heartRate = heartSensor.getHeartRate();
    
    // Comment: Very simple fake SpO2 — just varies a little with IR noise. Not accurate — for demo only!
    spo2 = 95 + (ir % 5); 
  } else {
    // Comment: No finger → reset values to zero.
    heartRate = 0;
    spo2 = 0;
  }
 
  // Comment: Read object (body) temperature in Celsius.
  bodyTemp = tempSensor.readObjectTempC();
 
  // Comment: Quick debug print of raw values.
  Serial.print("HR:");
  Serial.print(heartRate);
  Serial.print(" SpO2:");
  Serial.print(spo2);
  Serial.print(" Temp:");
  Serial.println(bodyTemp);
}

// ========== VALIDATE ==========

// Comment: Basic sanity check — prevents nonsense values from messing up alerts/display.
bool validate() {
  if (heartRate < 0 || heartRate > 250) return false;
  if (spo2 < 0 || spo2 > 100) return false;
  if (bodyTemp < 25 || bodyTemp > 45) return false;
  return true;  // Comment: All good → proceed.
}

// ========== EVALUATE ==========

// Comment: Uses thresholds to decide overall status.
String evaluate() {
  bool critical = false;
  bool warning = false;
 
  // Comment: Check heart rate if we have a valid reading.
  if (heartRate > 0) {
    if (heartRate < limits.hr_critical_min || heartRate > limits.hr_critical_max) {
      critical = true;
    } else if (heartRate < limits.hr_min || heartRate > limits.hr_max) {
      warning = true;
    }
  }
 
  // Comment: Check SpO2.
  if (spo2 > 0) {
    if (spo2 < limits.spo2_critical) {
      critical = true;
    } else if (spo2 < limits.spo2_min) {
      warning = true;
    }
  }
 
  // Comment: Check temperature (no "warning only" for temp in this logic — only critical or normal).
  if (bodyTemp < limits.temp_critical_min || bodyTemp > limits.temp_critical_max) {
    critical = true;
  } else if (bodyTemp < limits.temp_min || bodyTemp > limits.temp_max) {
    warning = true;
  }
 
  // Comment: Priority: critical > warning > normal.
  if (critical) return "critical";
  if (warning) return "warning";
  return "normal";
}

// ========== UPDATE LCD ==========

// Comment: Refreshes the 16x2 LCD with current readings and status.
void updateLCD() {
  lcd.clear();  // Comment: Wipe screen first.
 
  if (heartRate > 0) {
    lcd.setCursor(0, 0);
    lcd.print("HR:");
    lcd.print((int)heartRate);  // Comment: Cast to int — no decimals needed.
    lcd.print(" O2:");
    lcd.print((int)spo2);
    lcd.print("%");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Place finger...");  // Comment: User prompt when no finger detected.
  }
 
  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print(bodyTemp, 1);  // Comment: Show 1 decimal place.
  lcd.print("C ");
 
  // Comment: Show short status text.
  if (currentStatus == "normal") lcd.print("OK");
  else if (currentStatus == "warning") lcd.print("WARN");
  else lcd.print("ALERT");
}

// ========== UPDATE ALERTS ==========

// Comment: Controls visual (LEDs) and audible (buzzer) feedback.
void updateAlerts() {
  // Comment: Reset everything first.
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  noTone(BUZZER);
 
  if (currentStatus == "normal") {
    digitalWrite(GREEN_LED, HIGH);  // Comment: Steady green = all good.
  }
  else if (currentStatus == "warning") {
    digitalWrite(YELLOW_LED, HIGH);
    // Comment: 3 short beeps (2000 Hz, 100 ms each, 200 ms pause).
    for (int i = 0; i < 3; i++) {
      tone(BUZZER, 2000, 100);
      delay(200);
    }
  }
  else {
    digitalWrite(RED_LED, HIGH);
    tone(BUZZER, 2500, 1000);  // Comment: Long 1-second high-pitch beep for critical.
  }
}

// ========== LOG FIREBASE ==========

// Comment: Sends current reading to Firebase as JSON under a timestamp key.
void logFirebase() {
  // Comment: Path like /patients/patient_001/vitals/1741583400 (Unix seconds).
  String path = "/patients/patient_001/vitals/" + String(millis()/1000);
 
  // Comment: Create JSON object with our 4 values.
  FirebaseJson json;
  json.set("hr", heartRate);
  json.set("spo2", spo2);
  json.set("temp", bodyTemp);
  json.set("status", currentStatus);
 
  // Comment: Try to write JSON → success prints checkmark, else cross.
  if (Firebase.setJSON(firebaseData, path, json)) {
    Serial.println("✓ Firebase logged");
  } else {
    Serial.println("✗ Firebase fail");
    // Tip: Check Serial for error — often WiFi drop or rules issue.
  }
}

// ========== PRINT SERIAL ==========

// Comment: Nice formatted debug output to Serial Monitor every cycle.
void printSerial() {
  Serial.println("━━━━━━━━━━━━━━━━");
  Serial.print("HR: ");
  Serial.print(heartRate);
  Serial.println(" bpm");
  Serial.print("SpO2: ");
  Serial.print(spo2);
  Serial.println("%");
  Serial.print("Temp: ");
  Serial.print(bodyTemp);
  Serial.println("°C");
  Serial.print("Status: ");
  Serial.println(currentStatus);
  Serial.println("━━━━━━━━━━━━━━━━");
}
