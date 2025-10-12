#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/proto.h"
#include "lib/core/logger.h"
#include "lib/core/utils.h"
#include <fstream>
#include <google/protobuf/text_format.h>
#include <fmt/ranges.h>

static Config config;

enum ConstructorMode
{
    MODE_RO,
    MODE_WO,
    MODE_RW
};

struct ImageConstructor
{
    std::string extension;
    ImageReaderWriterType type;
    ConstructorMode mode;
};

static const std::vector<FluxConstructor> fluxConstructors = {
    {/* The .flux format must be first. */
        .name = "FluxEngine (.flux)",
     .pattern = std::regex("^(.*\\.flux)$"),
     .source =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_FLUX);
            proto->mutable_fl2()->set_filename(s);
        }, .sink =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_FLUX);
            proto->mutable_fl2()->set_filename(s);
        }},
    {
     .name = "Supercard Pro (.scp)",
     .pattern = std::regex("^(.*\\.scp)$"),
     .source =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_SCP);
            proto->mutable_scp()->set_filename(s);
        }, .sink =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_SCP);
            proto->mutable_scp()->set_filename(s);
        }, },
    {.name = "AppleSauce (.a2r)",
     .pattern = std::regex("^(.*\\.a2r)$"),
     .source =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_A2R);
            proto->mutable_a2r()->set_filename(s);
        }, .sink =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_A2R);
            proto->mutable_a2r()->set_filename(s);
        }},
    {.name = "CatWeazle (.cwf)",
     .pattern = std::regex("^(.*\\.cwf)$"),
     .source =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_CWF);
            proto->mutable_cwf()->set_filename(s);
        }},
    {.name = "CatWeazle DMK directory",
     .pattern = std::regex("^dmk:(.*)$"),
     .source =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_DMK);
            proto->mutable_dmk()->set_directory(s);
        }},
    {.pattern = std::regex("^erase:$"),
     .source =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_ERASE);
        }},
    {.name = "KryoFlux directory",
     .pattern = std::regex("^kryoflux:(.*)$"),
     .source =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_KRYOFLUX);
            proto->mutable_kryoflux()->set_directory(s);
        }},
    {.pattern = std::regex("^testpattern:(.*)"),
     .source =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_TEST_PATTERN);
        }},
    {.pattern = std::regex("^drive:(.*)"),
     .source =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_DRIVE);
            globalConfig().overrides()->mutable_drive()->set_drive(
                std::stoi(s));
        }, .sink =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_DRIVE);
            globalConfig().overrides()->mutable_drive()->set_drive(
                std::stoi(s));
        }},
    {.name = "FluxCopy directory",
     .pattern = std::regex("^flx:(.*)$"),
     .source =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_FLX);
            proto->mutable_flx()->set_directory(s);
        }},
    {.name = "Value Change Dump directory",
     .pattern = std::regex("^vcd:(.*)$"),
     .sink =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_VCD);
            proto->mutable_vcd()->set_directory(s);
        }},
    {.name = "Audio file directory",
     .pattern = std::regex("^au:(.*)$"),
     .sink =
            [](auto& s, auto* proto)
        {
            proto->set_type(FLUXTYPE_AU);
            proto->mutable_au()->set_directory(s);
        }},
};

static const std::vector<ImageConstructor> imageConstructors = {
    {".adf",      IMAGETYPE_IMG,      MODE_RW},
    {".d64",      IMAGETYPE_D64,      MODE_RW},
    {".d81",      IMAGETYPE_IMG,      MODE_RW},
    {".d88",      IMAGETYPE_D88,      MODE_RW},
    {".dim",      IMAGETYPE_DIM,      MODE_RO},
    {".diskcopy", IMAGETYPE_DISKCOPY, MODE_RW},
    {".dsk",      IMAGETYPE_IMG,      MODE_RW},
    {".fdi",      IMAGETYPE_FDI,      MODE_RO},
    {".imd",      IMAGETYPE_IMD,      MODE_RW},
    {".img",      IMAGETYPE_IMG,      MODE_RW},
    {".jv3",      IMAGETYPE_JV3,      MODE_RO},
    {".nfd",      IMAGETYPE_NFD,      MODE_RO},
    {".nsi",      IMAGETYPE_NSI,      MODE_RW},
    {".st",       IMAGETYPE_IMG,      MODE_RW},
    {".td0",      IMAGETYPE_TD0,      MODE_RO},
    {".vgi",      IMAGETYPE_IMG,      MODE_RW},
    {".xdf",      IMAGETYPE_IMG,      MODE_RW},
};

struct OptionLogMessage
{
    std::string message;
};

void renderLogMessage(LogRenderer& r, std::shared_ptr<const OptionLogMessage> m)
{
    r.newline().add("OPTION:").add(m->message).newline();
}

