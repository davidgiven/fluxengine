#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fmt/format.h"
#include <regex>
#include <sstream>

std::vector<std::string> DataSpec::split(
        const std::string& s, const std::string& delimiter)
{
    std::vector<std::string> ret;

    size_t start = 0;
    size_t end = 0;
    size_t len = 0;
    do
    {
        end = s.find(delimiter,start); 
        len = end - start;
        std::string token = s.substr(start, len);
        ret.emplace_back( token );
        start += len + delimiter.length();
    }
    while (end != std::string::npos);
    return ret;
}

DataSpec::Modifier DataSpec::parseMod(const std::string& spec)
{
    static const std::regex MOD_REGEX("([a-z]*)=([-x+0-9,]*)");
    static const std::regex DATA_REGEX("([0-9]+)(?:(?:-([0-9]+))|(?:\\+([0-9]+)))?(?:x([0-9]+))?");

    std::smatch match;
    if (!std::regex_match(spec, match, MOD_REGEX))
        Error() << "invalid data modifier syntax '" << spec << "'";
    
    Modifier m;
    m.name = match[1];
    m.source = spec;
    for (auto& data : split(match[2], ","))
    {
        int start = 0;
        int count = 1;
        int step = 1;

        std::smatch dmatch;
        if (!std::regex_match(data, dmatch, DATA_REGEX))
            Error() << "invalid data in mod '" << data << "'";
        
        start = std::stoi(dmatch[1]);
        if (!dmatch[2].str().empty())
            count = std::stoi(dmatch[2]) - start + 1;
        if (!dmatch[3].str().empty())
            count = std::stoi(dmatch[3]);
        if (!dmatch[4].str().empty())
            step = std::stoi(dmatch[4]);

        if (count < 0)
            Error() << "mod '" << data << "' specifies an illegal quantity";

        for (int i = start; i < (start+count); i += step)
            m.data.insert(i);
    }

    return m;
}

void DataSpec::set(const std::string& spec)
{
    std::vector<std::string> words = split(spec, ":");
    if (words.size() == 0)
        Error() << "empty data specification (you have to specify *something*)";

    filename = words[0];
    if (words.size() > 1)
    {
        for (size_t i = 1; i < words.size(); i++)
        {
            auto mod = parseMod(words[i]);
            modifiers[mod.name] = mod;
        }
    }
}

FluxSpec::FluxSpec(const DataSpec& spec)
{
    filename = spec.filename;

    locations.clear();

    const auto& drives = spec.modifiers.at("d").data;
    if (drives.size() != 1)
        Error() << "you must specify exactly one drive";
    drive = *drives.begin();

    const auto& tracks = spec.modifiers.at("t").data;
    const auto& sides = spec.modifiers.at("s").data;
    for (auto track : tracks)
    {
        for (auto side : sides)
            locations.push_back({ drive, track, side });
    }

    for (const auto& e : spec.modifiers)
    {
        const auto name = e.second.name;
        if ((name != "t") && (name != "s") && (name != "d"))
            Error() << fmt::format("unknown fluxspec modifier '{}'", name);
    }
}

ImageSpec::ImageSpec(const DataSpec& spec)
{
    filename = spec.filename;

    tracks = spec.modifiers.at("t").only();
    heads = spec.modifiers.at("h").only();
    sectors = spec.modifiers.at("s").only();

    for (const auto& e : spec.modifiers)
    {
        const auto name = e.second.name;
        if ((name != "t") && (name != "h") && (name != "s"))
            Error() << fmt::format("unknown fluxspec modifier '{}'", name);
    }
}

DataSpec::operator std::string(void) const
{
    std::stringstream ss;
    ss << filename;

    for (const auto& mod : modifiers)
        ss << ':' << mod.second.source;

    return ss.str();
}
