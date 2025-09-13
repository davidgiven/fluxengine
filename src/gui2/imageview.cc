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

Layout::LayoutBounds ImageView::getBounds()
{
    return Datastore::getDiskLogicalBounds();
}

std::shared_ptr<const Sector> ImageView::getSector(
    unsigned cylinder, unsigned head, unsigned sectorId)
{
    return Datastore::findSectorByLogicalLocation({cylinder, head, sectorId});
}
