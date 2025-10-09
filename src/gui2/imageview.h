#pragma once

#include <hex/ui/view.hpp>
#include "abstractsectorview.h"

class ImageView : public AbstractSectorView
{
public:
    ImageView();

    std::shared_ptr<const Sector> getSector(
        unsigned cylinder, unsigned head, unsigned sectorId) override;
    DiskLayout::LayoutBounds getBounds() override;
};
