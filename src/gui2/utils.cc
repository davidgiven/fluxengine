#include "lib/core/globals.h"
#include "lib/config/proto.h"
#include "globals.h"
#include "utils.h"

OptionsMap stringToOptions(const std::string& optionsString)
{
    OptionsMap result;
    for (auto it : std::views::split(optionsString, ' '))
    {
        std::string left(&*it.begin(), std::ranges::distance(it));
        std::string right;
        auto i = left.find('=');
        if (i != std::string::npos)
        {
            right = left.substr(i + 1);
            left = left.substr(0, i);
        }
        result[left] = right;
    }
    return result;
}

std::string optionsToString(const OptionsMap& options)
{
    std::stringstream ss;
    for (auto& it : options)
    {
        if (ss.rdbuf()->in_avail())
            ss << " ";
        ss << it.first;
        if (!it.second.empty())
            ss << "=" << it.second;
    }
    return ss.str();
}
