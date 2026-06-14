#pragma once
#include <Arduino.h>
#include "UintModRing.hpp"


/**
 * 現在時刻(millis())が時刻tを超過しているか安全に判定する.
 * millisをcastして評価する
 *
 * t<=now だったらtrue
 * now<t だったらfalse
 *
 * UINT 半分であればmillisがオーバーフローしていても判定できる。
 */
template <typename UINT>
inline bool millisReached(UINT t)
{
    return UIntModRing<UINT>::leq(t, millis());
}

/**
 * 前の状態と現在の状態を保持する
 */
template <typename T_state>
struct StateMngr
{
private:
    T_state _prev{};
    T_state _current{};

public:
    /**
     * 状態をセットする
     */
    T_state set(T_state newstate)
    {
        T_state r = _prev;
        _prev = _current;
        _current = newstate;
        return r;
    }

    /**
     * 同じ状態にセットする
     */
    T_state set()
    {
        return set(_current);
    }
    T_state prev() const
    {
        return this->_prev;
    }
    T_state current() const
    {
        return this->_current;
    }
    T_state operator()() const
    {
        return current();
    }
};