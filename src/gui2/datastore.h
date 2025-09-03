#pragma once

class DiskFlux;

class Datastore
{
public:
    static void beginRead();

    static std::shared_ptr<const DiskFlux> getDiskFlux();

    static void runOnWorkerThread(std::function<void()> callback);
};