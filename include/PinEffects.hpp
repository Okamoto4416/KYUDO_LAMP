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
    const unsigned patternPeriodMs;  // 周期default:1s
    uint8_t patternIndex{0}; // パターンの左から何番目のをみるか。0-7をとる
    uint8_t pattern{0};      // 点滅パターンを01で表す
    unsigned long nextChangeTimeMs;  // 次に切り替える時刻[ms]
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