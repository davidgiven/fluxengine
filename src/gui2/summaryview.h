#pragma once

#include <hex/ui/view.hpp>

class SummaryView : public hex::View::Window
{
public:
    SummaryView();
    ~SummaryView() override = default;

    void drawContent() override;
    void drawHelpText() override;

    [[nodiscard]] bool shouldDraw() const override
    {
        return true;
    }
    [[nodiscard]] bool hasViewMenuItemEntry() const override
    {
        return true;
    }
};
