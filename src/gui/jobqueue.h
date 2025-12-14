#pragma once
#include <deque>

class JobQueue
{
public:
    void QueueJob(std::function<void(void)> f);
    bool IsQueueEmpty() const;
    bool IsQueueRunning();

public:
    virtual void OnQueueFailed()
    {
        OnQueueEmpty();
    }
    virtual void OnQueueEmpty() {}

private:
    std::deque<std::function<void()>> _jobQueue;
};
