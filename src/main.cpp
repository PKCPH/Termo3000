/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com
*********/

#include <Arduino.h>
// DS18B20 libraries
#include <OneWire.h>
#include <DallasTemperature.h>

// Libraries to get time from NTP Server
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// Libraries for SD card
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// function declared
void getReadings();
void getTimeStamp();
void logSDCard();
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
// void send();

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;

//////////SDCard
// Define deep sleep options
uint64_t uS_TO_S_FACTOR = 1000000; // Conversion factor for micro seconds to seconds
// Sleep for 10 minutes = 600 seconds
uint64_t TIME_TO_SLEEP = 6;
// Define CS pin for the SD card module
#define SD_CS 5
// Save reading number on RTC memory
RTC_DATA_ATTR int readingID = 0;

String dataMessage;

unsigned long startMillis;
const unsigned long sleepDuration = 300000; // 5 minutes in milliseconds

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

float currentTemperature = 0.0;

// getting time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

// Replace with your network credentials
// const char* ssid = "E308";
// const char* password = "98806829";
const char *ssid = "Spiderman";
const char *password = "@C4mpD3tS3jl3r!";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String readBME280Temperature()
{
  // Read temperature as Celsius (the default)
  float t = sensors.getTempCByIndex(0);
  if (isnan(t))
  {
    Serial.println("Failed to read from DS18B20 sensor!");
  }
  else
  {
    currentTemperature = t; // Update the current temperature
  }
  return String(currentTemperature);
}

void sendTemperatureToClients()
{
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

// starter websocket
void initWebSocket()
{
  // ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup()
{
  // Start the Serial Monitor
  Serial.begin(115200);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");

  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);

  // Start the DS18B20 sensor
  sensors.begin();

  // Initialize SPIFFS
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  initWebSocket();

 

  // Initialize SD card
  SD.begin(SD_CS);
  if (!SD.begin(SD_CS))
  {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS))
  {
    Serial.println("ERROR - SD card initialization failed!");
    return; // init failed
  }



  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/data.txt");
  if (!file)
  {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/data.txt", "Reading ID, Date, Hour, Temperature \r\n");
  }
  else
  {
    Serial.println("File already exists");
  }
  file.close();

   // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html"); });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", readBME280Temperature().c_str()); });
 server.on("/downloadCSV", HTTP_GET, [](AsyncWebServerRequest *request){
  File file = SD.open("/data.txt");
  if (!file) {
     request->send(404, "text/plain", "CSV file not found");
     } else {
      request->send(SD, "/data.txt", "text/csv", false);
      file.close();
    }
  });


  // Start server
  server.begin();

  // Enable Timer wake_up
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // Start the DallasTemperature library
  sensors.begin();

  getReadings();
  getTimeStamp();
  logSDCard();

  // Increment readingID on every new reading
  readingID++;

  // Initialize your setup code here

  // Record the start time
  startMillis = millis();
}

void loop()
{
  // Check if 5 minutes (300,000 milliseconds) have passed
  if (millis() - startMillis >= sleepDuration)
  {
    // 5 minutes have passed, so initiate deep sleep
    // Start deep sleep
    Serial.println("DONE! Going to sleep now.");
    esp_deep_sleep_start();
  }
  // Your other loop code (if any) can go here
}

// Function to get temperature
void getReadings()
{
  sensors.requestTemperatures();
  currentTemperature = sensors.getTempCByIndex(0); // Temperature in Celsius
  // temperature = sensors.getTempFByIndex(0); // Temperature in Fahrenheit
  Serial.print("Temperature: ");
  Serial.println(currentTemperature);
}

// Function to get date and time from NTPClient
void getTimeStamp()
{
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
  Serial.println(timeStamp);
}

// Write to the SD card (DON'T MODIFY THIS FUNCTION)
void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message))
  {
    Serial.println("Message appended");
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}

// Write the sensor readings on the SD card
void logSDCard()
{
  dataMessage = String(readingID) + "," + String(dayStamp) + "," + String(timeStamp) + "," +
                String(currentTemperature) + "\r\n";
  Serial.print("Save data: ");
  Serial.println(dataMessage);
  /////////////////////////////////////////////////////////////
  appendFile(SD, "/data.txt", dataMessage.c_str());
}
