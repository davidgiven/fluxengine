#pragma once

#include <hex/ui/view.hpp>

class ControlPanelView : public hex::View::Window
{
public:
    ControlPanelView();
    ~ControlPanelView() override = default;

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
