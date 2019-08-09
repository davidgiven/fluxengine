#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fluxsink/fluxsink.h"

static bool ends_with(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::unique_ptr<FluxSink> FluxSink::create(const FluxSpec& spec)
{
    const auto& filename = spec.filename;

    if (filename.empty())
        return createHardwareFluxSink(spec.drive);
    else if (ends_with(filename, ".flux"))
        return createSqliteFluxSink(filename);

    Error() << "unrecognised flux filename extension";
    return std::unique_ptr<FluxSink>();
}
