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

float currentTemperature = 0.0;

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
AsyncWebSocket ws("/ws");

String readBME280Temperature() {
  // Read temperature as Celsius (the default)
  float t = sensors.getTempCByIndex(0);
  if (isnan(t)) {    
    Serial.println("Failed to read from DS18B20 sensor!");
  }
  else {
    currentTemperature = t; // Update the current temperature
  }
  return String(currentTemperature);
}

void sendTemperatureToClients() {
  String temperatureData = String(currentTemperature);
  ws.textAll(temperatureData);
}

// //besked fra client to server
// void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
//   AwsFrameInfo *info = (AwsFrameInfo *)arg;
//   if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
//     // Handle WebSocket message here if needed

//   }
// }

// //handler alle event der sker i websocket
// void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
//              void *arg, uint8_t *data, size_t len) {
//   switch (type) {
//     case WS_EVT_CONNECT:
//       Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
//       break;
//     case WS_EVT_DISCONNECT:
//       Serial.printf("WebSocket client #%u disconnected\n", client->id());
//       break;
//     case WS_EVT_DATA:
//       handleWebSocketMessage(arg, data, len);
//       break;
//     case WS_EVT_PONG:
//     case WS_EVT_ERROR:
//       break;
//   }
// }

//starter websocket
void initWebSocket() {
  //ws.onEvent(onEvent);
  server.addHandler(&ws);
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

  initWebSocket();

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
  if (!isnan(temperatureC) && currentTemperature != temperatureC) {
    currentTemperature = temperatureC;
  sendTemperatureToClients(); // Send temperature data to clients
  Serial.print(currentTemperature);
  Serial.println("ºC");
  }
  delay(1000);
}

