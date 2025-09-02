#pragma once

class DiskFlux;

class Datastore
{
public:
    static std::shared_ptr<const DiskFlux> getDiskFlux();

    static void runOnWorkerThread(std::function<void()> callback);
};