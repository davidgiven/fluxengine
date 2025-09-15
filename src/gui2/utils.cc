#include <imgui.h>
#include "lib/core/globals.h"
#include "lib/config/proto.h"
#include "globals.h"
#include "utils.h"

int maybeDisabledButton(
    const std::string& message, const ImVec2& size, bool isDisabled)
{
    ImGui::BeginDisabled(isDisabled);
    ON_SCOPE_EXIT
    {
        ImGui::EndDisabled();
    };
    return ImGui::Button(message.c_str(), size);
}

std::string shortenString(const std::string& s, size_t len)
{
    if (s.size() < len)
        return s;
    return s.substr(0, len) + "…";
}

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
