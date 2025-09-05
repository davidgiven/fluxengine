#pragma once

#include "lib/core/globals.h"
#include "lib/core/logger.h"
#include "lib/data/layout.h"

class DiskFlux;

class Datastore
{
public:
    static void init();

    static bool isBusy();
    static bool isConfigurationValid();
    static const Layout::LayoutBounds& getDiskBounds();
    static void rebuildConfiguration();
    static void onLogMessage(const AnyLogMessage& message);

    static void beginRead();
    static void stop();

    static std::shared_ptr<const DiskFlux> getDiskFlux();

    static void runOnWorkerThread(std::function<void()> callback);
};