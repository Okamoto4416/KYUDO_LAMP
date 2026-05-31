#include "NetWorkMngr.hpp"

NetworkMngr_t NetworkMngr;

LinkMonitor::LinkMonitor(NetworkMngr_t &NW) : NW(NW) {};

/////////////////////////////////////////////////////////////////
// 受信
/////////////////////////////////////////////////////////////////

bool LinkMonitor::receive_packet(JsonObjectConst rx_json)
{
    auto type = rx_json["type"].as<const char *>();
    if (type == NULL)
        return false;
    else if (std::strcmp(type, "measure") == 0)
    {
        // typeがmeasureだったら
        receive_measure_packet(rx_json);
        return true;
    }
    else if (std::strcmp(type, "ack_measure") == 0)
    {
        // typeがack_measureだったら
        receive_ack_measure_packet(rx_json);
        return true;
    }
    else
        return false;
}

// measureパケットが来たら呼び出される
// ack_measureパケットを返す
void LinkMonitor::receive_measure_packet(JsonObjectConst rx_json)
{
    auto measureSeqId = rx_json["measureSeqId"].as<uint16_t>();
    auto t0 = rx_json["t0"].as<unsigned long>();
    auto tx_json = NW.beginTxJson();
    // rx_jsonをtype:ack_measure,t1:millis()にして送り返す。
    tx_json["type"] = "ack_measure";
    tx_json["measureSeqId"] = rx_json["measureSeqId"];
    tx_json["t0"] = rx_json["t0"];
    tx_json["t1"] = millis();

    NW.sendTxJson();
}

void LinkMonitor::receive_ack_measure_packet(JsonObjectConst rx_json)
{
    // Serial.println("receive_ack_measure_packetに入りました。");
    using u16MR = UIntModRing<uint16_t>;
    using ulMR = UIntModRing<unsigned long>;

    // RTT取得
    // ackパケット登録
    // Peer時刻推定
    // peer packet id 有効範囲決定

    const auto t0 = rx_json["t0"].as<unsigned long>();
    const auto t1 = rx_json["t1"].as<unsigned long>();
    const auto t2 = millis();
    const auto seqid = rx_json["measureSeqId"].as<uint16_t>();

    const auto RTTms = t2 - t0;

    // Serial.printf("RTTms:%lu\n",RTTms);
    // Serial.printf("現在のseqid:%u\n",measureSeqId);

    // パケットがおかしくないか判定。おかしかったらパケット無視してreturn
    {
        // RTTがでかい場合
        if (RTTms > 10 * 1000){
            Serial.println("(RTTms > 10 * 1000)");
            return;}
        // seqidが50遅い場合,
        if (!u16MR::leq(measureSeqId - 50, seqid)){
            Serial.println("(!u16MR::leq(measureSeqId - 50, seqid))");
            return;}
        // seqidが未来の場合,
        if (!u16MR::leq(seqid, measureSeqId)){
            Serial.println("(!u16MR::leq(seqid, measureSeqId))");
            return;}
        // seqidとt0の整合が取れない場合(seqidから予測したRTTから100msはずれる場合)
        unsigned long predictionRTTms = periodicTimeMs * (measureSeqId - seqid);
        if (!(ulMR::leq(predictionRTTms - 100, RTTms) &&
              ulMR::leq(RTTms, predictionRTTms + 100))){
            Serial.println("seqidとt0の整合が取れない場合");
            return;
              }
    }
    // Serial.println("パケットはおかしくないようです。");

    // ackパケット登録(arrivalHistoryに登録)
    {
        uint16_t idx = measureSeqId - seqid; // 最新の送信済みパケットのidxが0なので
        // 範囲外なら終了
        if (idx >= ackHistory.size())
            return;
        // すでに到着していたら終了
        if (ackHistory[idx])
            return;
        ackHistory.set(idx);
        // Serial.print("ackHistory:");
        // auto str =ackHistory.to_string();
        // Serial.println(str.c_str());
    }

    // RTT
    {
        lastRTTms = RTTms;
        emaRTTms = emaRTT_alpha * RTTms + (1 - emaRTT_alpha) * emaRTTms;
    }

    // 時刻同期peerMillis
    {
        unsigned long samplePeerOffsetMs = t1 - ulMR::interpolate(t0, t2, 1, 1); // 今回のパケットからの推定オフセット

        if (!hasPeerMillisOffset)
        {
            // 初回はそのまま採用
            this->peerTimeOffsetMs = samplePeerOffsetMs;
            hasPeerMillisOffset = true;
        }
        else
        {
            // 二回目以降はEMAで改善していく
            double rtt_quality = 1.0 - std::min((double)RTTms / 3200.0, 1.0); // RTTが小さいものほど参考にしたい
            double alpha = minLearningRate + learningRate;

            auto ema_PeerOffsetMs = ulMR::interpolate(
                samplePeerOffsetMs,
                this->peerTimeOffsetMs,
                alpha + (rtt_quality * 0.04),
                1 - alpha);

            this->peerTimeOffsetMs = ema_PeerOffsetMs;
            learningRate *= coolingFactor;
        }
    }

    this->isreseted = false; // リセットしていないことにする。
}

