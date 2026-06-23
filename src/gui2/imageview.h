#pragma once

#include <hex/ui/view.hpp>
#include "abstractsectorview.h"

class ImageView : public AbstractSectorView
{
public:
    ImageView();

    const Sector* getSector(
        unsigned cylinder, unsigned head, unsigned sectorId) override;
    DiskLayout::LayoutBounds getBounds() override;
};
