#pragma once
#include <Arduino.h>
#include "common.hpp"

/*
パターンでPINのHIGH,LOWを変える。
パターンが01010100だったら
100msごと
L->H->L->H->L->H->L->L->
と替える
*/
class PatternBlinker8bit
{
    const unsigned patternPeriodMs; // 周期default:1s
    uint8_t patternIndex{0};        // パターンの左から何番目のをみるか。0-7をとる
    uint8_t pattern{0};             // 点滅パターンを01で表す
    unsigned long nextChangeTimeMs; // 次に切り替える時刻[ms]
    const uint8_t pin;

public:
    PatternBlinker8bit(uint8_t pin, unsigned patternPeriodMs = 1000)
        : nextChangeTimeMs(millis()),
          pin(pin),
          patternPeriodMs(patternPeriodMs) {}

    // パターンを設定
    void setPattern(uint8_t pattern)
    {
        this->pattern = pattern;
    }

    // 周期をリセット
    void restart()
    {
        this->patternIndex = 0;
        nextChangeTimeMs = millis();
    }

    // loopでいっぱい実行すべきもの
    void update()
    {
        if (millisReached(this->nextChangeTimeMs))
        {
            // 現在時刻が、変更時刻より後だったら一つ進める
            if (this->pattern & (0b10000000 >> patternIndex))
            {
                // patternの左からidx番目が1ならば点灯
                digitalWrite(this->pin, HIGH);
            }
            else
            {
                // 0ならば消灯
                digitalWrite(this->pin, LOW);
            }
            this->nextChangeTimeMs += patternPeriodMs / 8;
            this->patternIndex++;
            this->patternIndex %= 8;
        }
    }
};

/**
 * パルスを出力する
 */
class PulseOutput
{
    const unsigned pulseLengthMs; // パルスの長さ
    unsigned long finTimeMs;      // 次に切り替える時刻[ms]
    const uint8_t pin;
    uint8_t idleLevel = LOW;
    uint8_t pulseLevel = HIGH;
    bool isPulse = false; // パルス出力中か

public:
    /**
     * 出力ピンの設定とデフォルトのパルスの長さを設定する
     * pinは pinMode(pin,OUTOUT)しておく必要がある
     * */
    PulseOutput(uint8_t pin, unsigned pulseLengthMs = 10)
        : pin(pin),
          pulseLengthMs(pulseLengthMs) {}

    // パルスを出力
    void trigger(unsigned ms = 0)
    {
        if (ms == 0)
        {
            ms = pulseLengthMs;
        }
        digitalWrite(pin, pulseLevel);
        isPulse = true;
        finTimeMs = millis() + ms;
    }
    void update()
    {
        // パルス中で且つ終了時刻を超えていたらパルス終了
        if (isPulse && millisReached(finTimeMs))
        {
            digitalWrite(pin, idleLevel);
            isPulse = false;
        }
    }

    // デフォルトの出力をHIGHにするかLOWにするか
    void setDefaultVal(uint8_t HIGH_or_LOW)
    {
        if (HIGH_or_LOW)
        {
            idleLevel = HIGH;
            pulseLevel = LOW;
        }
        else
        {
            idleLevel = LOW;
            pulseLevel = HIGH;
        }
    }
};

/**
 * チャタリング対策したデジタル読み取りを提供する
 * ゲージ式
 *
 * gageCapacity : ゲージの容量[μs]
 * thLow        : LOWに切り替わる閾値
 * thHigh       : HIGHに切り替わる閾値
 *
 * 仕組み：
 * 読み取り時
 * HIGHの時ゲージをためる
 * LOWの時ゲージを減らす
 *
 * ゲージがthHighを超えたときHIGHと判定する
 * ゲージがthLowを下回った時LOWと判定する
 *
 * 使い方：
 * update関数をloopで回しまくる。
 * 読み取りたいときはread()する
 */
template <unsigned long gageCapacity = 10 * 1000,
          unsigned long thLow = gageCapacity / 3,
          unsigned long thHigh = gageCapacity / 3 * 2>
class GageDigitalRead
{
    const uint8_t pin;
    bool isHigh = false;
    unsigned long gage = 0; // ゲージ 読み取り結果がHIGHだったら溜まり、LOWだったら減っていく
    unsigned long preMicros;

public:
    Button(uint8_t pin, bool isHigh = false, unsigned gage = 0)
        : pin(pin), isHigh(isHigh), gage(gage), preMicros(micros()) {}

    // 呼び出さなくてよい
    void init()
    {
        preMicros = micros();
    }
    void update()
    {
        auto now = micros();
        auto deltaGage = micros() - preMicros;
        if (HIGH == digitalRead(pin))
        {
            // HIGHだったら経過マイクロ秒だけゲージを増やす
            gage += std::min(deltaGage, gageCapacity - gage);
        }
        else
        {
            // LOWだったら経過マイクロ秒だけゲージを減らす
            gage -= std::min(deltaGage, gage);
        }

        // 以下はupdateで必要(read時ではいけない)
        if (isHigh && gage < thLow)
        {
            isHigh = false;
        }
        else if (!isHigh && thHigh < gage)
        {
            isHigh = true;
        }
        preMicros = now;
    }

    // 判定結果読み取り
    int read() const
    {
        if (isHigh)
            return HIGH;
        else
            return LOW;
    }
};

/**
 * チャタリング対策したデジタル読み取りを提供する
 * 履歴式(軽量)
 *
 * T            : 読み取り履歴を保存するのに使用する符号なし整数型
 * interval_    : 読み取り周期[ms]
 * n_           : 読み取り履歴の最後いくつを判定にするか
 *
 * 仕組み：
 * interval [ms]ごとに読み取り、履歴に入れていく
 * 下n桁が全部1 → HIGH
 * 下n桁が全部0 → LOW
 * それ以外 → 前回状態維持
 *
 * 使い方：
 * update関数をloopで回しまくる。
 * 読み取りたいときはread()する
 */
template <typename UINT = uint8_t, unsigned interval = 5, unsigned n = 4>
class DebouncedDigitalRead
{
    static constexpr unsigned bitWidth = sizeof(UINT) * 8;
    static constexpr UINT mask = ~(~UINT{0} << n); // 下n桁が1
    static_assert(std::is_integral<UINT>::value &&
                      std::is_unsigned<UINT>::value,
                  "UINTは符号なし整数の必要がある。");
    static_assert(interval > 0, "intervalは正の数である必要がある。");
    static_assert(0 < n && n <= bitWidth, "n は 0より大きく、UINTのbitサイズ以下である必要がある。");

    unsigned long nextMillis; // 次の判定時刻
    UINT history{0};          // 読み取り結果履歴HIGHが読み取られたら1が立ちシフトしていく
    const uint8_t pin;        // 読み取る対象のpin
    bool state = false;

public:
    DebouncedDigitalRead(uint8_t pin, bool state = false)
        : pin(pin), state(state) nextMillis(millis()) {}

    void update()
    {

        if (millisReached(nextMillis))
        {
            // 次の時間のセット
            nextMillis += interval;

            // 履歴に入れる
            history <<= 1;
            if (HIGH == digitalRead(pin))
                history |= UINT{1};

            // 判定
            if ((history & mask) == mask)
            {
                state = true;
            }
            else if ((history & mask) == 0)
            {
                state = false;
            }
        }
    }

    // 判定結果読み取り
    int read() const
    {
        if (state)
            return HIGH;
        else
            return LOW;
    }
};