/**
 * 通信部分を担当
 *
 * 使い方:
 * setup()
 * {
 * ...
 * NetworkMngr.init(local_ip,peer_ip,fn)
 * ...
 * }
 * loop()
 * {
 * ...
 * NetworkMngr.update()
 * ...
 * }
 *
 * udpパケットを送る場合：
 * auto json = NetworkMngr.beginTxJson();
 * jsonに書き込む
 * NetworkMngr.sendTxJson();
 *
 */

#pragma once

#include <bitset>
#include <cstring>

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#include "routerAP_config.hpp"
#include "common.hpp"

using txPacketId_t = uint16_t;

class NetworkMngr_t;
/**
 * 通信の計測を行うクラス
 * measureパケットを送り、ack_measureを返す
 * 通信良好・不安定・可能を判断
 * Peer時刻推定
 * RTT計測
 * miss率計測(miss:500ms以内にackが返ってこない)
 * loss率計測
 * Peerの有効txpacket id範囲を決定
 */
class LinkMonitor
{
private:
    NetworkMngr_t &NW;

    bool isreseted = false; // 統計情報をリセットしたか

    // RTT
    double emaRTTms = 1600;                                // RTTの平均[ms]
    constexpr static double emaRTT_alpha = 2.0 / (32 + 1); // 32個分の平均のための係数
    uint16_t lastRTTms = 3200;                             // 最新のパケットのRTT[ms]

    // 時刻同期
    unsigned long peerTimeOffsetMs = 0;                       // peerのmillisとのずれ peerMillis=millis()+peerOffsetMs
    constexpr static double coolingFactor = 0.4;              // 冷却係数(5回で0.01になる)
    constexpr static double minLearningRate = 2.0 / (32 + 1); // サンプル推定値の最低割合
    double learningRate = 1 - minLearningRate;                // 熱(のちフィールド)//だんだん冷めていく
    bool hasPeerMillisOffset = false;                         // すでにOffsetは推定されているか(初回と二回目以降の分岐のため)

    // // Peerの有効txpacket id範囲について(受け取る価値のありそうなtxpacketIdの範囲)
    // // あまりに逸脱していたら変なパケットである。
    // txPacketId_t peer_last_txPacketId = 0;              // 直近に来たpeerのpacketId
    // constexpr static auto peer_txPacketId_margin = 100; // 100よりずれたパケットIdは変だと思う。

    // パケットmiss,パケットloss
    constexpr static auto missTimeoutSteps = 5;  // パケットミスの定義：送ってから500ms以内にackがなければmiss
    constexpr static auto lossTimeoutSteps = 32; // パケットロスの定義：送ってから3200ms以内にackがなければloss
    std::bitset<32> ackHistory{};                // 最近32個の送信measureパケットのack到着を格納する。
    std::bitset<16> notMissHistory{};            // 最近16個の送信measureパケットのmissを格納する。
    std::bitset<16> notLossHistory{};            // 最近16個の送信measureパケットのlossを格納する。

    uint16_t measureSeqId = 0; // 計測パケットの番号(最新の送信済みパケットのもの)

public:
    LinkMonitor(NetworkMngr_t &NW);

    /////////////////////////////////////////////////////////////////
    // 受信
    /////////////////////////////////////////////////////////////////
    /**
     * packetを受け取ったら
     * typeがmeasure or ack_measureだったら処理する->true
     * そうじゃなかったら->false
     */
    bool receive_packet(JsonObjectConst rx_json);
    /**
     * measureパケット受信
     * peerから来たmeasureパケットに対してackを返す
     */
private:
    void receive_measure_packet(JsonObjectConst rx_json);
    /**
     * ack_measureパケット受信
     * 解析する
     */
    void receive_ack_measure_packet(JsonObjectConst rx_json);

