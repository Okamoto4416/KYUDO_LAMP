#include <WiFi.h>
#include <WiFiUdp.h>
#include "routerAP_config.hpp"
// #include "test_lamp.hpp"
#include "NetWorkMngr.hpp"
#include "PinEffects.hpp"

void toggleLamp();

// ピン設定
// const int buttonPin = 26;
// const int lampPin = 14;

DebouncedDigitalRead<> button{26}; // pin26 ボタン
PatternBlinker8bit lampBlink{14};  // pin14 ランプに接続

// 状態
// volatile bool lampState = false;  // ON/OFF状態
// volatile bool blinkState = false; // 点滅用
bool lampState = false; // ON/OFF状態

// // タイマ
// hw_timer_t *timer = NULL;
// portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

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

// // 割り込み処理（0.5秒ごと）
// void IRAM_ATTR onTimer()
// {
//     portENTER_CRITICAL_ISR(&timerMux);
//     if (lampState)
//     {
//         blinkState = !blinkState;
//         digitalWrite(lampPin, blinkState);
//     }
//     portEXIT_CRITICAL_ISR(&timerMux);
// }

void setup()
{
    Serial.begin(115200);

    pinMode(button.pin, INPUT_PULLUP);
    pinMode(lampBlink.pin, OUTPUT);

    ////////////////////////////////////////////////////////////////////////////////
    // WiFiの準備
    // 自分ip,相手ip,パケット処理する関数の登録
    NetworkMngr.init(lamp_ip, remote_ip, processPacket);

    // // タイマ設定（80MHz / 80 = 1MHz → 1μs）
    // timer = timerBegin(0, 80, true);
    // timerAttachInterrupt(timer, &onTimer, true);
    // timerAlarmWrite(timer, 500000, true); // 0.5秒
    // timerAlarmEnable(timer);

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
            // 押された瞬間

            Serial.print("button pushed ");
            toggleLamp();
        }
        prevButton = currentButton;
    }
}