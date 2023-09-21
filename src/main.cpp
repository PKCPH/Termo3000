/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com  
*********/

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// Replace with your network credentials
const char* ssid = "E308";
const char* password = "98806829";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String readBME280Temperature() {
  // Read temperature as Celsius (the default)
  float t = sensors.getTempCByIndex(0);
  // Convert temperature to Fahrenheit
  //t = 1.8 * t + 32;
  if (isnan(t)) {    
    Serial.println("Failed to read from BME280 sensor!");
    return "";
  }
  else {
    Serial.println(t);
    return String(t);
  }
}

void setup() {
  // Start the Serial Monitor
  Serial.begin(115200);
  // Start the DS18B20 sensor
  sensors.begin();


  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

    // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

// Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readBME280Temperature().c_str());
  });

  
  // Start server
  server.begin();

}

void loop() {
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  //float temperatureF = sensors.getTempFByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");
  //Serial.print(temperatureF);
  //Serial.println("ºF");
  delay(1000);
}

