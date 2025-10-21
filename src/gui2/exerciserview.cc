#include <hex/api/content_registry/user_interface.hpp>
#include <hex/api/theme_manager.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include <fonts/tabler_icons.hpp>
#include <fmt/format.h>
#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/disk.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "globals.h"
#include "exerciserview.h"
#include "datastore.h"
#include "utils.h"
#include <implot.h>
#include <implot_internal.h>

using namespace hex;

ExerciserView::ExerciserView(): View::Modal("fluxengine.view.exerciser.name", ICON_VS_DEBUG) {}

void ExerciserView::drawContent()
{
}