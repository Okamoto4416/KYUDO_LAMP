#pragma once
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
template <typename Counter>
class OverflowSafeComparator
{
public:
    const Counter delta;
    OverflowSafeComparator(const Counter &delta) : delta(delta) {

    };
    /**
     * @brief t1 <= t2 かどうかを判定する関数
     * @param t1 First value
     * @param t2 Second value
     * @return true if t1 <= t2, false otherwise
     */
    bool leq(const Counter &t1, const Counter &t2) const
    {
        Counter t2delta = (Counter)(t2 - delta);

        if (t2delta < t2)
        {
            // オーバーフローが発生していないと考えられる場合
            return t1 <= t2;
        }
        else
        {
            // t2 < t2-delta
            // オーバーフローが発生したと考えられる場合
            // つまり、t2-delta <= 0<=t2とか t2-delta <= 127<-127<t2とかのとき
            if (t2delta <= t1)
            {
                return true;
            }
            else
            {
                return t1 <= t2;
            }
        }
    };

    /**
     * @brief t1 < t2 かどうかを判定する関数
     * @param t1 First value
     * @param t2 Second value
     * @return true if t1 < t2, false otherwise
     */
    bool lt(const Counter &t1, const Counter &t2) const
    {
        return leq(t1, t2) && !(t1 == t2);
    };
};