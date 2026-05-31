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
        if (forwardDistance(a,b) == forwardDistance(b,a))
            return a <= b;
        else if (forwardDistance(a,b) < forwardDistance(b,a))
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
        return a + static_cast<UINT>((b - a) * ratio);
    }

    // a,bをm:nに内分する点を返す(m,nは非負でm+nが0でないことを期待する)
    static UINT interpolate(UINT a, UINT b, double m, double n)
    {
        return interpolate(a, b, n / (m + n));
    }

    // 円周上の近いほうの距離
    static UINT distance(UINT a, UINT b)
    {
        if (forwardDistance(a,b)< forwardDistance(b,a))
            return forwardDistance(a,b);
        else
            return forwardDistance(b,a);
    }

    // aからbへ正の方向への距離
    static UINT forwardDistance(UINT a, UINT b)
    {
        return b - a;
    }

    // ちょうど半円周だけ離れているか
    static bool isHalfTurn(UINT a, UINT b)
    {
        return forwardDistance(a,b) == forwardDistance(b,a) && a != b;
    }
};
