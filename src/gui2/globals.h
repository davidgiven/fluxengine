#pragma once

#include "lib/core/globals.h"
#include <semaphore>

typedef std::function<void(void)> CB;

extern void runOnAppThread(CB cb);
extern void runOnUiThread(CB cb);

template <typename T>
T runOnUiThread(std::function<void(T)> cb)
{
    T result;
    std::binary_semaphore sem;
    runOnUiThread(
        [&]()
        {
            result = cb();
            sem.release();
        });
    sem.acquire();
    return result;
}

extern void guiInit();
extern void guiLoop(CB each);
extern void guiShutdown();
