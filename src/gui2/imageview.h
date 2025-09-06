#pragma once

#include <hex/ui/view.hpp>

class ImageView : public hex::View::Window {
public:
    ImageView();
    ~ImageView() override = default;

    void drawContent() override;

    [[nodiscard]] bool shouldDraw() const override { return true; }
    [[nodiscard]] bool hasViewMenuItemEntry() const override { return true; }

};
