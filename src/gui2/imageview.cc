#include <hex/api/content_registry/user_interface.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include "lib/core/globals.h"
#include "lib/data/image.h"
#include "lib/data/sector.h"
#include "globals.h"
#include "imageview.h"
#include "datastore.h"

using namespace hex;

ImageView::ImageView(): AbstractSectorView("fluxengine.view.image.name") {}

DiskLayout::LayoutBounds ImageView::getBounds()
{
    return Datastore::getDiskLayout()->getLogicalBounds();
}

std::shared_ptr<const Sector> ImageView::getSector(
    unsigned logicalCylinder, unsigned logicalHead, unsigned sectorId)
{
    auto diskFlux = Datastore::getDecodedDisk();
    return diskFlux->image->get({logicalCylinder, logicalHead, sectorId});
}