void LinkMonitor::update()
{
    // 100ms周期でmeasureパケット送信
    if (millisReached(nextTimeMs))
    {
        step_bitset();
        send_measure_packet();

        nextTimeMs += periodicTimeMs;
    }

    if (!isDiscovered() && !isreseted)
    {
        // 通信可能でない場合
        // wifi切れてもすぐにはリセットする必要はないでしょう。のでisDiscoveredで十分だと考える
        // 通信可能でのみ有効な統計をリセット
        this->RTT_reset();
        this->peerMillis_reset();
        isreseted = true;
    }
}

// measureパケットを送る
bool LinkMonitor::send_measure_packet()
{
    ackHistory[0] = 0;

    auto tx_json = NW.beginTxJson();
    tx_json["type"] = "measure";
    tx_json["measureSeqId"] = ++measureSeqId;
    tx_json["t0"] = millis();
    auto r = NW.sendTxJson();
    return r == NetworkMngr_t::Udp_SendResult::Success;
}

void LinkMonitor::step_bitset()
{
    notMissHistory <<= 1;
    notMissHistory[0] = ackHistory[missTimeoutSteps - 1];

    notLossHistory <<= 1;
    notLossHistory[0] = ackHistory[lossTimeoutSteps - 1];

    ackHistory <<= 1;
}

////////////////////////////////////////////////////////
// 状態
////////////////////////////////////////////////////////

bool LinkMonitor::isConnected() const
{
    return (bitsetCountWindow(notMissHistory, 10) >= 8) // 1秒のmiss率20%以下
           ||
           (bitsetCountWindow(ackHistory, 5) >= 4) // 直近5パケットのうち4パケットack到着済み
        ;
}

bool LinkMonitor::isDiscovered() const
{
    // notLossHistory1.6秒分と
    // ackHistory3.2秒分
    return notLossHistory.count() + ackHistory.count() > 0;
}

float LinkMonitor::lossRate(uint8_t windowSize) const
{
    if (windowSize == 0 || windowSize > 16)
    {
        windowSize = 16;
    }
    return 1 - (float)bitsetCountWindow(notLossHistory, windowSize) / windowSize;
}

float LinkMonitor::missRate(uint8_t windowSize) const
{
    if (windowSize == 0 || windowSize > 16)
    {
        windowSize = 16;
    }
    return 1 - (float)bitsetCountWindow(notMissHistory, windowSize) / windowSize;
}

void NetworkMngr_t::init(
    IPAddress local_ip,
    IPAddress peer_ip,
    UdpReceiveCallback_t fn,
    UdpReceiveCallback_t measurePacketCallback)
{
    this->local_ip = local_ip;
    this->peer_ip = peer_ip;
    this->udpReceiveCallback = fn;
    this->measurePacketCallback = measurePacketCallback;
    wifi_init();
}

void NetworkMngr_t::update()
{
    wifi_update();
    linkMonitor.update();
    udp_update();
}

/////////////////////////////////////////////////////////////
// WiFi接続状態関係
/////////////////////////////////////////////////////////////

void NetworkMngr_t::wifi_init()
{
    WiFi.config(local_ip, gateway, subnet);
    WiFi.begin(ssid, pass);
    state.set(State_t::connectingAP);
}

