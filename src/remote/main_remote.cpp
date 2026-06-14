#include <WiFi.h>
#include <WiFiUdp.h>
#include "routerAP_config.hpp"
// #include "test_remote.hpp"
#include "NetWorkMngr.hpp"
#include "PinEffects.hpp"

constexpr unsigned buttonPin = 21;      // ボタンのpin
constexpr unsigned NWpacketLEDpin = 19; // ネットワーク関係LEDパケットが来たら一瞬光る
PulseOutput<5> NWpacketLEDpulse(NWpacketLEDpin);
constexpr unsigned NWstateLEDpin = 18; // ネットワーク関係LED 状態の表示
PatternBlinker8bit NWstateLEDblink(NWstateLEDpin);

// packet処理
// NetworkMngr.initで登録したので、パケットが来たら呼ばれる
void processPacket(JsonObjectConst rx_json)
{
    auto type = rx_json["type"].as<const char *>(); // 項目typeの値の取り出し

    // パケットが来たのでパルス
    NWpacketLEDpulse.trigger();
}

void setup()
{
    Serial.begin(115200);

    // ピンのセット
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(NWpacketLEDpin, OUTPUT);
    pinMode(NWstateLEDpin, OUTPUT);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // WiFiの準備
    // 自分ip,相手ip,パケット処理する関数の登録
    NetworkMngr.init(remote_ip, lamp_ip, processPacket, processPacket);

    Serial.println("device_IN ready");
}

void loop()
{
    // update//////////////////////////////////////////////////////////////////////
    NetworkMngr.update();
    NWpacketLEDpulse.update();
    NWstateLEDblink.update();

    // NW状態LED
    {
        static auto prevState = NetworkMngr.get_state();
        auto currentState = NetworkMngr.get_state();

        if (prevState != currentState)
        {
            // 状態が変化したら

            switch (currentState)
            {
            case NetworkMngr_t::State_t::off:
                NWstateLEDblink.setPattern(0);
                break;

            case NetworkMngr_t::State_t::connectingAP:
                NWstateLEDblink.setPattern(0b00001111);
                break;

            case NetworkMngr_t::State_t::discoveringPeer:
            case NetworkMngr_t::State_t::unstable:
            case NetworkMngr_t::State_t::connected:
                NWstateLEDblink.setPattern(0b10100000);
                break;

            default:
                NWstateLEDblink.setPattern(0b11111111);
                break;
            }
            NWstateLEDblink.restart();
        }
    }

    // ボタン監視 10msごと
    {
        static unsigned long buttonNextTime = millis();
        static bool prevButton = false;
        if (millisReached(buttonNextTime))
        {

            bool currentButton = !digitalRead(buttonPin);

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
            buttonNextTime += 10; // delay(10);
        }
    }
}