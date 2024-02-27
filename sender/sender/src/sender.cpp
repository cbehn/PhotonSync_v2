#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h> // Library for handling JSON
#include <FS.h>
#include "SPIFFS.h" // Library for using SPIFFS

#define CONFIG_FILE "/config.json"
#define RED_BUTTON 12
#define BLUE_BUTTON 13
#define VERBOS true

// Define variables for configuration with default values
int Channel = 0;
int Num_Pixels = 1;                     // THe number of Pixels we will be displaying
int Pixel_Index = 0;                    // The index of the first pixel we will be displaying
uint8_t Start_Color[3] = {255, 255, 0}; // Default shows red color to indicated config was not loaded
uint8_t Receiver_Address[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

// Define structure to hold the data to be received
typedef struct __attribute__((packed))
{
  uint8_t index;
  uint8_t red;
  uint8_t green;
  uint8_t blue; // Data received
} Pixel;

// Global Objects
String success;
esp_now_peer_info_t peerInfo;

// Prototype Functions
void sendFade(Pixel startColor, Pixel endColor, int steps, int duration);
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
  // Serial.println("Loop started...");

  // Send first data: 0, 255, 0, 0
  Pixel red;
  red.index = 0; // First hexadecimal value
  red.red = 255; // Second hexadecimal value
  red.green = 0; // Third hexadecimal value
  red.blue = 0;  // Fourth hexadecimal value

  // Send second data: 0, 0, 0, 255
  Pixel blue;
  blue.index = 1;  // First hexadecimal value
  blue.red = 0;    // Second hexadecimal value
  blue.green = 0;  // Third hexadecimal value
  blue.blue = 255; // Fourth hexadecimal value

  

  // Send data to all peers
  if (digitalRead(RED_BUTTON) == HIGH)
  {
    Serial.println("Sending data Red fade");
    Pixel black = {0, 0, 0, 0};

    sendFade(red, black, 50, 1000);
    delay(100);
  }
  else if (digitalRead(BLUE_BUTTON) == HIGH)
  {
    Serial.println("Sending data Blue fade");
    Pixel black = {1, 0, 0, 0};

    sendFade(blue, black, 50, 1000);
    delay(100);
  }
  delay(1);
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  // This device is only a sender, not handling received data
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

void sendFade(Pixel startColor, Pixel endColor, int steps, int duration)
{
  // Calculate step size for each color component
  int stepR = (endColor.red - startColor.red) / steps;
  int stepG = (endColor.green - startColor.green) / steps;
  int stepB = (endColor.blue - startColor.blue) / steps;
  
  int index = startColor.index;

  // Calculate delay between steps
  int stepDelay = duration / steps;

  // Perform fading
  for (int i = 0; i <= steps; i++)
  {
    // Calculate intermediate color values based on current step
    uint8_t r = startColor.red + (stepR * i);
    uint8_t g = startColor.green + (stepG * i);
    uint8_t b = startColor.blue + (stepB * i);

    // Create a Pixel structure to store the current color
    Pixel currentColor = {index, r, g, b};

    // Send the current color data using ESP-NOW
    esp_err_t result = esp_now_send(Receiver_Address, (uint8_t *)&currentColor, sizeof(currentColor));

    if (result != ESP_OK)
    {
      Serial.println("Error sending color data over ESP-NOW");
      return;
    }

    // Delay for a short time to observe the color transition
    delay(stepDelay);
  }
  esp_err_t result = esp_now_send(Receiver_Address, (uint8_t *)&endColor, sizeof(endColor));

  if (result != ESP_OK)
  {
    Serial.println("Error sending color data over ESP-NOW");
    return;
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

  configFile.close();
}