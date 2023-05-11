#include "lib/globals.h"
#include "lib/config.h"
#include "lib/proto.h"
#include "lib/logger.h"
#include "lib/utils.h"
#include <fstream>
#include <google/protobuf/text_format.h>
#include <regex>

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

void Config::clear()
{
    (*this)->Clear();
}

void Config::set(std::string key, std::string value)
{
    setProtoByString(*this, key, value);
}

std::string Config::get(std::string key)
{
    return getProtoByString(*this, key);
}

static ConfigProto loadSingleConfigFile(std::string filename)
{
    const auto& it = formats.find(filename);
    if (it != formats.end())
        return *it->second;
    else
    {
        std::ifstream f(filename, std::ios::out);
        if (f.fail())
            error("Cannot open '{}': {}", filename, strerror(errno));

        std::ostringstream ss;
        ss << f.rdbuf();

        ConfigProto config;
        if (!google::protobuf::TextFormat::MergeFromString(
                ss.str(), globalConfig()))
            error("couldn't load external config proto");
        return config;
    }
}

void Config::readConfigFile(std::string filename)
{
    globalConfig()->MergeFrom(loadSingleConfigFile(filename));
}

const OptionProto& Config::findOption(const std::string& optionName)
{
    const OptionProto* found = nullptr;

    auto searchOptionList = [&](auto& optionList)
    {
        for (const auto& option : optionList)
        {
            if (optionName == option.name())
            {
                found = &option;
                return true;
            }
        }
        return false;
    };

    if (searchOptionList((*this)->option()))
        return *found;

    for (const auto& optionGroup : (*this)->option_group())
    {
        if (searchOptionList(optionGroup.option()))
            return *found;
    }

    throw OptionNotFoundException("option name not found");
}

bool Config::isOptionValid(const OptionProto& option)
{
    for (const auto& req : option.requires())
    {
        bool matched = false;
        try
        {
            auto value = get(req.key());
            for (auto requiredValue : req.value())
                matched |= (requiredValue == value);
        }
        catch (const ProtoPathNotFoundException e)
        {
            /* This field isn't available, therefore it cannot match. */
        }

        if (!matched)
            return false;
    }

    return true;
}

void Config::applyOption(const OptionProto& option)
{
    if (option.config().option_size() > 0)
        throw InvalidOptionException(fmt::format(
            "option '{}' has an option inside it, which isn't allowed",
            option.name()));
    if (option.config().option_group_size() > 0)
        throw InvalidOptionException(fmt::format(
            "option '{}' has an option group inside it, which isn't allowed",
            option.name()));
    if (!isOptionValid(option))
        throw InapplicableOptionException(
            fmt::format("option '{}' is inapplicable to this configuration",
                option.name()));

    log("OPTION: {}",
        option.has_message() ? option.message() : option.comment());

    (*this)->MergeFrom(option.config());
}

void Config::setFluxSource(std::string filename)
{
    _readState = IO_FLUX;

    static const std::vector<std::pair<std::regex,
        std::function<void(const std::string&, FluxSourceProto*)>>>
        formats = {
            {std::regex("^(.*\\.flux)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSourceProto::FLUX);
                    proto->mutable_fl2()->set_filename(s);
                }},
            {std::regex("^(.*\\.scp)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSourceProto::SCP);
                    proto->mutable_scp()->set_filename(s);
                }},
            {std::regex("^(.*\\.a2r)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSourceProto::A2R);
                    proto->mutable_a2r()->set_filename(s);
                }},
            {std::regex("^(.*\\.cwf)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSourceProto::CWF);
                    proto->mutable_cwf()->set_filename(s);
                }},
            {std::regex("^erase:$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSourceProto::ERASE);
                }},
            {std::regex("^kryoflux:(.*)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSourceProto::KRYOFLUX);
                    proto->mutable_kryoflux()->set_directory(s);
                }},
            {std::regex("^testpattern:(.*)"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSourceProto::TEST_PATTERN);
                }},
            {std::regex("^drive:(.*)"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSourceProto::DRIVE);
                    globalConfig()->mutable_drive()->set_drive(std::stoi(s));
                }},
            {std::regex("^flx:(.*)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSourceProto::FLX);
                    proto->mutable_flx()->set_directory(s);
                }},
    };

    for (const auto& it : formats)
    {
        std::smatch match;
        if (std::regex_match(filename, match, it.first))
        {
            it.second(match[1], (*this)->mutable_flux_source());
            return;
        }
    }

    error("unrecognised flux filename '{}'", filename);
}