Config& globalConfig()
{
    return config;
}

ConfigProto* Config::combined()
{
    if (!_configValid)
    {
        _combinedConfig = _baseConfig;

        for (const auto& optionInfo : _appliedOptions)
            _combinedConfig.MergeFrom(optionInfo.option->config());

        /* Add in the user overrides. */

        _combinedConfig.MergeFrom(_overridesConfig);

        /* At this point the config is valid, although when fluxsources or
         * imagereaders are loaded it may be adjusted again. */

        _configValid = true;
    }
    return &_combinedConfig;
}

void Config::invalidate()
{
    _configValid = false;
}

void Config::clear()
{
    _configValid = false;
    _baseConfig.Clear();
    _overridesConfig.Clear();
    _combinedConfig.Clear();
    _appliedOptions.clear();
}

static std::string getValidValues(const OptionGroupProto& group)
{
    return fmt::format("{}",
        fmt::join(
            std::views::transform(group.option(), &OptionProto::name), ", "));
}

std::vector<std::string> Config::validate()
{
    std::vector<std::string> results;

    /* Ensure that only one item in each group is set. */

    std::map<const OptionGroupProto*, const OptionProto*> optionsByGroup;
    for (auto& [group, option, hasArgument] : _appliedOptions)
        if (group)
        {
            auto& o = optionsByGroup[group];
            if (o)
                results.push_back(
                    fmt::format("multiple mutually exclusive values set for "
                                "group '{}': valid values are: {}",
                        group->comment(),
                        getValidValues(*group)));
            o = option;
        }

    /* Ensure that every group has an option set. */

    for (const auto& group : base()->option_group())
    {
        if (!optionsByGroup.contains(&group))
        {
            results.push_back(
                fmt::format("no value set for group '{}': valid values are: {}",
                    group.comment(),
                    getValidValues(group)));
        }
    }

    /* Check option requirements. */

    for (auto [group, option, hasArgument] : _appliedOptions)
    {
        try
        {
            checkOptionValid(*option);
        }
        catch (const InapplicableOptionException& e)
        {
            results.push_back(e.message);
        }
    }

    return results;
}

void Config::validateAndThrow()
{
    auto r = validate();
    if (!r.empty())
    {
        std::stringstream ss;
        ss << "invalid configuration:\n";
        for (auto& s : r)
            ss << s << '\n';
        throw InapplicableOptionException(ss.str());
    }
}

void Config::set(std::string key, std::string value)
{
    setProtoByString(overrides(), key, value);
}

void Config::setTransient(std::string key, std::string value)
{
    setProtoByString(&_combinedConfig, key, value);
}

std::string Config::get(std::string key)
{
    return getProtoByString(combined(), key);
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
        if (!google::protobuf::TextFormat::MergeFromString(ss.str(), &config))
            error("couldn't load external config proto");
        return config;
    }
}

void Config::readBaseConfigFile(std::string filename)
{
    base()->MergeFrom(loadSingleConfigFile(filename));
}

void Config::readBaseConfig(std::string data)
{
    if (!google::protobuf::TextFormat::MergeFromString(data, base()))
        error("couldn't load external config proto");
}

Config::OptionInfo Config::findOption(
    const std::string& name, const std::string value)
{
    const OptionProto* found = nullptr;

    auto searchOptionList = [&](auto& optionList, const std::string& optionName)
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

    /* First look for any group names which match. */

    if (!value.empty())
        for (const auto& optionGroup : base()->option_group())
            if (optionGroup.name() == name)
            {
                /* The option must therefore be one of these. */

                if (searchOptionList(optionGroup.option(), value))
                    return {&optionGroup, found, true};

                throw OptionNotFoundException(fmt::format(
                    "value {} is not valid for option {}; valid values are: {}",
                    value,
                    name,
                    fmt::join(std::views::transform(
                                  optionGroup.option(), &OptionProto::name),
                        ", ")));
            }

    /* Now search for individual options. */

    if (searchOptionList(base()->option(), name))
        return {nullptr, found, false};

    for (const auto& optionGroup : base()->option_group())
    {
        if (optionGroup.name().empty())
            if (searchOptionList(optionGroup.option(), name))
                return {&optionGroup, found, false};
    }

    throw OptionNotFoundException(fmt::format("option {} not found", name));
}

void Config::checkOptionValid(const OptionProto& option)
{
    for (const auto& req : option.prerequisite())
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
            /* This field isn't available, therefore it
             * cannot match. */
        }

        if (!matched)
        {
            std::stringstream ss;
            ss << '[';
            bool first = true;
            for (auto requiredValue : req.value())
            {
                if (!first)
                    ss << ", ";
                ss << quote(requiredValue);
                first = false;
            }
            ss << ']';

            throw InapplicableOptionException(
                fmt::format("option '{}' is inapplicable to this configuration "
                            "because {}={} could not be met",
                    option.name(),
                    req.key(),
                    ss.str()));
        }
    }
}

