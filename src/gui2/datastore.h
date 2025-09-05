#pragma once

#include "lib/core/globals.h"
#include "lib/data/layout.h"

class DiskFlux;

class Datastore
{
public:
    static bool isConfigurationValid();
    static const Layout::LayoutBounds& getDiskBounds();
    static void rebuildConfiguration();

    static void beginRead();

    static std::shared_ptr<const DiskFlux> getDiskFlux();

    static void runOnWorkerThread(std::function<void()> callback);
};