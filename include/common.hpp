#pragma once
#include <Arduino.h>
#include "UintModRing.hpp"

/*
現在時刻(millis())が時刻tを超過しているか安全に判定する.

t<=now だったらtrue
now<t だったらfalse

unsigned long 半分であればmillisがオーバーフローしていても判定できる。
*/
bool millisReached(unsigned long t)
{
    return UIntModRing<unsigned long>::leq(t, millis());
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
        auto r = prev;
        prev = current;
        current = newstate;
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
        return prev;
    }
    T_state current() const
    {
        return current;
    }
    T_state operator()() const
    {
        return current();
    }
};