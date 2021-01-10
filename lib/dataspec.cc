#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fmt/format.h"
#include "dep/agg/include/agg2d.h"
#include "dep/stb/stb_image_write.h"
#include <regex>
#include <sstream>

MissingModifierException::MissingModifierException(const std::string& mod):
    mod(mod),
    std::runtime_error(fmt::format("missing mandatory modifier '{}'", mod))
{}

std::vector<std::string> DataSpec::split(
        const std::string& s, const std::string& delimiter)
{
    std::vector<std::string> ret;

	if (!s.empty())
	{
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
	}

    return ret;
}

std::set<unsigned> DataSpec::parseRange(const std::string& data)
{
	static const std::regex DATA_REGEX("([0-9]+)(?:(?:-([0-9]+))|(?:\\+([0-9]+)))?(?:x([0-9]+))?");

	std::set<unsigned> result;
    for (auto& data : split(data, ","))
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
            result.insert(i);
    }

    return result;
}

DataSpec::Modifier DataSpec::parseMod(const std::string& spec)
{
	static const std::regex MOD_REGEX("([a-z]*)=([-x+0-9,]*)");

    std::smatch match;
    if (!std::regex_match(spec, match, MOD_REGEX))
        Error() << "invalid data modifier syntax '" << spec << "'";
    
    Modifier m;
    m.name = match[1];
    m.source = spec;
	m.data = parseRange(match[2]);
    return m;
}

void DataSpec::set(const std::string& spec)
{
    std::vector<std::string> words = split(spec, ":");
    if (words.size() == 0)
		return;

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

const DataSpec::Modifier& DataSpec::at(const std::string& mod) const
{
    try
    {
        return modifiers.at(mod);
    }
    catch (const std::out_of_range& e)
    {
        throw MissingModifierException(mod);
    }
}

bool DataSpec::has(const std::string& mod) const
{
    return modifiers.find(mod) != modifiers.end();
}

FluxSpec::FluxSpec(const DataSpec& spec)
{
    try 
    {
        filename = spec.filename;

        locations.clear();

        const auto& drives = spec.at("d").data;
        if (drives.size() != 1)
            Error() << "you must specify exactly one drive";
        drive = *drives.begin();

        const auto& tracks = spec.at("t").data;
        const auto& sides = spec.at("s").data;
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
    catch (const MissingModifierException& e)
    {
        Error() << e.what() << " in fluxspec '" << spec << "'";
    }
}

ImageSpec::ImageSpec(const DataSpec& spec)
{
    try
    {
        filename = spec.filename;

        if (!spec.has("c") && !spec.has("h") && !spec.has("s") && !spec.has("b"))
        {
            cylinders = heads = sectors = bytes = 0;
            initialised = false;
        }
        else
        {
            cylinders = spec.at("c").only();
            heads = spec.at("h").only();
            sectors = spec.at("s").only();
            bytes = spec.at("b").only();
            initialised = true;
        }
    }
    catch (const MissingModifierException& e)
    {
        Error() << e.what() << " in imagespec '" << spec << "'";
    }

    for (const auto& e : spec.modifiers)
    {
        const auto name = e.second.name;
        if ((name != "c") && (name != "h") && (name != "s") && (name != "b"))
            Error() << fmt::format("unknown fluxspec modifier '{}'", name);
    }
}

ImageSpec::ImageSpec(const std::string filename,
        unsigned cylinders, unsigned heads, unsigned sectors, unsigned bytes):
    filename(filename),
    cylinders(cylinders),
    heads(heads),
    sectors(sectors),
    bytes(bytes),
    initialised(true)
{}

DataSpec::operator std::string(void) const
{
    std::stringstream ss;
    ss << filename;

    for (const auto& mod : modifiers)
        ss << ':' << mod.second.source;

    return ss.str();
}

BitmapSpec::BitmapSpec(const DataSpec& spec)
{
    try
    {
        filename = spec.filename;

        if (!spec.has("w") && !spec.has("h"))
        {
            width = height = 0;
            initialised = false;
        }
        else
        {
            width = spec.at("w").only();
            height = spec.at("h").only();
            initialised = true;
        }
    }
    catch (const MissingModifierException& e)
    {
        Error() << e.what() << " in imagespec '" << spec << "'";
    }

    for (const auto& e : spec.modifiers)
    {
        const auto name = e.second.name;
        if ((name != "w") && (name != "h"))
            Error() << fmt::format("unknown fluxspec modifier '{}'", name);
    }
}

BitmapSpec::BitmapSpec(const std::string filename, unsigned width, unsigned height):
    filename(filename),
    width(width),
    height(height),
    initialised(true)
{}

Agg2D& BitmapSpec::painter()
{
	if (!_painter)
	{
		_bitmap.resize(width * height * 4, 255);
		_painter.reset(new Agg2D());
		_painter->attach(&_bitmap[0], width, height, width*4);
	}
	return *_painter;
}

void BitmapSpec::save()
{
	stbi_write_png(filename.c_str(), width, height, 4, &_bitmap[0], width*4);
}