void NetworkMngr_t::wifi_update()
{
    switch (state())
    {
    case State_t::off:
    {
        state.set();
    }
    break;

    case State_t::connectingAP:
    {
        /*
        APへの接続を開始し、
        接続を待つ
        */

        // 初めてconnectingAPになったか、ここで5秒間失敗していたら再接続開始
        if ((state.prev() != state.current()) || millisReached(nextTryTimeMs))
        {
            WiFi.disconnect();
            WiFi.reconnect();
            nextTryTimeMs = millis() + 5000; // 5秒後にリトライ
        }

        // APにつながったら、udp待ち開始して、connectingDeviceに遷移
        if (WiFi.status() == WL_CONNECTED)
        {
            udp.begin(udpPort);
            state.set(State_t::discoveringPeer);
        }
        else
        {
            state.set();
        }
    }
    break;

    case State_t::discoveringPeer:
    {
        if (linkMonitor.isDiscovered())
        {
            // peerを見つけたら遷移
            state.set(State_t::unstable);
        }
        else if (WiFi.status() != WL_CONNECTED)
        {
            // wifiきれたらconnectingAP
            state.set(State_t::connectingAP);
        }
        else
        {
            state.set();
        }
    }
    break;

    case State_t::unstable:
    {
        if (!linkMonitor.isDiscovered())
        {
            // peerを見失ったら遷移
            state.set(State_t::discoveringPeer);
        }
        else if (linkMonitor.isConnected())
        {
            // connected条件を満たしたらconnected
            state.set(State_t::connected);
        }
        else if (WiFi.status() != WL_CONNECTED)
        {
            // wifiきれたらconnectingAP
            state.set(State_t::connectingAP);
        }
        else
        {
            state.set();
        }
    }
    break;

    case State_t::connected:
    {
        /*
        通信可能状態
        */
        if (!linkMonitor.isConnected())
        {
            // connected条件を不満したら unstable
            state.set(State_t::unstable);
        }
        else if (WiFi.status() != WL_CONNECTED)
        {
            // wifiきれたらconnectingAP
            state.set(State_t::connectingAP);
        }
        else
        {
            state.set();
        }
    }
    break;

    case State_t::error:
    {
    }
    break;
    }
}

/////////////////////////////////////////////////////////////
// udp送受信関係
/////////////////////////////////////////////////////////////

/*
txjsonDocWorkの内容を送る
送れたらUdp_SendResult::Success
*/
NetworkMngr_t::Udp_SendResult NetworkMngr_t::udp_send()
{
    // APに接続されていなければ終了
    if (WiFi.status() != WL_CONNECTED)
        return Udp_SendResult::Err_NotConnectedAP;
    // packet_idを付与
    tx_jsonDocWork["txPacketId"] = this->txPacketId++;

    // JSONを文字列化
    size_t capacity = measureJson(tx_jsonDocWork); // jsonのバッファサイズを計算
    if (capacity + 1 > sizeof(tx_packetBuf))
    {
        // 情報が大きすぎる
        return Udp_SendResult::Err_PacketTooLarge;
    }
    size_t len = serializeJson(tx_jsonDocWork, tx_packetBuf); // シリアライズ

    // パケット作って送信
    int r;
    r = udp.beginPacket(peer_ip, udpPort);
    if (!r)
        return Udp_SendResult::Err_BeginPacketFailed;
    udp.write((const uint8_t *)tx_packetBuf, len);
    r = udp.endPacket();
    if (!r)
        return Udp_SendResult::Err_EndPacketFailed;
    return Udp_SendResult::Success;
}

/*
UDP受信して、JSONにパースして、処理する
1:成功
0:パケットがなかった
-1:パケットあったけどパースに失敗した。
*/
int NetworkMngr_t::udp_receive()
{
    // APに接続されていなければ終了
    if (WiFi.status() != WL_CONNECTED)
        return 0;

    // UDP受信があるか確認する
    int packetSize = udp.parsePacket();
    if (!packetSize)
    {
        // パケットなかった
        return 0;
    }

    // ぱけっと読み取り
    int len = udp.read(rx_packetBuf, sizeof(rx_packetBuf));

    // jsonにパース
    auto err = deserializeJson(rx_jsonDocWork, rx_packetBuf, len);
    if (err != DeserializationError::Ok)
    {
        // パースに失敗
        return -1;
    }

    // 処理
    // 計測パケットだったらこちらで処理
    auto r = this->linkMonitor.receive_packet(rx_jsonDocWork.as<JsonObjectConst>());
    if (r)
    {
        // 計測パケットだったらmeasurePacketCallbackに送る
        this->measurePacketCallback(rx_jsonDocWork.as<JsonObjectConst>());
    }
    else
        // 計測パケットじゃなかったら委託
        this->udpReceiveCallback(rx_jsonDocWork.as<JsonObjectConst>());

    // jsonクリア
    rx_jsonDocWork.clear();
    return 1;
}

// 受信をさばいていく
void NetworkMngr_t::udp_update()
{
    constexpr auto t = 1;
    const auto deadline = millis() + t;
    int r;
    do
    {
        r = udp_receive();
    } while (!millisReached(deadline) && r != 0);
}