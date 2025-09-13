#pragma once

#include <hex/ui/view.hpp>
#include "lib/core/globals.h"
#include "lib/data/layout.h"

class Sector;

class AbstractSectorView : public hex::View::Window
{
public:
    AbstractSectorView(const std::string& name);

    void drawContent() override;

    [[nodiscard]] bool shouldDraw() const override
    {
        return true;
    }
    [[nodiscard]] bool hasViewMenuItemEntry() const override
    {
        return true;
    }

protected:
    virtual std::shared_ptr<const Sector> getSector(
        unsigned cylinder, unsigned head, unsigned sectorId) = 0;
    virtual Layout::LayoutBounds getBounds() = 0;
};
