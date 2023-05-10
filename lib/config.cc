#include "lib/globals.h"
#include "lib/config.h"
#include "lib/proto.h"

static Config config;

Config& globalConfig()
{
    return config;
}

ConfigProto* Config::operator->() const
{
    return &globalConfigProto();
}

Config::operator ConfigProto*() const
{
    return &globalConfigProto();
}

Config::operator ConfigProto&() const
{
    return globalConfigProto();
}
