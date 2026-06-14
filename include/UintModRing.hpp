#pragma once
#include <type_traits>
/*
 * @file OverflowSafeComparator.h
 * @brief 有限カウンタのオーバーフローに対策した比較を扱うクラスの定義
 *
 * このクラスは、オーバーフローを考慮した比較関数を提供します。
 * 例えば、符号なし8ビット整数の場合、255から0へのオーバーフローが発生する可能性があります。
 * このクラスは、与えられた差分deltaを使用して、オーバーフローを超えた場合も、t1 <= t2やt1 < t2の関係を判定するための関数を提供します。
 * delta=100の場合、t2の100前までは少なくともt2よりも小さいとみなします。
 * 例えば、t1=250、t2=18である場合、t1<t2と判定されます。
 *
 * @tparam Counter 最大値を超えたら最小値からスタートするカウンタの型(unsignedの型とか)
 */
// template <typename Counter>
// class [[deprecated("UIntModRingをつかってほしい")]] OverflowSafeComparator
// {
// public:
//     const Counter delta;
//     OverflowSafeComparator(const Counter &delta) : delta(delta) {

//                                                    };
//     /**
//      * @brief t1 <= t2 かどうかを判定する関数
//      * @param t1 First value
//      * @param t2 Second value
//      * @return true if t1 <= t2, false otherwise
//      */
//     bool leq(const Counter &t1, const Counter &t2) const
//     {
//         Counter t2delta = (Counter)(t2 - delta);

//         if (t2delta < t2)
//         {
//             // オーバーフローが発生していないと考えられる場合
//             return t1 <= t2;
//         }
//         else
//         {
//             // t2 < t2-delta
//             // オーバーフローが発生したと考えられる場合
//             // つまり、t2-delta <= 0<=t2とか t2-delta <= 127<-127<t2とかのとき
//             if (t2delta <= t1)
//             {
//                 return true;
//             }
//             else
//             {
//                 return t1 <= t2;
//             }
//         }
//     };

//     /**
//      * @brief t1 < t2 かどうかを判定する関数
//      * @param t1 First value
//      * @param t2 Second value
//      * @return true if t1 < t2, false otherwise
//      */
//     bool lt(const Counter &t1, const Counter &t2) const
//     {
//         return leq(t1, t2) && !(t1 == t2);
//     };
// };

/**
 * Nbit符号なし整数型はZ/(2^N)Zの剰余環を成す。
 * 加法において巡回群であるので、巡回を考慮した
 * - 順序みたいなやつ
 * - 内分
 * - 距離
 *
 * を定義する
 * 巡回しているので円周を用いて考える
 *
 */
template <typename UINT>
struct UIntModRing
{
    static_assert(std::is_integral<UINT>::value &&
                      std::is_unsigned<UINT>::value,
                  "uintTは符号なし整数の必要がある。");

    /**
     * less than equal (<=) 順序じゃないけどね。局所的に全順序を成す
     * - 円周上で b が a から半周未満にあるなら trueを返す
     * - aとbがちょうど半周離れているとき、a<=bを返す
     * - その他 falseを返す
     *
     * a(<=)b :<-> (b-a!=a-b -> b-a<a-b)∧(b-a==a-b -> a<=b)
     */
    static bool leq(const UINT a, const UINT b)
    {
        if (forwardDistance(a, b) == forwardDistance(b, a))
            return a <= b;
        else if (forwardDistance(a, b) < forwardDistance(b, a))
            return true;
        else
            return false;
    }

    /**
     * less than a(<)b
     * leqを参照のこと
     */
    static bool lt(const UINT a, const UINT b)
    {
        return a != b && leq(a, b);
    }

    /**
     * aからbへratioだけ進んだ点(0<=ratio<=1を期待する)
     * 1-ratio:ratioに内分する
     * ratio =0 -> return a
     * ratio =1 -> return b
     */
    static UINT interpolate(UINT a, UINT b, double ratio)
    {
        if (lt(b, a))
        {
            // a(<=)bになるように入れ替え
            UINT t = a;
            a = b;
            b = t;

            ratio = 1 - ratio;
        }
        return add(a, static_cast<UINT>(sub(b, a) * ratio));
    }

