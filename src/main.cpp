#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid     = "el2g-6cec04";
const char* password = "402fng1001";

IPAddress local_IP(192, 168, 0, 60);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

const char* receiverIP = "192.168.0.50";
const int udpPort = 12345;

WiFiUDP udp;

const int buttonPin = 21; // 好きなGPIO
bool prevButton = false;

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP);

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("device_IN ready");
}

void loop() {
  bool currentButton = !digitalRead(buttonPin);

  // 押した瞬間だけ送る（エッジ検出）
  if (currentButton && !prevButton) {
    udp.beginPacket(receiverIP, udpPort);
    udp.print("TRUE");
    udp.endPacket();

    Serial.println("Sent TRUE");
  }

  prevButton = currentButton;
  delay(10);
}