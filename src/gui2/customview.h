#pragma once

#include <hex/ui/view.hpp>

class CustomView : public hex::View::Window {
public:
    CustomView();
    ~CustomView() override = default;

    void drawContent() override;

    [[nodiscard]] bool shouldDraw() const override { return true; }
    [[nodiscard]] bool hasViewMenuItemEntry() const override { return true; }

};
