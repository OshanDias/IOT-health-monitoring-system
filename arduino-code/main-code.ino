// ======================================================================
// NIBM HNDSE25.2F IOT PROJECT
// ======================================================================
// HEALTH MONITORING IOT PROJECT - ESP8266 + MAX30102 + MLX90614 + LCD + Firebase
// This code reads heart rate (BPM), body temperature, and simulates SpO2,
// displays on LCD, prints to Serial, and uploads to Firebase Realtime Database.
// ======================================================================

#include <ESP8266WiFi.h>              // Library for WiFi connectivity on ESP8266 boards (NodeMCU, etc.)
#include <Firebase_ESP_Client.h>      // Firebase client library for ESP8266/ESP32 - used to send data to Realtime Database
#include <Wire.h>                     // I2C communication library (needed for MAX30102, MLX90614, and LCD)
#include <LiquidCrystal_I2C.h>        // Library for I2C-based 16x2 LCD display
#include "MAX30105.h"                 // SparkFun library for MAX30102/MAX30105 pulse oximeter & heart rate sensor
#include "heartRate.h"                // Helper file (usually comes with MAX30105 examples) containing checkForBeat() algorithm
#include <Adafruit_MLX90614.h>        // Library for contactless IR temperature sensor (MLX90614)

// ======================== WiFi Credentials ============================
#define WIFI_SSID ""        // Your WiFi network name (SSID)
#define WIFI_PASSWORD "" // WiFi password

// ======================== Firebase Configuration ======================
#define API_KEY ""  
                                      // Web API Key from Firebase project settings → used for authentication
#define DATABASE_URL "https://health-monitor-iot-c3835-default-rtdb.asia-southeast1.firebasedatabase.app/"  
                                      // Full URL of your Firebase Realtime Database (Asia-Southeast1 region)

// ======================== Hardware Pin Definition =====================
#define STATUS_LED D5                 // GPIO pin (D5 on NodeMCU) connected to an LED to show system status

// ======================== Global Objects ==============================
FirebaseData fbdo;                    // Firebase data object - used to handle read/write operations
FirebaseAuth auth;                    // Authentication object (not using email/password here, legacy mode)
FirebaseConfig config;                // Configuration object for Firebase connection settings

LiquidCrystal_I2C lcd(0x27,16,2);     // LCD object: I2C address 0x27, 16 columns, 2 rows

MAX30105 particleSensor;              // MAX30102 sensor object (called particleSensor in library examples)
Adafruit_MLX90614 mlx = Adafruit_MLX90614();  
                                      // MLX90614 contactless temperature sensor object

// ======================== Heart Rate Variables ========================
long lastBeat = 0;                    // Timestamp (in millis) of the previous detected heartbeat
float beatsPerMinute;                 // Instantaneous heart rate (BPM) calculated from time between beats
int beatAvg;                          // Averaged/smoothed heart rate value (displayed and sent)

// ======================== Sensor Readings =============================
float temperature;                    // Body temperature read from MLX90614 (°C)
float spo2 = 98;                      // Blood oxygen saturation (SpO2) – currently simulated/fixed for demo

