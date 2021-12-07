#include "globals.h"
#include <pthread.h>
#include <semaphore.h>
#include "ui.h"

static sem_t semaphore;
static std::function<void(void)> appCallback;
static std::function<void(void)> uiCallback;
static pthread_t appThread = 0;

void UIInitThreading()
{
    sem_init(&semaphore, 0, 0);
}

/* Run on the UI thread. */
static void queue_cb(void*)
{
    uiCallback();
    sem_post(&semaphore);
}

void UIRunOnUIThread(std::function<void(void)> callback)
{
    uiCallback = callback;
    uiQueueMain(queue_cb, nullptr);
    sem_wait(&semaphore);
}

static void* appthread_cb(void*)
{
    appCallback();
    appThread = 0;
    return nullptr;
}

void UIStartAppThread(std::function<void(void)> callback)
{
    if (appThread)
        Error() << "application thread already running";
    appCallback = callback;
    int res = pthread_create(&appThread, nullptr, appthread_cb, nullptr);
    if (!res)
        Error() << "could not start application thread";
    pthread_detach(appThread);
}
