#pragma once

#include "lib/core/logger.h"
#include <hex/ui/view.hpp>

class LogView : public hex::View::Window
{
public:
    LogView();
    ~LogView() override = default;

    void drawContent() override;
    static void logMessage(const AnyLogMessage& message);

    [[nodiscard]] bool shouldDraw() const override
    {
        return true;
    }
    [[nodiscard]] bool hasViewMenuItemEntry() const override
    {
        return true;
    }
};
