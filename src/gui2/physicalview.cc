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

Layout::LayoutBounds PhysicalView::getBounds()
{
    return Datastore::getDiskPhysicalBounds();
}

std::shared_ptr<const Sector> PhysicalView::getSector(
    unsigned cylinder, unsigned head, unsigned sectorId)
{
    return Datastore::findSectorByPhysicalLocation({cylinder, head, sectorId});
}
