#include <WiFi.h>
#include <WiFiUdp.h>
#include "routerAP_config.hpp"
#include "test_lamp.hpp"

// const int udpPort = 12345;

// WiFiUDP udp;
// char incomingPacket[255];

// // ピン設定
// const int buttonPin = 26;
// const int lampPin   = 14;

// // 状態
// volatile bool lampState = false;  // ON/OFF状態
// volatile bool blinkState = false; // 点滅用

// bool prevButton = false;

// // タイマ
// hw_timer_t * timer = NULL;
// portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// // 割り込み処理（0.5秒ごと）
// void IRAM_ATTR onTimer() {
//   portENTER_CRITICAL_ISR(&timerMux);
//   if (lampState) {
//     blinkState = !blinkState;
//     digitalWrite(lampPin, blinkState);
//   }
//   portEXIT_CRITICAL_ISR(&timerMux);
// }

// void setup() {
//   Serial.begin(115200);

//   pinMode(buttonPin, INPUT_PULLUP);
//   pinMode(lampPin, OUTPUT);

//   //WiFi
//   WiFi.config(lamp_ip, gateway, subnet);
//   WiFi.begin(ssid, pass);
//   while (WiFi.status() != WL_CONNECTED) {
//     Serial.println("Failed");
//     delay(500);
//   }

//   udp.begin(udpPort);

//   // タイマ設定（80MHz / 80 = 1MHz → 1μs）
//   timer = timerBegin(0, 80, true);
//   timerAttachInterrupt(timer, &onTimer, true);
//   timerAlarmWrite(timer, 500000, true); // 0.5秒
//   timerAlarmEnable(timer);

//   Serial.println("device_OUT ready");
// }

// void toggleLamp() {
//   portENTER_CRITICAL(&timerMux);
//   lampState = !lampState;

//   if (!lampState) {
//     digitalWrite(lampPin, LOW);
//     blinkState = false;
//   }

//   portEXIT_CRITICAL(&timerMux);

//   Serial.print("Lamp: ");
//   Serial.println(lampState ? "ON" : "OFF");
// }

// void loop() {
//   // ボタン監視
//   bool currentButton = !digitalRead(buttonPin);
//   if (currentButton && !prevButton) {
//     Serial.print("button pushed ");
//     toggleLamp();
//   }
//   prevButton = currentButton;

//   // UDP受信
//   int packetSize = udp.parsePacket();
//   if (packetSize) {
//     int len = udp.read(incomingPacket, 255);
//     if (len > 0) incomingPacket[len] = '\0';

//     Serial.print("Received: ");
//     Serial.println(incomingPacket);

//     if (strcmp(incomingPacket, "TRUE") == 0) {
//       toggleLamp();
//     }
//   }

//   delay(10);
// }