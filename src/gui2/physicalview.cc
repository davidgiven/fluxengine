#include <hex/api/content_registry/user_interface.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include "lib/core/globals.h"
#include "lib/data/image.h"
#include "lib/data/sector.h"
#include "globals.h"
#include "physicalview.h"
#include "datastore.h"

using namespace hex;

PhysicalView::PhysicalView():
    AbstractSectorView("fluxengine.view.physical.name")
{
}

PhysicalView::~PhysicalView() {}

DiskLayout::LayoutBounds PhysicalView::getBounds()
{
    return Datastore::getDiskLayout()->getPhysicalBounds();
}

std::shared_ptr<const Sector> PhysicalView::getSector(
    unsigned physicalCylinder, unsigned physicalHead, unsigned sectorId)
{
    const auto& diskFlux = Datastore::getDecodedDisk();
    const auto& diskLayout = Datastore::getDiskLayout();
    const auto& ptl = findOrDefault(
        diskLayout->layoutByPhysicalLocation, {physicalCylinder, physicalHead});
    if (!ptl)
        return nullptr;
    const auto& ltl = ptl->logicalTrackLayout;
    return diskFlux->image->get(
        {ltl->logicalCylinder, ltl->logicalHead, sectorId});
}