    /////////////////////////////////////////////////////////////////////////
    // 定期実行と送信
    /////////////////////////////////////////////////////////////////////////
private:
    constexpr static auto periodicTimeMs = 100;           // 定期実行の周期100ms
    unsigned long nextTimeMs = millis() + periodicTimeMs; // 次の定期実行時刻

public:
    /**
     * loopでめっちゃ回されるやつ（NWで呼ばれる)
     * 100ms秒周期で定期実行
     */
    void update();

private:
    /**
     * measure_packet送信
     * 返り値：送信できたらtrue。この送信の可否でも設定がちゃんとしているか見たい
     */
    bool send_measure_packet();

    /**
     * bitsetを一つ進める
     * loss率の解析のため
     * miss率の解析のため
     */
    void step_bitset();

    ////////////////////////////////////////////////////////
    // 状態
    ////////////////////////////////////////////////////////
public:
    /**
     * connected条件を満たしているか調べる
     *
     * 1秒のmiss率20%以下
     * または
     * 直近5パケットのうち4パケットack到着済み
     */
    bool isConnected() const;

    /**
     * discovered条件を満たしているか調べる
     *
     * 過去4.8秒間に計測パケットのackが届いている
     */
    bool isDiscovered() const;

    /**
     * 直近のパケットロス率
     *
     * lossであるかlossでないか確定したものを集計
     */
    float lossRate(uint8_t windowSize = 16) const;

    /**
     * 直近のパケットmiss率
     *
     * missであるかmissでないか確定したものを集計
     */
    float missRate(uint8_t windowSize = 16) const;

    ////////////////////////////////////////////////////////////
    // connectedまたはunstableの時有効な統計量
    ////////////////////////////////////////////////////////////

    /**
     * RTTのリセット
     *
     */
    void RTT_reset()
    {
        emaRTTms = 1600;  // RTTの平均[ms]
        lastRTTms = 3200; // 最新のパケットのRTT[ms]
    }
    /**
     * 平均RTT
     * connectedまたはunstableの時有効
     */
    uint16_t get_EMA_RTTms() const
    {
        return this->emaRTTms;
    }
    /**
     * 最近のパケットのRTT
     * connectedまたはunstableの時有効
     */
    uint16_t get_lastRTTms() const
    {
        return this->lastRTTms;
    }

    /**
     * 時刻同期のリセット
     */
    void peerMillis_reset()
    {
        peerTimeOffsetMs = 0;
        learningRate = 1 - minLearningRate; // 熱(のちフィールド)//だんだん冷めていく
        hasPeerMillisOffset = false;        // すでにOffsetは推定されているか(初回と二回目以降の分岐のため)
    }

    /**
     * peerのmillisとのずれを取得
     * connectedまたはunstableの時有効
     */
    unsigned long get_peerMillisOffsetMs() const
    {
        return this->peerTimeOffsetMs;
    }
    /**
     * peerの推定millis()
     * connectedまたはunstableの時有効
     */
    unsigned long peerMillis() const
    {
        return millis() + this->peerTimeOffsetMs;
    }

    /**
     * peerの時刻をlocalの時刻に変換
     * connectedまたはunstableの時有効
     */
    unsigned long localMillis_from(unsigned long t_peerMillis) const
    {
        return t_peerMillis - this->peerTimeOffsetMs;
    }

    /**
     * 下位windowSize bitの1をカウントするa
     */
    template <typename bitsetT, typename sizeT = uint8_t>
    static sizeT bitsetCountWindow(const bitsetT &b, sizeT windowSize)
    {
        bitsetT mask{0};
        mask = ~mask;
        mask <<= windowSize;
        mask = ~mask;
        mask &= b;
        return mask.count();
    }
};

