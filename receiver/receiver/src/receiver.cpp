#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Adafruit_NeoPixel.h>

#define CHANNEL 0       // ESP-NOW channel
#define NEOPIXEL_PIN 15 // Pin connected to the NeoPixel
#define NUM_PIXELS 1    // Number of NeoPixels
#define PIXEL_INDEX 0
#define RECEIVER_ADDRESS               \
  {                                    \
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11 \
  }

// NeoPixel configuration
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

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
uint8_t mac_address[] = RECEIVER_ADDRESS;
bool newData = false;

// function prototypes
void fadeToColor(unsigned long fadeTime, uint8_t r, uint8_t g, uint8_t b);
void fadeToOriginal(unsigned long fadeTime, uint8_t r, uint8_t g, uint8_t b);
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);

void setup()
{
  Serial.begin(115200);
  Serial.println("Setup started...");

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to WiFi...");

  // Manually define MAC address
  Serial.print("[OLD] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  esp_wifi_set_mac(WIFI_IF_STA, &mac_address[0]);
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  // Initialize NeoPixel
  Serial.print("Starting light");
  currentColor.index = 255;
  currentColor.red = 255;
  currentColor.green = 255;
  currentColor.blue = 255;
  pixels.begin();
  pixels.setPixelColor(PIXEL_INDEX, pixels.Color(currentColor.red, currentColor.green, currentColor.blue)); // Set initial color to white
  pixels.show();

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
    //Serial.println("No new data");
    fadeToColor(255, currentColor.red, currentColor.green, currentColor.blue);    // Fade off to red in 2 seconds
    fadeToOriginal(255, currentColor.red, currentColor.green, currentColor.blue); // Fade back to original color in 2 seconds
  }
  else if (newData)
  {
    //Serial.println("Updated Received");
    //newData = false;
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

    pixels.setPixelColor(0, red, green, blue);
    pixels.show();

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
    pixels.setBrightness(i);
    pixels.setPixelColor(0, r, g, b);
    pixels.show();
    unsigned long elapsedTime = millis() - startTime;
    delay((fadeTime / 255) - (elapsedTime % (fadeTime / 255)));
  }
}

void fadeToOriginal(unsigned long fadeTime, uint8_t r, uint8_t g, uint8_t b)
{
  unsigned long startTime = millis();
  for (int i = 0; i <= 255; i++)
  {
    pixels.setBrightness(i);
    pixels.setPixelColor(0, r, g, b);
    pixels.show();
    unsigned long elapsedTime = millis() - startTime;
    delay((fadeTime / 255) - (elapsedTime % (fadeTime / 255)));
  }
}
