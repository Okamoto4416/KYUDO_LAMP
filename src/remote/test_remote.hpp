#include "NetWorkMngr.hpp"

const auto local_ip = remote_ip;
const auto peer_ip = lamp_ip;

void measurefn(JsonObjectConst json)
{
    Serial.println("measurefnです。");
    const char *type = json["type"].as<const char *>();
    if (std::strcmp(type, "ack_measure") == 0)
    {
        char buf[256];
        auto len = serializeJson(json, buf, sizeof(buf));
        Serial.printf("receive packet:");
        Serial.write(buf, len);
        Serial.println();

        Serial.printf(
            "json usage=%u\n",
            json.memoryUsage());
    }
}

void setup()
{
    Serial.begin(115200);
    NetworkMngr.init(local_ip, peer_ip, nullptr, measurefn);
}
void loop()
{
    NetworkMngr.update();
    {
        static unsigned long nextTime = millis();
        if (millisReached(nextTime))
        {
            nextTime += 500;

            Serial.printf("state:%s\n", NetworkMngr.get_state_str());
            Serial.printf("localTime:%lu[ms]\n", millis());
            Serial.printf("estimate_peerTime:%lu[ms]\n", NetworkMngr.linkMonitor.peerMillis());
            Serial.printf("emaRTT:%u\n", NetworkMngr.linkMonitor.get_EMA_RTTms());
            Serial.printf("lastRTT:%u\n", NetworkMngr.linkMonitor.get_lastRTTms());
            Serial.printf("missRate:%.2f\n", NetworkMngr.linkMonitor.missRate());
            Serial.printf("lossRate:%.2f\n", NetworkMngr.linkMonitor.lossRate());
        }
    }
}