#pragma once
#include <Arduino.h>
#include "OverflowSafeComparator.hpp"

/*
現在時刻(millis())が時刻tを超過しているか安全に判定する.

t<=now だったらtrue
now<t だったらfalse

24時間の差であればmillisがオーバーフローしていても判定できる。
*/
bool millisReached(unsigned long t)
{
    static const OverflowSafeComparator<unsigned long> Comp(24 * 3600 * 1000); //
    return Comp.leq(t, millis());
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