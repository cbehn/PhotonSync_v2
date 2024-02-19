#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

#define CHANNEL 0 // ESP-NOW channel

bool newData = false;

// Define structure to hold the data to be received
typedef struct __attribute__((packed)) {
  uint8_t data[4]; // Data received
} MyData;

// NeoPixel configuration
#define NEOPIXEL_PIN 15 // Pin connected to the NeoPixel
#define NUM_PIXELS 1    // Number of NeoPixels

Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Prototype declaration
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);

void setup() {
  Serial.begin(115200);
  Serial.println("Setup started...");

  // Manually define MAC address
  uint8_t macAddress[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}; // Replace with your desired MAC address
    Serial.println("Mac address set as 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC");

  
  // Initialize NeoPixel
  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(255, 255, 255)); // Set initial color to white
  pixels.show();

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to WiFi...");

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register callback function to handle received data
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Setup completed.");
}

void loop() {
  if (!newData) {
    Serial.println("No new data");
  } else if (newData) {
    Serial.println("Updated Received");
    newData = false;
  }

    // Slave device only listens for messages, nothing to do in loop
  delay(1000);
}

// Callback function to handle received data
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  // Check if the received data is the correct size
  if (data_len == sizeof(MyData)) {
    MyData receivedData;
    memcpy(&receivedData, data, sizeof(MyData));

    // Set NeoPixel color based on received data
    uint8_t red = receivedData.data[1];   // Second hexadecimal value
    uint8_t green = receivedData.data[2]; // Third hexadecimal value
    uint8_t blue = receivedData.data[3];  // Fourth hexadecimal value

    pixels.setPixelColor(0, pixels.Color(red, green, blue));
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