    // a,bをm:nに内分する点を返す(m,nは非負でm+nが0でないことを期待する)
    static UINT interpolate(UINT a, UINT b, double m, double n)
    {
        return interpolate(a, b, n / (m + n));
    }

    // 円周上の近いほうの距離
    static UINT distance(UINT a, UINT b)
    {
        if (forwardDistance(a, b) < forwardDistance(b, a))
            return forwardDistance(a, b);
        else
            return forwardDistance(b, a);
    }

    // 引き算
    constexpr static UINT sub(UINT a, UINT b)
    {
        return a - b;
    }

    // 足し算
    constexpr static UINT add(UINT a, UINT b)
    {
        return a + b;
    }

    // aからbへ正の方向への距離
    constexpr static UINT forwardDistance(UINT a, UINT b)
    {
        return sub(b, a);
    }

    // ちょうど半円周だけ離れているか
    constexpr static bool isHalfTurn(UINT a, UINT b)
    {
        return forwardDistance(a, b) == forwardDistance(b, a) && a != b;
    }
};

#include <cassert>
// 正しく実装できているかテストする。
inline bool uintModRingTest()
{
    using u16mod = UIntModRing<uint16_t>;
    using u8mod = UIntModRing<uint8_t>;

    //==================================================
    // leq: 巡回なしの通常ケース
    //==================================================
    assert(u8mod::leq(0, 0) == true);
    assert(u8mod::leq(0, 1) == true);
    assert(u8mod::leq(0, 126) == true);
    assert(u8mod::leq(1, 0) == false);

    //==================================================
    // leq: オーバーフローをまたぐケース
    // 250 -> 18 は 24 進むので 250 <= 18 とみなす
    //==================================================
    assert(u8mod::leq(250, 18) == true);
    assert(u8mod::lt(250, 18) == true);
    assert(u8mod::leq(18, 250) == false);

    //==================================================
    // 半周ケース uint8_t では 128 離れ
    // tie-breakとして通常の a <= b を使う
    //==================================================
    assert(u8mod::isHalfTurn(0, 128) == true);
    assert(u8mod::isHalfTurn(128, 0) == true);
    assert(u8mod::leq(0, 128) == true);
    assert(u8mod::leq(128, 0) == false);

    //==================================================
    // lt
    //==================================================
    assert(u8mod::lt(0, 0) == false);
    assert(u8mod::lt(0, 1) == true);
    assert(u8mod::lt(1, 0) == false);
    assert(u8mod::lt(250, 18) == true);

    //==================================================
    // forwardDistance
    //==================================================
    assert(u8mod::forwardDistance(10, 20) == 10);
    assert(u8mod::forwardDistance(250, 5) == 11);

    //==================================================
    // distance: 近い方の距離
    //==================================================
    assert(u8mod::distance(10, 20) == 10);
    assert(u8mod::distance(20, 10) == 10);
    assert(u8mod::distance(250, 5) == 11);
    assert(u8mod::distance(5, 250) == 11);
    assert(u8mod::distance(0, 128) == 128);

    //==================================================
    // interpolate: 巡回なし
    //==================================================
    assert(u8mod::interpolate(10, 20, 0.0) == 10);
    assert(u8mod::interpolate(10, 20, 1.0) == 20);
    assert(u8mod::interpolate(10, 20, 0.5) == 15);

    //==================================================
    // interpolate: 巡回あり
    // 250 -> 10 は 16 進む
    // 半分進むと 258 mod 256 = 2
    //==================================================
    assert(u8mod::interpolate(250, 10, 0.0) == 250);
    assert(u8mod::interpolate(250, 10, 1.0) == 10);
    assert(u8mod::interpolate(250, 10, 0.5) == 2);

    //==================================================
    // interpolate(m:n)
    // a,bを m:n に内分
    // m=1,n=1 なら中点
    //==================================================
    assert(u8mod::interpolate(10, 20, 1.0, 1.0) == 15);
    assert(u8mod::interpolate(250, 10, 1.0, 1.0) == 2);

    //==================================================
    // uint16_tでも軽く確認
    //==================================================
    assert(u16mod::leq(65000, 100) == true);
    assert(u16mod::lt(65000, 100) == true);
    assert(u16mod::distance(65000, 100) == static_cast<uint16_t>(636));

    return true;
}