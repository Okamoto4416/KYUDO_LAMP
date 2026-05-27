#include "NetWorkMngr.hpp"

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

void LinkMonitor::receive_measure_packet(JsonObjectConst rx_json)
{
    auto tx_json = NW.beginTxJson();
    // rx_jsonをtype:ack_measure,t1:millis()にして送り返す。
    tx_json.set(rx_json);
    tx_json["type"] = "ack_measure";
    tx_json["t1"] = millis();

    NW.sendTxJson();
}

void LinkMonitor::receive_ack_measure_packet(JsonObjectConst rx_json)
{
    const static OverflowSafeComparator<uint16_t> comp16(1000);          // 安全比較器
    const static OverflowSafeComparator<unsigned long> compLong(100000); // 安全比較器
    // RTT取得
    // ackパケット登録
    // Peer時刻推定
    // peer packet id 有効範囲決定

    const auto t0 = rx_json["t0"].as<unsigned long>();
    const auto t1 = rx_json["t1"].as<unsigned long>();
    const auto t2 = millis();
    const auto seqid = rx_json["measureSeqId"].as<uint16_t>();

    const auto RTTms = t2 - t0;

    // パケットがおかしくないか判定。おかしかったらパケット無視してreturn
    {
        // RTTがでかい場合
        if (RTTms > 10 * 1000)
            return;
        // seqidが50遅い場合,
        if (!comp16.leq(measureSeqId - 50, seqid))
            return;
        // seqidが未来の場合,
        if (!comp16.leq(seqid, measureSeqId))
            return;
        // seqidとt0の整合が取れない場合(seqidから予測したRTTから50msはずれる場合)
        unsigned long predictionRTTms = periodicTimeMs * (measureSeqId - seqid);
        if (!(compLong.leq(predictionRTTms - 50, RTTms) &&
              compLong.leq(RTTms, predictionRTTms + 50)))
            return;
    }

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
    }

    // RTT
    lastRTTms = RTTms;
    EMA_RTTms = EMA_RTT_alpha * RTTms + (1 - EMA_RTT_alpha) * EMA_RTTms;
}

void LinkMonitor::update()
{
    if (millisReached(nextTimeMs))
    {
        step_bitset();
        send_measure_packet();

        nextTimeMs += periodicTimeMs;
    }
}

bool LinkMonitor::send_measure_packet()
{
    ackHistory[0] = 0;

    auto tx_json = NW.beginTxJson();
    tx_json["type"] = "measure";
    tx_json["measureSeqId"] = ++measureSeqId;
    tx_json["t0"] = millis();
    return NW.sendTxJson() == NetworkMngr_t::Udp_SendResult::Success;
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

float LinkMonitor::lossRate(uint8_t windowSize = 16) const
{
    if (windowSize == 0 || windowSize > 16)
    {
        windowSize = 16;
    }
    return 1 - (float)bitsetCountWindow(notLossHistory, windowSize) / windowSize;
}

float LinkMonitor::missRate(uint8_t windowSize = 16) const
{
    if (windowSize == 0 || windowSize > 16)
    {
        windowSize = 16;
    }
    return 1 - (float)bitsetCountWindow(notMissHistory, windowSize) / windowSize;
}

void NetworkMngr_t::init(IPAddress local_ip, IPAddress peer_ip, UdpReceiveCallback_t fn)
{
    this->local_ip = local_ip;
    this->peer_ip = peer_ip;
    this->udpReceiveCallback = fn;
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
    if (capacity + 1 > sizeof(packetBuf))
    {
        // 情報が大きすぎる
        return Udp_SendResult::Err_PacketTooLarge;
    }
    size_t len = serializeJson(tx_jsonDocWork, packetBuf); // シリアライズ

    // パケット作って送信
    int r;
    r = udp.beginPacket(peer_ip, udpPort);
    if (!r)
        return Udp_SendResult::Err_BeginPacketFailed;
    udp.write((const uint8_t *)packetBuf, len);
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
    // UDP受信があるか確認する
    int packetSize = udp.parsePacket();
    if (!packetSize)
    {
        // パケットなかった
        return 0;
    }

    // ぱけっと読み取り
    int len = udp.read(packetBuf, sizeof(packetBuf));

    // jsonにパース
    auto err = deserializeJson(rx_jsonDocWork, packetBuf, len);
    if (err != DeserializationError::Ok)
    {
        // パースに失敗
        return -1;
    }

    // 処理
    // 計測パケットだったらこちらで処理
    auto r = this->linkMonitor.receive_packet(rx_jsonDocWork.as<JsonObjectConst>());
    // 計測パケットじゃなかったら委託
    if (!r)
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