class NetworkMngr_t
{
public:
    enum class State_t
    {
        off,             // WiFi off
        connectingAP,    // AP接続待ち
        discoveringPeer, // 他方のデバイスへの発見待ち
        unstable,        // 通信可能だが不安定
        connected,       // 通信良好
        error,           // その他エラー
    };
    static const char *state_str(State_t s)
    {
        switch (s)
        {
        case State_t::off:
            return "off";
        case State_t::connectingAP:
            return "connectingAP";
        case State_t::discoveringPeer:
            return "discoveringPeer";
        case State_t::unstable:
            return "unstable";
        case State_t::connected:
            return "connected";
        case State_t::error:
            return "error";
        default:
            return "";
        }
    }
    using JsonDoc_t = StaticJsonDocument<192>;
    using Buff_t = char[256];
    /**
     * 引数：受け取ったjsonobject
     * (元文字列を渡そうと思ったけど、デシリアライズしている時点で改変されるので無理だった)
     */
    using UdpReceiveCallback_t = void (*)(JsonObjectConst);

private:
    static void NOPfn(JsonObjectConst){};

private:
    StateMngr<State_t> state; // wifiのステータス
    IPAddress local_ip;       // このipアドレス
    IPAddress peer_ip;        // 相手のipアドレス
public:
    LinkMonitor linkMonitor{*this}; // 通信路計測する奴

private:
    // 状態管理のための

    // connectingAP
    unsigned long nextTryTimeMs{};

public:
    /**
     * 初期化
     *
     * local_ipはこの機器のip
     * peer_ipは相手の機器のip
     * UdpReceiveCallback_t fn は udpを受信したときに呼び出される関数(nullptrだと何もしない)
     * measurePacketCallback は 計測パケットが来た時に呼び出される関数(nullptrだと何もしない)
     */
    void init(
        IPAddress local_ip,
        IPAddress peer_ip,
        UdpReceiveCallback_t fn = nullptr,
        UdpReceiveCallback_t measurePacketCallback = nullptr);

    void update(); // 更新

    State_t get_state() const
    {
        return state();
    }

    const char *get_state_str() const
    {
        return state_str(state());
    }

    /////////////////////////////////////////////////////////////
    // WiFi接続状態関係
    /////////////////////////////////////////////////////////////
private:
    void wifi_init();

    void wifi_update();

    /////////////////////////////////////////////////////////////
    // udp送受信関係
    /////////////////////////////////////////////////////////////
private:
    // udp
    WiFiUDP udp;
    txPacketId_t txPacketId = 0;
    UdpReceiveCallback_t udpReceiveCallback; // 受信したudpを処理する関数
    Buff_t tx_packetBuf;                     // packetバッファ。情報を保持させるな。送信するときだけ使え
    Buff_t rx_packetBuf;                     // packetバッファ。情報を保持させるな。受信するときだけ使え

    UdpReceiveCallback_t measurePacketCallback; // 計測パケットが来た時に呼ばれる関数

    JsonDoc_t tx_jsonDocWork{}; // JSON。 情報を保持させるな。パースするときだけに使え。送信用
    JsonDoc_t rx_jsonDocWork{}; // JSON。 情報を保持させるな。パースするときだけに使え。受信用

public:
    /**
     * udp_send()の返り値
     */
    enum class Udp_SendResult
    {
        Success,
        Err_NotConnectedAP,
        Err_PacketTooLarge,
        Err_BeginPacketFailed,
        Err_EndPacketFailed,
    };

    /*
    txjsonDocWorkの内容を送る
    送れたらUdp_SendResult::Success
    */
    Udp_SendResult udp_send();

    /**
     * クリアされたtxjsonDocWorkへの参照を得る。
     * 返された JsonObject は次回 beginTxJson() まで有効
     * 情報入れたらすぐに sendTxJson()しろ。
     *
     * 使用例：
     * auto json = NetworkMngr.beginTxJson()
     * json["type"]="test";
     * json["value"]=1234;
     * NetworkMngr.sendTxJson();
     */
    JsonObject beginTxJson()
    {
        return tx_jsonDocWork.to<JsonObject>(); // クリアして、キャスト
    }

    /*
    udp_send()に同じ
    txjsonDocWorkの内容を送る
    送れたらUdp_SendResult::Success
    */
    Udp_SendResult sendTxJson()
    {
        return udp_send();
    }

private:
    /*
    UDP受信して、JSONにパースして、処理する
    1:成功
    0:パケットがなかった
    -1:パケットあったけどパースに失敗した。
    */
    int udp_receive();

    // 受信をさばいていく
    void udp_update();
};

extern NetworkMngr_t NetworkMngr;