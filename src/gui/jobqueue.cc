#include "lib/core/globals.h"
#include "lib/core/utils.h"
#include "gui.h"
#include "jobqueue.h"

void JobQueue::QueueJob(std::function<void(void)> f)
{
    _jobQueue.push_back(f);
    if (!IsQueueRunning())
    {
        runOnWorkerThread(
            [this]()
            {
                auto fail = [&]()
                {
                    runOnUiThread(
                        [&]()
                        {
                            _jobQueue.clear();
                            OnQueueFailed();
                        });
                };

                for (;;)
                {
                    std::function<void()> f;
                    runOnUiThread(
                        [&]()
                        {
                            if (!_jobQueue.empty())
                            {
                                f = _jobQueue.front();
                                _jobQueue.pop_front();
                            }
                        });
                    if (!f)
                        break;

                    try
                    {
                        f();
                    }
                    catch (const EmergencyStopException& e)
                    {
                        fail();
                        throw e;
                    }
                    catch (const ErrorException& e)
                    {
                        fail();
                        throw e;
                    }

                    runOnUiThread(
                        [&]()
                        {
                            OnQueueEmpty();
                        });
                }
            });
    }
}

bool JobQueue::IsQueueEmpty() const
{
    return _jobQueue.empty();
}

bool JobQueue::IsQueueRunning()
{
    return wxGetApp().IsWorkerThreadRunning();
}
