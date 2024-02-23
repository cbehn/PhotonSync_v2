#include <esp_now.h>
#include <WiFi.h>
#include <config.h>

#define CHANNEL 0 // ESP-NOW channel
#define RED_BUTTON 12
#define BLUE_BUTTON 13

// Global Variables
uint8_t broadcastAddress[] = RECEIVER_ADDRESS; // Universal
String success;
esp_now_peer_info_t peerInfo;

// Define structure to hold the data to be received
typedef struct __attribute__((packed))
{
  uint8_t index;
  uint8_t red;
  uint8_t green;
  uint8_t blue; // Data received
} Pixel;

// Prototype Functions
void sendFade(Pixel startColor, Pixel endColor, int steps, int duration);

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

void setup()
{
  Serial.begin(115200);
  Serial.println("Setup started...");

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to WiFi...");

  // Initialize ESP Now
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = CHANNEL;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop()
{
  //Serial.println("Loop started...");

  // Send first data: 0, 255, 0, 0
  Pixel red;
  red.index = 0; // First hexadecimal value
  red.red = 255; // Second hexadecimal value
  red.green = 0; // Third hexadecimal value
  red.blue = 0;  // Fourth hexadecimal value

  // Send second data: 0, 0, 0, 255
  Pixel blue;
  blue.index = 0;  // First hexadecimal value
  blue.red = 0;    // Second hexadecimal value
  blue.green = 0;  // Third hexadecimal value
  blue.blue = 255; // Fourth hexadecimal value

  Pixel black = {0, 0, 0, 0};

  // Send data to all peers
  if (digitalRead(RED_BUTTON) == HIGH)
  {
    Serial.println("Sending data Red fade");

    sendFade(red, black, 50, 1000);
    delay(100);
  }
  else if (digitalRead(BLUE_BUTTON) == HIGH)
  {
    Serial.println("Sending data Blue fade");

    sendFade(blue, black, 50, 1000);
    delay(100);
  }
  delay(1);
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  // This device is only a sender, not handling received data
}

void sendFade(Pixel startColor, Pixel endColor, int steps, int duration)
{
  // Calculate step size for each color component
  int stepR = (endColor.red - startColor.red) / steps;
  int stepG = (endColor.green - startColor.green) / steps;
  int stepB = (endColor.blue - startColor.blue) / steps;

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
    Pixel currentColor = {0, r, g, b};

    // Send the current color data using ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&currentColor, sizeof(currentColor));

    if (result != ESP_OK)
    {
      Serial.println("Error sending color data over ESP-NOW");
      return;
    }

    // Delay for a short time to observe the color transition
    delay(stepDelay);
  }
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&endColor, sizeof(endColor));

    if (result != ESP_OK)
    {
      Serial.println("Error sending color data over ESP-NOW");
      return;
    }
}