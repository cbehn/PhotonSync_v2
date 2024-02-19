#include <esp_now.h>
#include <WiFi.h>

#define CHANNEL 0  // ESP-NOW channel

uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  // Universal
uint8_t receiverAddress[] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC };   // ESP32

String success;

// Define structure to hold the data to be sent
typedef struct __attribute__((packed)) {
  uint8_t data[4];  // Data to be sent
} MyData;


void setup() {
  Serial.begin(115200);
  Serial.println("Setup started...");

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to WiFi...");


  // Initialize ESP Now
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  

  // Register callback
  esp_now_register_send_cb(OnDataSent);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0) {
    success = "Delivery Success :)";
  } else {
    success = "Delivery Fail :(";
  }
}

void loop() {
  Serial.println("Loop started...");

  // Send first data: 0, 255, 0, 0
  MyData sendData1;
  sendData1.data[0] = 0;    // First hexadecimal value
  sendData1.data[1] = 255;  // Second hexadecimal value
  sendData1.data[2] = 0;    // Third hexadecimal value
  sendData1.data[3] = 0;    // Fourth hexadecimal value
  Serial.println("Sending data: 0, 255, 0, 0");

  // Send data to all peers
  esp_err_t result1 = esp_now_send(broadcastAddress, (uint8_t *)&sendData1, sizeof(sendData1));
  if (result1 == ESP_OK) {
    Serial.println("Data sent successfully");
  } else {
    Serial.print("Error sending data. Error code: ");
    Serial.println(result1);
  }

  delay(1000);  // Wait 1000ms

  // Send second data: 0, 0, 0, 255
  MyData sendData2;
  sendData2.data[0] = 0;    // First hexadecimal value
  sendData2.data[1] = 0;    // Second hexadecimal value
  sendData2.data[2] = 0;    // Third hexadecimal value
  sendData2.data[3] = 255;  // Fourth hexadecimal value
  Serial.println("Sending data: 0, 0, 0, 255");

  // Send data to all peers
  esp_err_t result2 = esp_now_send(broadcastAddress, (uint8_t *)&sendData2, sizeof(sendData2));
  if (result2 == ESP_OK) {
    Serial.println("Data sent successfully");
  } else {
    Serial.print("Error sending data. Error code: ");
    Serial.println(result2);
  }

  Serial.println("Loop completed.");
  delay(1000);  // Wait 1000ms before repeating
}

// Callback function to handle received data
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  // This device is only a sender, not handling received data
}
