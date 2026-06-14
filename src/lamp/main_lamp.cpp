#include <WiFi.h>
#include <WiFiUdp.h>
#include "routerAP_config.hpp"
// #include "test_lamp.hpp"
#include "NetWorkMngr.hpp"
#include "PinEffects.hpp"

void toggleLamp();

// ピン設定
DebouncedDigitalRead<> button{26}; // pin 26 ボタン読み取り
PatternBlinker8bit lampBlink{14};  // pin 14 ランプに接続

// ランプ状態
bool lampState = false; // ON/OFF状態

// packet処理
// NetworkMngr.initで登録したので、パケットが来たら呼ばれる
void processPacket(JsonObjectConst rx_json)
{
    auto type = rx_json["type"].as<const char *>(); // 項目typeの値の取り出し

    if (type && strcmp(type, "toggle") == 0)
    {
        // type:"toggle"だったら

        toggleLamp();
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(button.pin, INPUT_PULLUP);
    pinMode(lampBlink.pin, OUTPUT);

    ////////////////////////////////////////////////////////////////////////////////
    // WiFiの準備
    // 自分ip,相手ip,パケット処理する関数の登録
    NetworkMngr.init(lamp_ip, remote_ip, processPacket);

    Serial.println("device_OUT ready");
}

void toggleLamp()
{
    lampState = !lampState;

    if (lampState)
    {
        lampBlink.setPattern(0b11110000); // 点滅パターン設定
        lampBlink.restart();
    }
    else
    {
        lampBlink.setPattern(0b0); // 消灯パターン設定
        lampBlink.restart();
    }

    Serial.print("Lamp: ");
    Serial.println(lampState ? "ON" : "OFF");
}

void loop()
{

    // update//////////////////////////////////////////////////////////////
    NetworkMngr.update();
    button.update();
    lampBlink.update();

    // ボタン監視
    {
        static bool prevButton = false;
        const bool currentButton = !button.read();
        if (true == currentButton && false == prevButton)
        {
            //押された瞬間

            Serial.print("button pushed ");
            toggleLamp();
        }
        prevButton = currentButton;
    }
}