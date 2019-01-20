#include "globals.h"
#include "flags.h"

static std::vector<Flag*> all_flags;
static std::map<const std::string, Flag*> flags_by_name;

Flag::Flag(const std::vector<std::string>& names, const std::string helptext):
    _names(names),
    _helptext(helptext)
{
    for (auto& name : names)
    {
        if (flags_by_name.find(name) != flags_by_name.end())
            Error() << "two flags use the name '" << name << "'";
        flags_by_name[name] = this;
    }

    all_flags.push_back(this);
}

void Flag::parseFlags(int argc, const char* argv[])
{
    int index = 1;
    while (index < argc)
    {
        std::string thisarg = argv[index];
        std::string thatarg = (index < (argc-1)) ? argv[index+1] : "";

        std::string key;
        std::string value;
        bool usesthat = false;

        if ((thisarg.size() == 0) || (thisarg[0] != '-'))
            Error() << "non-option parameter " << thisarg << " seen (try --help)";
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

        index++;
        if (usesthat && flag->second->hasArgument())
            index++;
    }
        
}

void BoolFlag::set(const std::string& value)
{
	if ((value == "true") || (value == "y"))
		this->value = true;
	else if ((value == "false") || (value == "n"))
		this->value = false;
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

static ActionFlag helpFlag = ActionFlag(
    { "--help", "-h" },
    "Shows the help.",
    doHelp);
