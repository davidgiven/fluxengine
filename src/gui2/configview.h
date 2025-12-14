#pragma once

#include <hex/ui/view.hpp>

class ConfigView : public hex::View::Window
{
public:
    ConfigView();
    ~ConfigView() override = default;

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
