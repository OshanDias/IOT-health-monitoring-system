#include <ESP8266WiFi.h>      
#include <Wire.h>             
#include <Adafruit_MLX90614.h> 
// Include MLX90614 temperature sensor library
#define WIFI_SSID ""      
// Your WiFi network name
#define WIFI_PASSWORD ""  
// Your WiFi password
WiFiServer server(80);        
// Create a web server object running on port 80 (default HTTP port)

Adafruit_MLX90614 mlx = Adafruit_MLX90614();  
// Create MLX90614 sensor object

void setup() {

Serial.begin(115200);         
// Start serial communication for debugging

Wire.begin(D2,D1);            
// Initialize I2C communication
// D2 = SDA
// D1 = SCL

mlx.begin();                  

WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  
// Start connecting ESP8266 to WiFi network

Serial.print("Connecting");   

while (WiFi.status() != WL_CONNECTED){
  // Wait until WiFi is connected

  delay(500);                
  Serial.print(".");          
  // Print dots to show connection progress
}

Serial.println("WiFi Connected");  
// Print success message after connection

Serial.println(WiFi.localIP());    
// Print ESP8266 IP address 

server.begin();               
// Start the web server
}

void loop(){

WiFiClient client = server.available();  
// Check if any browser/client is requesting webpage

if (!client) return;          
// If no client → exit loop and wait

float temp = mlx.readObjectTempC();  
// Read body/object temperature from MLX90614 sensor

client.println("HTTP/1.1 200 OK");  
// Send HTTP response status

client.println("Content-Type: text/html");  
// Tell browser that we are sending HTML webpage

client.println();             
// Blank line (important in HTTP response)

client.println("<html><head>");
client.println("<meta http-equiv='refresh' content='2'>");  
// Auto refresh webpage every 2 seconds

client.println("<style>body{font-family:Arial;text-align:center;margin-top:50px;} h1{font-size:40px;} </style>");  
// Simple CSS styling for webpage design

client.println("</head><body>");

client.println("<h1>Smart Health Monitor</h1>");  
// Main webpage title

client.println("<h2>Body Temperature</h2>");  


client.print("<h1>");
client.print(temp);           


client.println(" C</h1>");    
// Display Celsius unit

client.println("</body></html>");  
// End HTML webpage

delay(1);                     
// Small delay for stability
}
