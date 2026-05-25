#include <Arduino.h>
#include "OverflowSafeComparator.hpp"

/*
パターンでPINのHIGH,LOWを変える。
パターンが01010100だったら
100msごと
L->H->L->H->L->H->L->L->
と替える
*/
class PatternBlinking8bit
{
    constexpr static unsigned T = 100; // 切り替わるのに100msかける。周期800ms
    unsigned char idx{0};              // パターンの左から何番目のをみるか。0-7をとる
    unsigned char pattern{0};          // 点滅パターンを01で表す
    unsigned long nextChangeTime;      // 次に切り替える時刻[ms]
    const uint8_t pin;

public:
    PatternBlinking8bit(uint8_t pin) : nextChangeTime(millis()), pin(pin) {}

    // パターンを設定
    void setPattern(unsigned char pattern)
    {
        this->pattern = pattern;
    }

    // 周期をリセット
    void resetTerm()
    {
        this->idx = 0;
        nextChangeTime = millis();
    }

    // loopでいっぱい実行すべきもの
    void update()
    {
        const OverflowSafeComparator<unsigned long> comp(1000000);
        auto now = millis();
        if (comp.leq(now, this->nextChangeTime))
        {
            //現在時刻が、変更時刻より後だったら一つ進める
            if ((this->pattern << idx) & 0b10000000)
            {
                // patternの左からidx番目が1ならば点灯
                digitalWrite(this->pin, HIGH);
            }
            else
            {
                // 0ならば消灯
                digitalWrite(this->pin, LOW);
            }
            this->nextChangeTime += T;
            this->idx++;
            this->idx %= 8;
        }
    }

};