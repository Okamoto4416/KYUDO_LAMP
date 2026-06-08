#include <WiFi.h>
#include <WiFiUdp.h>
#include "routerAP_config.hpp"
// #include "test_remote.hpp"
#include "NetWorkMngr.hpp"

const int buttonPin = 21; // 好きなGPIO

void setup()
{
    Serial.begin(115200);

    pinMode(buttonPin, INPUT_PULLUP);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // WiFiの準備
    // 自分ip,相手ip,パケット処理する関数の登録
    // 今回パケット処理しないのでNOfnを指定しておく
    NetworkMngr.init(remote_ip, lamp_ip);

    Serial.println("device_IN ready");
}

void loop()
{
    // update//////////////////////////////////////////////////////////////////////
    NetworkMngr.update();

    // ボタン監視 10msごと
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