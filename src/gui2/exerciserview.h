#pragma once

#include <hex/ui/view.hpp>

class ExerciserView : public hex::View::Modal
{
public:
    ExerciserView();
    ~ExerciserView() override = default;

    void drawContent() override;

protected:
    void onOpen() override;
    void onClose() override;

public:
    [[nodiscard]] bool shouldDraw() const override
    {
        return true;
    }
    [[nodiscard]] bool hasViewMenuItemEntry() const override
    {
        return false;
    }

    ImVec2 getMinSize() const override
    {
        return {800, 100};
    }

    int getWindowFlags() const override
    {
        return ImGuiWindowFlags_AlwaysAutoResize;
    }
};
