#include "globals.h"
#include "fmt/format.h"
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include "ui.h"

static sem_t* semaphore;
static std::function<void(void)> appCallback;
static std::function<void(void)> uiCallback;
static std::function<void(void)> exitCallback;
static pthread_t appThread = 0;

void UIInitThreading()
{
	std::string name = fmt::format("/com.cowlark.fluxengine.{}", getpid());
	semaphore = sem_open(name.c_str(), O_CREAT|O_EXCL, 0, 0);
	if (!semaphore)
		Error() << fmt::format("cannot create semaphore: {}", strerror(errno));
	sem_unlink(name.c_str());
}

/* Run on the UI thread. */
static void queue_cb(void*)
{
    uiCallback();
    sem_post(semaphore);
}

void UIRunOnUIThread(std::function<void(void)> callback)
{
    uiCallback = callback;
    uiQueueMain(queue_cb, nullptr);
    sem_wait(semaphore);
}

static void* appthread_cb(void*)
{
    appCallback();
    appThread = 0;
	UIRunOnUIThread(exitCallback);
    return nullptr;
}

void UIStartAppThread(std::function<void(void)> runCallback, std::function<void(void)> exitCallback)
{
    if (appThread)
        Error() << "application thread already running";
    ::appCallback = runCallback;
	::exitCallback = exitCallback;
    int res = pthread_create(&appThread, nullptr, appthread_cb, nullptr);
    if (res)
        Error() << fmt::format("could not start application thread: {}", strerror(errno));
    pthread_detach(appThread);
}
