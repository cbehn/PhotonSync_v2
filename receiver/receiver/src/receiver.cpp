#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h> // Library for handling JSON
#include <FS.h>
#include "SPIFFS.h" // Library for using SPIFFS
#include <iostream>
#include <unordered_map>
#include <iterator>
#include <stdio.h>

#define CONFIG_FILE "/config.json"
#define NEOPIXEL_PIN 23
#define VERBOS false
#define NUM_LED 8 // The number of physical LEDs connected
#define MAX_NUM_PIXELS 64

// Define variables for configuration with default values
int Channel = 0;
int Num_Pixels = 1;                   // THe number of Pixels we will be displaying
int Pixel_Index = 0;                  // The index of the first pixel we will be displaying
uint8_t Start_Color[3] = {255, 0, 0}; // Default shows red color to indicated config was not loaded
uint8_t Receiver_Address[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

// NeoPixel configuration
Adafruit_NeoPixel pixelOutput(NUM_LED, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800); // Placeholder values, will be initialized later

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
std::unordered_map<int, Pixel> colorMap; // Dictionary holding all recieved pixel values. Key = pixel index, Value= Pixel
int ledMap[MAX_NUM_PIXELS];

// Function prototypes
void fillFadeToBlack(unsigned long fadeTime, Pixel color);
void fillFadeFromBlack(unsigned long fadeTime, Pixel color);
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void loadConfig();
void mapLED();
//---------------------------------------------------------------------------------------

void setup()
{
  // Begin Setup
  Serial.begin(115200);
  Serial.println("Setup started...");
  Serial.print("Running on core: ");
  Serial.println(xPortGetCoreID());

  // Initialize SPIFFS
  if (SPIFFS.begin(true) != true)
  {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }
  else if (VERBOS)
  {
    Serial.println("Successfully mounted SPIFFS girl, read them files");
  }

  // Load configuration from JSON file
  loadConfig();
  mapLED();

  currentColor = {0, Start_Color[0], Start_Color[1], Start_Color[2]};
  pixelOutput.updateLength(NUM_LED);

  // pixelOutput = new Adafruit_NeoPixel(Num_Pixels, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to WiFi...");

  // Manually define MAC address
  Serial.print("[OLD] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  if (ESP_OK != esp_wifi_set_mac(WIFI_IF_STA, &Receiver_Address[0]))
  {
    Serial.println("Failed to reassign MAC Address, using default");
    Serial.println(Receiver_Address[0]);
  }
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  // Initialize NeoPixel
  Serial.println("Starting light");
  pixelOutput.begin();
  pixelOutput.setBrightness(255);
  pixelOutput.fill(pixelOutput.Color(currentColor.red, currentColor.green, currentColor.blue)); // Set initial color of LEDs
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
  // Serial.println("Starting Loop");
  // Serial.print("Running on core: ");
  // Serial.println(xPortGetCoreID());

  if (!newData)
  {
    delay(1500);
    // Serial.println("No new data");
    fillFadeToBlack(500, currentColor); // Fade off to red in 2 seconds
    delay(500);
    fillFadeFromBlack(500, currentColor); // Fade back to original color in 2 seconds
    return;
  }

  for (int i = 0; i < NUM_LED; i++)
  {
    pixelOutput.setPixelColor(i, colorMap[ledMap[i]].red, colorMap[ledMap[i]].green, colorMap[ledMap[i]].blue);
    // Serial.println(i);
  }
  pixelOutput.setBrightness(255);
  pixelOutput.show();
  
  // for (int i = 0; i < Num_Pixels; i++)
  // {
  //   String text = "ColorMap- " + String(i) + ": " + String(colorMap[i].red) + ", " + String(colorMap[i].green) + ", " + String(colorMap[i].blue);
  //   Serial.println(text);
  // }
  // String text1 = "ColorMap- 0: " + String(colorMap[0].red) + ", " + String(colorMap[0].green) + ", " + String(colorMap[0].blue);
  // String text2 = "ColorMap- 1: " + String(colorMap[1].red) + ", " + String(colorMap[1].green) + ", " + String(colorMap[1].blue);
  // Serial.println(text1);
  // Serial.println(text2);
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  // Serial.println("Data Received, running onDataDecv");
  // Serial.print("Running on core: ");
  // Serial.println(xPortGetCoreID());

  // Check if the received data is the correct size
  if (data_len % sizeof(Pixel) == 0)
  {
    int numPixels = data_len / sizeof(Pixel); // Calculate the number of pixels
    Pixel receivedData[numPixels];            // Local array to hold received data

    // Copy received data to local variable
    memcpy(receivedData, data, data_len);

    for (int i = 0; i < numPixels; ++i)
    {
      int index = receivedData[i].index;
      colorMap[index] = receivedData[i];
    }

    newData = true;
  }
}

void loadConfig()
{
  Serial.println("Starting loadConfig");
  Serial.print("Running on core: ");
  Serial.println(xPortGetCoreID());

  if (!SPIFFS.exists(CONFIG_FILE))
  {
    Serial.println("Config file does not exist");
    return;
  }
  else if (VERBOS)
  {
    Serial.println("Your file exists at least");
  }

  File configFile = SPIFFS.open(CONFIG_FILE, "r");
  if (!configFile)
  {
    Serial.println("Failed to open config file");
    return;
  }
  else if (VERBOS)
  {
    Serial.println("we read them files to filth girl");
  }

  size_t size = configFile.size();
  if (size > 1024)
  {
    Serial.println("Config file size is too large");
    return;
  }

  if (VERBOS)
  {
    Serial.print("Config file size: ");
    Serial.print(size);
    Serial.println(" , she's a thiccc bitch");
  }

  // Allocate a buffer to store contents of the file
  std::unique_ptr<char[]> buf(new char[size]);

  // Read data from the file into the buffer
  configFile.readBytes(buf.get(), size);

  // Parse the JSON object in the file
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error)
  {
    Serial.println("Failed to parse config file");
    return;
  }
  else if (VERBOS)
  {
    Serial.println("Please welcum to the stage, J. SON-DHEIM!");
  }

  // Extract values
  Channel = doc["Channel"];
  Num_Pixels = doc["Num_Pixels"];
  Pixel_Index = doc["Pixel_Index"];
  const char *receiverAddressStr = doc["Receiver_Address"];
  sscanf(receiverAddressStr, "%x:%x:%x:%x:%x:%x",
         &Receiver_Address[0], &Receiver_Address[1], &Receiver_Address[2],
         &Receiver_Address[3], &Receiver_Address[4], &Receiver_Address[5]);
  JsonArray startColor = doc["Start_Color"];
  for (int i = 0; i < 3; i++)
  {
    Start_Color[i] = startColor[i];
  }
  String message = "Red: " + String(Start_Color[0]) + ", Green: " + String(Start_Color[1]) + ", Blue: " + String(Start_Color[2]);
  Serial.println(message);

  configFile.close();
}

void mapLED()
// This function assigns the colorMap key fro the appropriate Pixel index to each LED
{
  Serial.println("Starting mapLED");
  Serial.print("Running on core: ");
  Serial.println(xPortGetCoreID());

  int groupSize = NUM_LED / Num_Pixels;
  int remainder = NUM_LED % Num_Pixels;

  for (int i = 0; i < Num_Pixels; i++) // for each pixel that will be displayed
  {
    int usedRemainders = 0;
    int ii = 0;
    for (ii; ii < groupSize; ii++) // set the group size worth of
    {
      ledMap[(i * groupSize) + ii + usedRemainders] = Pixel_Index + i; // This should be the key in colorMap that matches the correct color to be displayed.
    }
    if (remainder < 0)
    {
      ledMap[(i * groupSize) + ii + usedRemainders + 1];
      remainder = remainder - 1;
    }
  }
  if (true)
  {
    Serial.print("LED Map: ");
    for (int i = 0; i < NUM_LED; i++)
    {
      Serial.print(ledMap[i]);
      Serial.print(", ");
    }
    Serial.println();
  }

  Serial.println("LED to Pixel map generated");
}

void fillFadeToBlack(unsigned long fadeTime, Pixel color)
{
  unsigned long startTime = millis();
  for (int i = 255; i >= 0; i--)
  {
    pixelOutput.setBrightness(i);
    pixelOutput.fill(pixelOutput.Color(color.red, color.green, color.blue));
    pixelOutput.show();
    unsigned long elapsedTime = millis() - startTime;
    delay((fadeTime / 255) - (elapsedTime % (fadeTime / 255)));
  }
  pixelOutput.fill(0, 0, 0);
  pixelOutput.show();
}

void fillFadeFromBlack(unsigned long fadeTime, Pixel color)
{
  unsigned long startTime = millis();
  for (int i = 0; i <= 255; i++)
  {
    pixelOutput.setBrightness(i);
    pixelOutput.fill(pixelOutput.Color(color.red, color.green, color.blue));
    pixelOutput.show();
    unsigned long elapsedTime = millis() - startTime;
    delay((fadeTime / 255) - (elapsedTime % (fadeTime / 255)));
  }
}
