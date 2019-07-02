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
        locations.clear();

        for (size_t i = 1; i < words.size(); i++)
        {
            auto mod = parseMod(words[i]);
            if ((mod.name != "t") && (mod.name != "s") && (mod.name != "d"))
                Error() << fmt::format("unknown data modifier '{}'", mod.name);
            modifiers[mod.name] = mod;
        }

        const auto& drives = modifiers["d"].data;
        if (drives.size() != 1)
            Error() << "you must specify exactly one drive";
        drive = *drives.begin();

        const auto& tracks = modifiers["t"].data;
        const auto& sides = modifiers["s"].data;
        for (auto track : tracks)
        {
            for (auto side : sides)
                locations.push_back({ drive, track, side });
        }
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
