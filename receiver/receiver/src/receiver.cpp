#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h> // Library for handling JSON
#include <FS.h>
#include "SPIFFS.h" // Library for using SPIFFS

#define CONFIG_FILE "/config.json"
#define NEOPIXEL_PIN 15
#define VERBOS true

// Define variables for configuration with default values
int CHANNEL = 0;
int NUM_PIXELS = 1;
int PIXEL_INDEX = 0;
uint8_t START_COLOR[3] = {255, 0, 100};
uint8_t RECEIVER_ADDRESS[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

// NeoPixel configuration
Adafruit_NeoPixel pixelOutput(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800); // Placeholder values, will be initialized later

// Define structure to hold the data to be received
typedef struct __attribute__((packed))
{
  uint8_t index;
  uint8_t red;
  uint8_t green;
  uint8_t blue; // Data received
} Pixel;

// Global Objects
Pixel currentColor;
bool newData = false;

// Function prototypes
void fadeToColor(unsigned long fadeTime, uint8_t r, uint8_t g, uint8_t b);
void fadeToOriginal(unsigned long fadeTime, uint8_t r, uint8_t g, uint8_t b);
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void loadConfig();

void setup()
{
  // Begin Setup
  Serial.begin(115200);
  if (VERBOS)
  {
    Serial.println("Setup started...");
  }
  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }
  else
  {
    Serial.println("Successfully mounted SPIFFS");
  }

  // Load configuration from JSON file
  loadConfig();
  currentColor = {0, START_COLOR[0], START_COLOR[1], START_COLOR[2]};
  pixelOutput.updateLength(NUM_PIXELS);
  // pixelOutput = new Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to WiFi...");

  // Manually define MAC address
  Serial.print("[OLD] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  esp_wifi_set_mac(WIFI_IF_STA, &RECEIVER_ADDRESS[0]);
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  // Initialize NeoPixel
  Serial.println("Starting light");
  pixelOutput.begin();
  pixelOutput.setPixelColor(PIXEL_INDEX, pixelOutput.Color(currentColor.red, currentColor.green, currentColor.blue)); // Set initial color to white
  pixelOutput.show();

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback function to handle received data
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Setup completed.");
}

void loop()
{
  if (!newData)
  {
    // Serial.println("No new data");
    fadeToColor(255, currentColor.red, currentColor.green, currentColor.blue);    // Fade off to red in 2 seconds
    fadeToOriginal(255, currentColor.red, currentColor.green, currentColor.blue); // Fade back to original color in 2 seconds
  }
  else if (newData)
  {
    // Serial.println("Updated Received");
    // newData = false;
  }

  //                                                                   // Wait for 2 seconds

  // Slave device only listens for messages, nothing to do in loop
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  // Check if the received data is the correct size
  if (data_len == sizeof(Pixel))
  {
    Pixel receivedData;
    memcpy(&receivedData, data, sizeof(Pixel));
    currentColor = receivedData;

    // Set NeoPixel color based on received data
    uint8_t red = receivedData.red;     // Second hexadecimal value
    uint8_t green = receivedData.green; // Third hexadecimal value
    uint8_t blue = receivedData.blue;   // Fourth hexadecimal value

    pixelOutput.setPixelColor(0, red, green, blue);
    pixelOutput.show();

    Serial.print("Received data: R=");
    Serial.print(red);
    Serial.print(", G=");
    Serial.print(green);
    Serial.print(", B=");
    Serial.println(blue);

    newData = true;
  }
}

void fadeToColor(unsigned long fadeTime, uint8_t r, uint8_t g, uint8_t b)
{
  unsigned long startTime = millis();
  for (int i = 255; i >= 0; i--)
  {
    pixelOutput.setBrightness(i);
    pixelOutput.setPixelColor(0, r, g, b);
    pixelOutput.show();
    unsigned long elapsedTime = millis() - startTime;
    delay((fadeTime / 255) - (elapsedTime % (fadeTime / 255)));
  }
}

void fadeToOriginal(unsigned long fadeTime, uint8_t r, uint8_t g, uint8_t b)
{
  unsigned long startTime = millis();
  for (int i = 0; i <= 255; i++)
  {
    pixelOutput.setBrightness(i);
    pixelOutput.setPixelColor(0, r, g, b);
    pixelOutput.show();
    unsigned long elapsedTime = millis() - startTime;
    delay((fadeTime / 255) - (elapsedTime % (fadeTime / 255)));
  }
}

void loadConfig()
{
  if (!SPIFFS.exists(CONFIG_FILE))
  {
    Serial.println("Config file does not exist");
    return;
  }

  File configFile = SPIFFS.open(CONFIG_FILE, "r");
  if (!configFile)
  {
    Serial.println("Failed to open config file");
    return;
  }

  size_t size = configFile.size();
  if (size > 1024)
  {
    Serial.println("Config file size is too large");
    return;
  }

  Serial.print("Config file size: ");
  Serial.println(size);

  // Allocate a buffer to store contents of the file
  std::unique_ptr<char[]> buf(new char[size]);

  // Read data from the file into the buffer
  configFile.readBytes(buf.get(), size);

/*   // Print file content for debugging
  Serial.println("File content:");
  Serial.println(buf.get()); */

  // Parse the JSON object in the file
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error)
  {
    Serial.println("Failed to parse config file");
    return;
  }

  // Extract values
  CHANNEL = doc["CHANNEL"];
  NUM_PIXELS = doc["NUM_PIXELS"];
  PIXEL_INDEX = doc["PIXEL_INDEX"];
  const char* receiverAddressStr = doc["RECEIVER_ADDRESS"];
  sscanf(receiverAddressStr, "%x:%x:%x:%x:%x:%x", 
         &RECEIVER_ADDRESS[0], &RECEIVER_ADDRESS[1], &RECEIVER_ADDRESS[2],
         &RECEIVER_ADDRESS[3], &RECEIVER_ADDRESS[4], &RECEIVER_ADDRESS[5]);
  JsonArray startColor = doc["START_COLOR"];
  for (int i = 0; i < 3; i++)
  {
    START_COLOR[i] = startColor[i];
  }

  configFile.close();
}