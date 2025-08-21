#include "globals.h"
#include <mutex>
#include <thread>

static std::mutex queueMutex;
static std::counting_semaphore<INT_MAX> appThreadSize(0);
static std::deque<CB> appThreadQueue;
static std::deque<CB> uiThreadQueue;
static bool appThreadShouldExit = false; /* app thread only */

void runOnAppThread(CB cb)
{
    std::lock_guard<std::mutex> guard(queueMutex);
    appThreadQueue.push_back(cb);
    appThreadSize.release();
}

void runOnUiThread(CB cb)
{
    std::lock_guard<std::mutex> guard(queueMutex);
    uiThreadQueue.push_back(cb);
}

static CB atomicallyPopFromQueue(std::deque<CB>& queue)
{
    CB cb;
    std::lock_guard<std::mutex> guard(queueMutex);
    if (!queue.empty())
    {
        cb = queue.front();
        queue.pop_front();
    }
    return cb;
}

static void appThreadMain(void)
{
    while (!appThreadShouldExit)
    {
        appThreadSize.acquire();

        CB cb = atomicallyPopFromQueue(appThreadQueue);
        cb();
    }
}

int main(int argc, const char* argv[])
{
    guiInit();

    std::thread appThread(appThreadMain);

    guiLoop(
        []()
        {
            for (;;)
            {
                CB cb = atomicallyPopFromQueue(uiThreadQueue);
                if (!cb)
                    break;
                cb();
            }
        });

    runOnAppThread(
        []()
        {
            appThreadShouldExit = true;
        });
    appThread.join();

    guiShutdown();

    return 0;
}