bool Config::isOptionValid(const OptionProto& option)
{
    try
    {
        checkOptionValid(option);
        return true;
    }
    catch (const InapplicableOptionException& e)
    {
        return false;
    }
}

void Config::applyOption(const OptionInfo& optionInfo)
{
    auto* option = optionInfo.option;
    log(OptionLogMessage{
        option->has_message() ? option->message() : option->comment()});

    _appliedOptions.insert(optionInfo);
}

bool Config::applyOption(const std::string& name, const std::string value)
{
    auto optionInfo = findOption(name, value);
    applyOption(optionInfo);
    return optionInfo.usesValue;
}

void Config::applyDefaultOptions()
{
    std::set<const OptionGroupProto*> appliedOptionGroups;
    for (auto& [group, option, hasArgument] : _appliedOptions)
        if (group)
            appliedOptionGroups.insert(group);

    /* For every group which doesn't have an option set, find the default and
     * set it. */

    for (const auto& group : base()->option_group())
    {
        if (!appliedOptionGroups.contains(&group))
        {
            for (const auto& option : group.option())
            {
                if (option.set_by_default())
                    applyOption({&group, &option, false});
            }
        }
    }
}

void Config::clearOptions()
{
    _appliedOptions.clear();
    invalidate();
}

static void setFluxSourceImpl(
    const std::string& filename, FluxSourceProto* proto)
{
    for (const auto& it : fluxConstructors)
    {
        std::smatch match;
        if (std::regex_match(filename, match, it.pattern))
        {
            if (!it.source)
                throw new InapplicableValueException();
            it.source(match[1], proto);
            return;
        }
    }

    error("unrecognised flux filename '{}'", filename);
}

void Config::setFluxSource(std::string filename)
{
    setFluxSourceImpl(filename, overrides()->mutable_flux_source());
}

static void setFluxSinkImpl(const std::string& filename, FluxSinkProto* proto)
{
    for (const auto& it : fluxConstructors)
    {
        std::smatch match;
        if (std::regex_match(filename, match, it.pattern))
        {
            if (!it.sink)
                throw new InapplicableValueException();
            it.sink(match[1], proto);
            return;
        }
    }

    error("unrecognised flux filename '{}'", filename);
}

void Config::setFluxSink(std::string filename)
{
    setFluxSinkImpl(filename, overrides()->mutable_flux_sink());
}

void Config::setCopyFluxTo(std::string filename)
{
    setFluxSinkImpl(
        filename, overrides()->mutable_decoder()->mutable_copy_flux_to());
}

void Config::setVerificationFluxSource(std::string filename)
{
    setFluxSourceImpl(filename, &_verificationFluxSourceProto);
}

void Config::setImageReader(std::string filename)
{
    for (const auto& it : imageConstructors)
    {
        if (endsWith(filename, it.extension))
        {
            if (it.mode == MODE_WO)
                throw new InapplicableValueException();

            overrides()->mutable_image_reader()->set_type(it.type);
            overrides()->mutable_image_reader()->set_filename(filename);
            return;
        }
    }

    error("unrecognised image filename '{}'", filename);
}

void Config::setImageWriter(std::string filename)
{
    for (const auto& it : imageConstructors)
    {
        if (endsWith(filename, it.extension))
        {
            if (it.mode == MODE_RO)
                throw new InapplicableValueException();

            overrides()->mutable_image_writer()->set_type(it.type);
            overrides()->mutable_image_writer()->set_filename(filename);
            return;
        }
    }

    error("unrecognised image filename '{}'", filename);
}

bool Config::hasFluxSource()
{
    return (*this)->flux_source().type() != FLUXTYPE_NOT_SET;
}

bool Config::hasVerificationFluxSource() const
{
    return _verificationFluxSourceProto.type() != FLUXTYPE_NOT_SET;
}

bool Config::hasImageReader()
{
    return (*this)->image_reader().type() != IMAGETYPE_NOT_SET;
}

bool Config::hasFluxSink()
{
    return (*this)->flux_sink().type() != FLUXTYPE_NOT_SET;
}

bool Config::hasImageWriter()
{
    return (*this)->image_writer().type() != IMAGETYPE_NOT_SET;
}

bool Config::hasEncoder()
{
    return (*this)->has_encoder();
}

bool Config::hasDecoder()
{
    return _combinedConfig.has_decoder();
}

const std::vector<FluxConstructor>& Config::getFluxFormats()
{
    return fluxConstructors;
}