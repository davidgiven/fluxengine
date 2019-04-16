#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fluxwriter.h"

static bool ends_with(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::unique_ptr<FluxWriter> FluxWriter::create(const DataSpec& spec)
{
    const auto& filename = spec.filename;

    if (filename.empty())
        return createHardwareFluxWriter(spec.drive);
    else if (ends_with(filename, ".flux"))
        return createSqliteFluxWriter(filename);

    Error() << "unrecognised flux filename extension";
    return std::unique_ptr<FluxWriter>();
}