static void setFluxSinkImpl(std::string filename, FluxSinkProto* proto)
{
    static const std::vector<std::pair<std::regex,
        std::function<void(const std::string&, FluxSinkProto*)>>>
        formats = {
            {std::regex("^(.*\\.a2r)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSinkProto::A2R);
                    proto->mutable_a2r()->set_filename(s);
                }},
            {std::regex("^(.*\\.flux)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSinkProto::FLUX);
                    proto->mutable_fl2()->set_filename(s);
                }},
            {std::regex("^(.*\\.scp)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSinkProto::SCP);
                    proto->mutable_scp()->set_filename(s);
                }},
            {std::regex("^vcd:(.*)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSinkProto::VCD);
                    proto->mutable_vcd()->set_directory(s);
                }},
            {std::regex("^au:(.*)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSinkProto::AU);
                    proto->mutable_au()->set_directory(s);
                }},
            {std::regex("^drive:(.*)"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSinkProto::DRIVE);
                    globalConfig()->mutable_drive()->set_drive(std::stoi(s));
                }},
    };

    for (const auto& it : formats)
    {
        std::smatch match;
        if (std::regex_match(filename, match, it.first))
        {
            it.second(match[1], proto);
            return;
        }
    }

    error("unrecognised flux filename '{}'", filename);
}

void Config::setFluxSink(std::string filename)
{
    setFluxSinkImpl(filename, (*this)->mutable_flux_sink());
}

void Config::setCopyFluxTo(std::string filename)
{
    setFluxSinkImpl(
        filename, (*this)->mutable_decoder()->mutable_copy_flux_to());
}

void Config::setImageReader(std::string filename)
{
    static const std::map<std::string, std::function<void(ImageReaderProto*)>>
        formats = {
  // clang-format off
		{".adf",      [](auto* proto) { proto->set_type(ImageReaderProto::IMG); }},
		{".d64",      [](auto* proto) { proto->set_type(ImageReaderProto::D64); }},
		{".d81",      [](auto* proto) { proto->set_type(ImageReaderProto::IMG); }},
		{".d88",      [](auto* proto) { proto->set_type(ImageReaderProto::D88); }},
		{".dim",      [](auto* proto) { proto->set_type(ImageReaderProto::DIM); }},
		{".diskcopy", [](auto* proto) { proto->set_type(ImageReaderProto::DISKCOPY); }},
		{".dsk",      [](auto* proto) { proto->set_type(ImageReaderProto::IMG); }},
		{".fdi",      [](auto* proto) { proto->set_type(ImageReaderProto::FDI); }},
		{".imd",      [](auto* proto) { proto->set_type(ImageReaderProto::IMD); }},
		{".img",      [](auto* proto) { proto->set_type(ImageReaderProto::IMG); }},
		{".jv3",      [](auto* proto) { proto->set_type(ImageReaderProto::JV3); }},
		{".nfd",      [](auto* proto) { proto->set_type(ImageReaderProto::NFD); }},
		{".nsi",      [](auto* proto) { proto->set_type(ImageReaderProto::NSI); }},
		{".st",       [](auto* proto) { proto->set_type(ImageReaderProto::IMG); }},
		{".td0",      [](auto* proto) { proto->set_type(ImageReaderProto::TD0); }},
		{".vgi",      [](auto* proto) { proto->set_type(ImageReaderProto::IMG); }},
		{".xdf",      [](auto* proto) { proto->set_type(ImageReaderProto::IMG); }},
  // clang-format on
    };

    for (const auto& it : formats)
    {
        if (endsWith(filename, it.first))
        {
            it.second((*this)->mutable_image_reader());
            (*this)->mutable_image_reader()->set_filename(filename);
            return;
        }
    }

    error("unrecognised image filename '{}'", filename);
}

void Config::setImageWriter(std::string filename)
{
    static const std::map<std::string, std::function<void(ImageWriterProto*)>>
        formats = {
  // clang-format off
		{".adf",      [](auto* proto) { proto->set_type(ImageWriterProto::IMG); }},
		{".d64",      [](auto* proto) { proto->set_type(ImageWriterProto::D64); }},
		{".d81",      [](auto* proto) { proto->set_type(ImageWriterProto::IMG); }},
		{".d88",      [](auto* proto) { proto->set_type(ImageWriterProto::D88); }},
		{".diskcopy", [](auto* proto) { proto->set_type(ImageWriterProto::DISKCOPY); }},
		{".dsk",      [](auto* proto) { proto->set_type(ImageWriterProto::IMG); }},
		{".img",      [](auto* proto) { proto->set_type(ImageWriterProto::IMG); }},
		{".imd",      [](auto* proto) { proto->set_type(ImageWriterProto::IMD); }},
		{".ldbs",     [](auto* proto) { proto->set_type(ImageWriterProto::LDBS); }},
		{".nsi",      [](auto* proto) { proto->set_type(ImageWriterProto::NSI); }},
		{".raw",      [](auto* proto) { proto->set_type(ImageWriterProto::RAW); }},
		{".st",       [](auto* proto) { proto->set_type(ImageWriterProto::IMG); }},
		{".vgi",      [](auto* proto) { proto->set_type(ImageWriterProto::IMG); }},
		{".xdf",      [](auto* proto) { proto->set_type(ImageWriterProto::IMG); }},
  // clang-format on
    };

    for (const auto& it : formats)
    {
        if (endsWith(filename, it.first))
        {
            it.second((*this)->mutable_image_writer());
            (*this)->mutable_image_writer()->set_filename(filename);
            return;
        }
    }

    error("unrecognised image filename '{}'", filename);
}
