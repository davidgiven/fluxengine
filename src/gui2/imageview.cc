#include <hex/api/content_registry/user_interface.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include "lib/core/globals.h"
#include "lib/data/image.h"
#include "lib/data/sector.h"
#include "imageview.h"
#include "datastore.h"

using namespace hex;

ImageView::ImageView():
    View::Window("fluxengine.view.image.name", ICON_VS_DEBUG_LINE_BY_LINE)
{
}

void ImageView::drawContent()
{
    auto diskFlux = Datastore::getDiskFlux();
    if (!diskFlux)
        return;
    auto& image = diskFlux->image;
    if (!image)
        return;

    int lastSector = -1;
    for (int i = 0; i < image->getBlockCount(); i++)
    {
        auto [logicalCylinder, logicalHead, logicalSector] =
            image->findBlock(i);
        auto sector = image->getBlock(i);

        if (logicalSector >= lastSector)
            ImGui::SameLine();
        ImGui::Selectable(
            fmt::format("c{}h{}s{}", logicalCylinder, logicalHead, logicalSector)
                .c_str(),
            true);
        lastSector = logicalSector;
    }
}
