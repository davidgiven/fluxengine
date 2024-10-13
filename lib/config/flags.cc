#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/config/proto.h"
#include "lib/core/utils.h"
#include "lib/core/logger.h"
#include <google/protobuf/text_format.h>
#include <regex>
#include <fstream>

static FlagGroup* currentFlagGroup;
static std::vector<Flag*> all_flags;
static std::map<const std::string, Flag*> flags_by_name;

static void doHelp();
static void doShowConfig();
static void doDoc();

static FlagGroup helpGroup;
static ActionFlag helpFlag = ActionFlag({"--help"}, "Shows the help.", doHelp);

static ActionFlag showConfigFlag = ActionFlag({"--config", "-C"},
    "Shows the currently set configuration and halts.",
    doShowConfig);

static ActionFlag docFlag = ActionFlag(
    {"--doc"}, "Shows the available configuration options and halts.", doDoc);

FlagGroup::FlagGroup()
{
    currentFlagGroup = this;
}

FlagGroup::FlagGroup(std::initializer_list<FlagGroup*> groups): _groups(groups)
{
    currentFlagGroup = this;
}

void FlagGroup::addFlag(Flag* flag)
{
    _flags.push_back(flag);
}

std::vector<std::string> FlagGroup::parseFlagsWithFilenames(int argc,
    const char* argv[],
    std::function<bool(const std::string&)> callback)
{
    if (_initialised)
        throw std::runtime_error("called parseFlags() twice");

    /* Recursively accumulate a list of all flags. */

    all_flags.clear();
    flags_by_name.clear();
    std::function<void(FlagGroup*)> recurse;
    recurse = [&](FlagGroup* group)
    {
        if (group->_initialised)
            return;

        for (FlagGroup* subgroup : group->_groups)
            recurse(subgroup);

        for (Flag* flag : group->_flags)
        {
            for (const auto& name : flag->names())
            {
                if (flags_by_name.find(name) != flags_by_name.end())
                    error("two flags use the name '{}'", name);
                flags_by_name[name] = flag;
            }

            all_flags.push_back(flag);
        }

        group->_initialised = true;
    };
    recurse(this);
    recurse(&helpGroup);

    /* Now actually parse them. */

    std::vector<std::string> filenames;
    int index = 1;
    while (index < argc)
    {
        std::string thisarg = argv[index];
        std::string thatarg = (index < (argc - 1)) ? argv[index + 1] : "";

        std::string key;
        std::string value;
        bool usesthat = false;

        if (thisarg.size() == 0)
        {
            /* Ignore this argument. */
        }
        else if (thisarg[0] != '-')
        {
            /* This is a filename. Pass it to the callback, and if not consumed
             * queue it. */
            if (!callback(thisarg))
                filenames.push_back(thisarg);
        }
        else
        {
            /* This is a flag. */

            if ((thisarg.size() > 1) && (thisarg[1] == '-'))
            {
                /* Long option. */

                auto equals = thisarg.rfind('=');
                if (equals != std::string::npos)
                {
                    key = thisarg.substr(0, equals);
                    value = thisarg.substr(equals + 1);
                }
                else
                {
                    key = thisarg;
                    value = thatarg;
                    usesthat = true;
                }
            }
            else
            {
                /* Short option. */

                if (thisarg.size() > 2)
                {
                    key = thisarg.substr(0, 2);
                    value = thisarg.substr(2);
                }
                else
                {
                    key = thisarg;
                    value = thatarg;
                    usesthat = true;
                }
            }

            auto flag = flags_by_name.find(key);
            if (flag == flags_by_name.end())
            {
                if (beginsWith(key, "--"))
                {
                    std::string path = key.substr(2);
                    if (key.find('.') != std::string::npos)
                    {
                        globalConfig().set(path, value);
                        index += usesthat;
                    }
                    else
                        globalConfig().applyOption(path);
                }
                else
                    error("unrecognised flag '-{}'; try --help", key);
            }
            else
            {
                flag->second->set(value);
                if (usesthat && flag->second->hasArgument())
                    index++;
            }
        }

        index++;
    }

    globalConfig().validateAndThrow();
    return filenames;
}

void FlagGroup::parseFlags(int argc,
    const char* argv[],
    std::function<bool(const std::string&)> callback)
{
    auto filenames = parseFlagsWithFilenames(argc, argv, callback);
    if (!filenames.empty())
        error(
            "non-option parameter '{}' seen (try --help)", *filenames.begin());
}

void FlagGroup::parseFlagsWithConfigFiles(int argc,
    const char* argv[],
    const std::map<std::string, const ConfigProto*>& configFiles)
{
    parseFlags(argc,
        argv,
        [&](const auto& filename)
        {
            globalConfig().readBaseConfigFile(filename);
            return true;
        });
}

void FlagGroup::checkInitialised() const
{
    if (!_initialised)
        throw std::runtime_error("Attempt to access uninitialised flag");
}

Flag::Flag(const std::vector<std::string>& names, const std::string helptext):
    _group(*currentFlagGroup),
    _names(names),
    _helptext(helptext)
{
    if (!currentFlagGroup)
        error("no flag group defined for {}", *names.begin());
    _group.addFlag(this);
}

void BoolFlag::set(const std::string& value)
{
    if ((value == "true") || (value == "y"))
        _value = true;
    else if ((value == "false") || (value == "n"))
        _value = false;
    else
        error("can't parse '{}'; try 'true' or 'false'", value);
    _callback(_value);
    _isSet = true;
}

const std::string HexIntFlag::defaultValueAsString() const
{
    return fmt::format("0x{:x}", _defaultValue);
}

static void doHelp()
{
    std::cout << "FluxEngine options:\n";
    std::cout
        << "Note: options are processed left to right and order matters!\n";
    for (auto flag : all_flags)
    {
        std::cout << "  ";
        bool firstname = true;
        for (auto name : flag->names())
        {
            if (!firstname)
                std::cout << ", ";
            std::cout << name;
            firstname = false;
        }

        if (flag->hasArgument())
            std::cout << " <default: \"" << flag->defaultValueAsString()
                      << "\">";
        std::cout << ": " << flag->helptext() << std::endl;
    }
    exit(0);
}

static void doShowConfig()
{
    std::string s;
    google::protobuf::TextFormat::PrintToString(globalConfig(), &s);
    std::cout << s << '\n';

    exit(0);
}

static void doDoc()
{
    const auto fields = findAllProtoFields(globalConfig().base());
    for (const auto field : fields)
    {
        const std::string& path = field.first;
        const google::protobuf::FieldDescriptor* f = field.second;

        if (f->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
            continue;

        std::string helpText = f->options().GetExtension(help);
        std::cout << fmt::format("{}: {}\n", path, helpText);
    }

    exit(0);
}