// ======================================================================
// SETUP FUNCTION - Runs once when the ESP8266 starts / resets
// ======================================================================
void setup()
{
  Serial.begin(115200);               // Start serial communication at 115200 baud for debugging (view in Serial Monitor)

  pinMode(STATUS_LED, OUTPUT);        // Set D5 pin as output to control the status LED

  lcd.init();                         // Initialize the I2C LCD
  lcd.backlight();                    // Turn on LCD backlight

  lcd.setCursor(0,0);                 // Set cursor to column 0, row 0
  lcd.print("Health Monitor");        // Show project title on first line
  lcd.setCursor(0,1);                 // Move to second line
  lcd.print("Booting...");            // Show booting message

  delay(2000);                        // Wait 2 seconds so user can read the message

  // --------------------- Connect to WiFi -----------------------------
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  
                                      // Start connecting to WiFi with given credentials

  lcd.clear();                        // Clear LCD screen
  lcd.print("Connecting WiFi");       // Show WiFi connecting message

  while (WiFi.status() != WL_CONNECTED)  
  {                                   // Loop until connected
    delay(500);                       // Wait 0.5 sec
    Serial.print(".");                // Print dot for progress in Serial Monitor
  }

  lcd.clear();                        // Clear screen after connection
  lcd.print("WiFi Connected");        // Confirmation message

  digitalWrite(STATUS_LED, HIGH);     // Turn ON status LED to indicate successful WiFi connection

  delay(1500);                        // Short pause

  // --------------------- Initialize Firebase -------------------------
  config.api_key = API_KEY;           // Set the Web API Key in config
  config.database_url = DATABASE_URL; // Set your database URL

  Firebase.begin(&config, &auth);     // Start Firebase connection (legacy token mode if no user auth)
  Firebase.reconnectWiFi(true);       // Automatically reconnect to WiFi if connection drops

  // --------------------- Initialize MAX30102 Sensor ------------------
  if (!particleSensor.begin(Wire))    // Try to detect and initialize MAX30102 over I2C
  {                                   // If failed...
    Serial.println("MAX30102 not found");  
    lcd.clear();
    lcd.print("MAX30102 Error");      // Show error on LCD
    while(1);                         // Infinite loop → stop program (halt on error)
  }

  particleSensor.setup();             // Apply default settings: LED power, sample rate, etc.
  particleSensor.setPulseAmplitudeRed(0x0A);  
                                      // Set red LED current low (0x0A = very dim) – mainly using IR for HR
  particleSensor.setPulseAmplitudeGreen(0);  
                                      // Turn OFF green LED (not used here)

  // --------------------- Initialize MLX90614 Temperature Sensor ------
  mlx.begin();                        // Start MLX90614 sensor (I2C address is default 0x5A)

  lcd.clear();                        // Clear screen
  lcd.print("Sensors Ready");         // Success message

  delay(2000);                        // Pause to show message
}

// ======================================================================
// LOOP FUNCTION - Runs repeatedly forever after setup()
// ======================================================================
void loop()
{
  long irValue = particleSensor.getIR();  
                                      // Read raw IR LED value from MAX30102 (main signal for heart beat detection)

  // --------------------- Heart Rate Detection ------------------------
  if (checkForBeat(irValue))          // checkForBeat() is from heartRate.h – detects pulse peak in IR signal
  {                                   // Returns true when a new heartbeat is found
    long delta = millis() - lastBeat; // Calculate time (ms) since last beat
    lastBeat = millis();              // Update timestamp of this beat

    beatsPerMinute = 60 / (delta / 1000.0);  
                                      // Convert time between beats (ms) → BPM (beats per minute)

    if (beatsPerMinute < 255 && beatsPerMinute > 20)  
    {                                 // Basic sanity filter: ignore unrealistic values
      beatAvg = beatsPerMinute;       // Update average (simple assignment – can be improved with smoothing)
    }
  }

  // --------------------- Read Temperature ----------------------------
  temperature = mlx.readObjectTempC();  
                                      // Read object (body) temperature in Celsius from MLX90614

  // --------------------- SpO2 (simulated for this demo) --------------
  spo2 = random(96,99);               // Fake SpO2 value between 96–98% (real SpO2 needs red+IR ratio calculation)

  // --------------------- Update LCD Display --------------------------
  lcd.clear();                        // Clear screen before new data

  lcd.setCursor(0,0);                 // Top row
  lcd.print("HR:");                   // Heart Rate label
  lcd.print(beatAvg);                 // Show averaged BPM
  lcd.print(" Sp:");                  // SpO2 label
  lcd.print(spo2);                    // Show SpO2 value

  lcd.setCursor(0,1);                 // Bottom row
  lcd.print("Temp:");                 // Temperature label
  lcd.print(temperature);             // Show temperature

  // --------------------- Print to Serial Monitor ---------------------
  Serial.print("Heart Rate: ");
  Serial.println(beatAvg);            // Print BPM

  Serial.print("Temperature: ");
  Serial.println(temperature);        // Print temp

  Serial.print("SpO2: ");
  Serial.println(spo2);               // Print SpO2

  // --------------------- Send Data to Firebase Realtime Database -----
  Firebase.RTDB.setInt(&fbdo, "Patients/Patient01/latest/HeartRate", beatAvg);  
                                      // Upload integer heart rate to /Patients/Patient01/latest/HeartRate

  Firebase.RTDB.setFloat(&fbdo, "Patients/Patient01/latest/Temperature", temperature);  
                                      // Upload float temperature

  Firebase.RTDB.setFloat(&fbdo, "Patients/Patient01/latest/SpO2", spo2);  
                                      // Upload float SpO2

  // Note: No error checking here – in production add: if (!success) { Serial.println(fbdo.errorReason()); }

  delay(5000);                        // Wait 5 seconds before next reading (sampling rate ~ every 5 sec)
}
