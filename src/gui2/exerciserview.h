#pragma once

#include <hex/ui/view.hpp>

class ExerciserView : public hex::View::Modal
{
public:
    ExerciserView();
    ~ExerciserView() override = default;

    void drawContent() override;

    [[nodiscard]] bool shouldDraw() const override
    {
        return true;
    }
    [[nodiscard]] bool hasViewMenuItemEntry() const override
    {
        return false;
    }
};

