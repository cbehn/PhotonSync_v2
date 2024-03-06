#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "SPIFFS.h" // Library for using SPIFFS
#include <vector>
#include <sstream>
#include <string>
#include <cstdint>

#define CONFIG_FILE "/config.json"
#define VERBOS true
#define DATASIZE 250

// Define variables for configuration with default values
int Channel = 0;
uint8_t Receiver_Address[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

// Define structure to hold the data to be received
typedef struct __attribute__((packed))
{
  uint8_t index;
  uint8_t red;
  uint8_t green;
  uint8_t blue; // Data received
} Pixel;

String pixelToString(const Pixel &pixel)
{
  String result = String(pixel.index) + " " +
                  String(pixel.red) + " " +
                  String(pixel.green) + " " +
                  String(pixel.blue);
  return result;
}

Pixel stringToPixel(const String &str)
{
  Pixel pixel;
  int index, red, green, blue;
  sscanf(str.c_str(), "%d %d %d %d", &index, &red, &green, &blue);
  pixel.index = static_cast<uint8_t>(index);
  pixel.red = static_cast<uint8_t>(red);
  pixel.green = static_cast<uint8_t>(green);
  pixel.blue = static_cast<uint8_t>(blue);
  return pixel;
}

// Global Objects
String success;
esp_now_peer_info_t peerInfo;
Pixel currentColor; // Variable for current color

// Prototype Functions
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void loadConfig();
//---------------------------------------------------------------------------------------

void setup()
{
  // Begin Setup
  Serial.begin(115200);
  Serial.println("Setup started...");

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

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to WiFi...");

  // Initialize ESP Now
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  else if (VERBOS)
  {
    Serial.println("She's giving ESP-Ussy");
  }

  // Register callback
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, Receiver_Address, 6);
  peerInfo.channel = Channel;
  peerInfo.encrypt = false;
  if (VERBOS)
  {
    Serial.print("Broadcasting at the bus stop on ");
    for (int i = 0; i < 6; i++)
    {
      // Print each byte with a colon separator
      Serial.print(peerInfo.peer_addr[i], HEX);
      Serial.print(":");
    }
    Serial.println();
  }

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
  else if (VERBOS)
  {
    Serial.println("and you are on the list ;)");
  }
}

void loop()
{
  Serial.println("Starting Loop...");

  // Check for incoming serial data
  if (Serial.available())
  {
    String incomingString = Serial.readStringUntil('\n');
    Serial.println(incomingString);

    // Attempt to convert the string to a Pixel
    Pixel receivedPixel = stringToPixel(incomingString);

    // If conversion was successful, broadcast the pixel
    if (receivedPixel.index != 0)
    { // Check for valid index
      // Assign received pixel to currentColor
      currentColor = receivedPixel;

      // Broadcast the pixel over ESP-NOW
      esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t *)&currentColor, sizeof(currentColor));

      if (result == ESP_OK)
      {
        Serial.println("Pixel broadcast successful!");
      }
      else
      {
        Serial.println("Error broadcasting pixel...");
      }
    }
    else
    {
      Serial.println("Invalid pixel data received over serial.");
    }
  }
  else
  {
    delay(500);
  }

  // ... other loop code
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0)
  {
    success = "Delivery Success :)";
  }
  else
  {
    success = "Delivery Fail :(";
  }
}

void loadConfig()
{
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
  const char *receiverAddressStr = doc["Receiver_Address"];
  sscanf(receiverAddressStr, "%x:%x:%x:%x:%x:%x",
         &Receiver_Address[0], &Receiver_Address[1], &Receiver_Address[2],
         &Receiver_Address[3], &Receiver_Address[4], &Receiver_Address[5]);
  configFile.close();

  Serial.println("LEts go girls");
}
