#include "globals.h"
#include "flags.h"

static FlagGroup* currentFlagGroup;
static std::vector<Flag*> all_flags;
static std::map<const std::string, Flag*> flags_by_name;

static void doHelp();

static FlagGroup helpGroup;
static ActionFlag helpFlag = ActionFlag(
    { "--help", "-h" },
    "Shows the help.",
    doHelp);

FlagGroup::FlagGroup(const std::initializer_list<FlagGroup*> groups):
    _groups(groups.begin(), groups.end())
{
    currentFlagGroup = this;
}

FlagGroup::FlagGroup()
{
    currentFlagGroup = this;
}

void FlagGroup::addFlag(Flag* flag)
{
    _flags.push_back(flag);
}

std::vector<std::string> FlagGroup::parseFlagsWithFilenames(int argc, const char* argv[])
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
                    Error() << "two flags use the name '" << name << "'";
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
        std::string thatarg = (index < (argc-1)) ? argv[index+1] : "";

        std::string key;
        std::string value;
        bool usesthat = false;

        if (thisarg.size() == 0)
        {
            /* Ignore this argument. */
        }
        else if (thisarg[0] != '-')
        {
            /* This is a filename. */
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
                    value = thisarg.substr(equals+1);
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
                Error() << "unknown flag '" << key << "'; try --help";

            flag->second->set(value);
            if (usesthat && flag->second->hasArgument())
                index++;
        }

        index++;
    }

    return filenames;
}

void FlagGroup::parseFlags(int argc, const char* argv[])
{
    auto filenames = parseFlagsWithFilenames(argc, argv);
    if (!filenames.empty())
        Error() << "non-option parameter " << *filenames.begin() << " seen (try --help)";
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
        Error() << "no flag group defined for " << *names.begin();
    _group.addFlag(this);
}

void BoolFlag::set(const std::string& value)
{
	if ((value == "true") || (value == "y"))
		_value = true;
	else if ((value == "false") || (value == "n"))
		_value = false;
	else
		Error() << "can't parse '" << value << "'; try 'true' or 'false'";
}

static void doHelp()
{
    std::cout << "FluxEngine options:" << std::endl;
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
            std::cout << " <default: \"" << flag->defaultValueAsString() << "\">";
        std::cout << ": " << flag->helptext() << std::endl;
    }
    exit(0);
}
