#pragma once

#include <chrono>
#include <stdlib.h>
#include <stdio.h>

class Timer
{
public:
    Timer() : mStartTime(std::chrono::high_resolution_clock::now())
    {
    }

    void reset()
    {
        mStartTime = std::chrono::high_resolution_clock::now();
    }

    double getElapsedSeconds()
    {
        auto s = peekElapsedSeconds();
        reset();
        return s;
    }

    double peekElapsedSeconds()
    {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = now - mStartTime;
        return diff.count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> mStartTime;
};

class ScopedTime
{
public:
    ScopedTime(const char *action) : mAction(action)
    {
        mTimer.reset();
    }
    ~ScopedTime(void)
    {
        double dtime = mTimer.getElapsedSeconds();
        printf("%s took %0.5f seconds\n", mAction, dtime);
    }

    const char *mAction{nullptr};
    Timer       mTimer;
};
