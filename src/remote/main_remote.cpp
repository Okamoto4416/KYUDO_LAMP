#include <WiFi.h>
#include <WiFiUdp.h>
#include "routerAP_config.hpp"
#include "test_remote.hpp"


// const int udpPort = 12345;

// WiFiUDP udp;

// const int buttonPin = 21; // 好きなGPIO
// bool prevButton = false;

// void setup() {
//   Serial.begin(115200);

//   pinMode(buttonPin, INPUT_PULLUP);

//   WiFi.config(remote_ip, gateway, subnet);
//   WiFi.begin(ssid, pass);

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//   }

//   Serial.println("device_IN ready");
// }

// void loop() {
//   bool currentButton = !digitalRead(buttonPin);

//   // 押した瞬間だけ送る（エッジ検出）
//   if (currentButton && !prevButton) {
//     udp.beginPacket(lamp_ip, udpPort);
//     udp.print("TRUE");
//     udp.endPacket();

//     Serial.println("Sent TRUE");
//   }

//   prevButton = currentButton;
//   delay(10);
// }