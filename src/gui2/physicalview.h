#pragma once

#include <hex/ui/view.hpp>
#include "abstractsectorview.h"

class PhysicalView : public AbstractSectorView
{
public:
    PhysicalView();
    ~PhysicalView();

    const Sector* getSector(
        unsigned cylinder, unsigned head, unsigned sectorId) override;
    DiskLayout::LayoutBounds getBounds() override;
};
