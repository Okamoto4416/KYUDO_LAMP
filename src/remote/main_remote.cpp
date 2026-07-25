#include <WiFi.h>
#include <WiFiUdp.h>
#include "routerAP_config.hpp"
// #include "test_remote.hpp"
#include "NetWorkMngr.hpp"
#include "PinEffects.hpp"

// pin
DebouncedDigitalRead<> button{21};             // pin21 ボタン
PulseOutput<20> NWpacketLEDpulse{19};          // pin19 ネットワーク関係LEDパケットが来たら一瞬(20ms)光る
PatternBlinker16bit NWstateLEDblink{18, 2000}; // pin18 ネットワーク関係LED 状態の表示
PulseOutput<10000> lampStateLEDpulse{22};      // pin22 ランプの状態を表示するLED(onだったら光る)(10秒パルス)

// packet処理
// NetworkMngr.initで登録したので、パケットが来たら呼ばれる
void processPacket_ack_measure_pulse(JsonObjectConst rx_json)
{
    auto type = rx_json["type"].as<const char *>(); // 項目typeの値の取り出し

    // typeがなかったら無視
    if (!type)
    {
        return;
    }

    if (type && strcmp(type, "ack_measure") == 0)
    {
        // ack_measureパケットが来たらパルス
        NWpacketLEDpulse.trigger();
    }
    else if (strcmp(type, "lamp_state") == 0)
    {
        // lamp_stateパケットが来たら状態ランプを操作

        if (rx_json["state"].is<bool>())
        {
            auto s = rx_json["state"].as<bool>();
            if (s)
            {
                // trueだったら10秒パルス
                lampStateLEDpulse.trigger();
            }
            else
            {
                // falseだったら消す
                lampStateLEDpulse.idle();
            }
        }
    }
}

void setup()
{
    Serial.begin(115200);

    // ピンのセット
    pinMode(button.pin, INPUT_PULLUP);
    pinMode(NWpacketLEDpulse.pin, OUTPUT);
    pinMode(NWstateLEDblink.pin, OUTPUT);
    pinMode(lampStateLEDpulse.pin, OUTPUT);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // WiFiの準備
    // 自分ip,相手ip,パケット処理する関数の登録
    NetworkMngr.init(
        remote_ip,
        lamp_ip,
        nullptr,
        processPacket_ack_measure_pulse);

    Serial.println("device_IN ready");
}

void loop()
{
    // update//////////////////////////////////////////////////////////////////////
    NetworkMngr.update();
    NWpacketLEDpulse.update();
    NWstateLEDblink.update();
    button.update();
    lampStateLEDpulse.update();

    // NW状態LED
    {
        switch (NetworkMngr.get_state())
        {
        case NetworkMngr_t::State_t::off:
            NWstateLEDblink.setPattern(0);
            break;

        case NetworkMngr_t::State_t::connectingAP:
            NWstateLEDblink.setPattern(0b0000000011111111);
            break;

        case NetworkMngr_t::State_t::discoveringPeer:
        case NetworkMngr_t::State_t::unstable:
        case NetworkMngr_t::State_t::connected:
            NWstateLEDblink.setPattern(0b1010000010100000);
            break;

        default:
            NWstateLEDblink.setPattern(0b1111111111111111);
            break;
        }
    }

    // ボタン監視
    {
        static bool prevButton = false;
        bool currentButton = !button.read();

        // 押した瞬間だけ送る（エッジ検出）
        if (currentButton && !prevButton)
        {
            //////////////////////////////////////////////////////////////////////////////////////////
            // 送信するときはjsonに書き込んで送る
            auto txjson = NetworkMngr.beginTxJson(); // 書き込むjsonを取得
            txjson["type"] = "toggle";               // type:"toggle"を書き込み
            NetworkMngr.sendTxJson();                // 送信
            //////////////////////////////////////////////////////////////////////////////////////////

            Serial.println("Sent TRUE");
        }

        prevButton = currentButton;
    }
}