#pragma once

#include "lib/core/logger.h"
#include <hex/ui/view.hpp>

class VisualiserView : public hex::View::Window
{
public:
    VisualiserView();
    ~VisualiserView() override = default;

    void drawContent() override;

    [[nodiscard]] bool shouldDraw() const override
    {
        return true;
    }
    [[nodiscard]] bool hasViewMenuItemEntry() const override
    {
        return true;
    }
};

