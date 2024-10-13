#include "lib/core/globals.h"
#include "lib/config/proto.h"
#include "lib/config/flags.h"
#include "lib/core/bytes.h"
#include "fmt/format.h"

extern const std::map<std::string, const ConfigProto*> formats;

bool failed = false;

template <typename... Args>
static void configError(fmt::string_view format_str, const Args&... args)
{
    fmt::print(stderr, format_str, args...);
    fputc('\n', stderr);
    failed = true;
}

template <>
struct fmt::formatter<std::vector<std::string>>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::vector<std::string>& vector,
        FormatContext& ctx) const -> decltype(ctx.out())
    {
        auto it = ctx.out();
        for (const auto& s : vector)
        {
            if (s.empty())
                continue;

            it = fmt::format_to(it, "+{}", s);
        }

        return it;
    }
};

static const ConfigProto& findConfig(std::string name)
{
    const auto it = formats.find(name);
    if (it == formats.end())
        configError("{}: couldn't load", name);
    return *it->second;
}

static std::vector<std::vector<std::string>> generateCombinations(
    const std::vector<std::vector<std::string>>& groups)
{

    std::vector<std::vector<std::string>> result;
    std::vector<std::string> current(groups.size());

    std::function<void(int)> recurse = [&](int index)
    {
        if (index == groups.size())
            result.push_back(current);
        else
        {
            auto& group = groups.at(index);
            for (auto& g : group)
            {
                current[index] = g;
                recurse(index + 1);
            }
        }
    };

    if (!groups.empty())
        recurse(0);

    return result;
}

static void validateConfigWithOptions(std::string baseConfigName,
    const std::vector<std::string>& options,
    const ConfigProto& config)
{
    /* All configs must have a tpi. */

    if (!config.layout().has_format_type())
        configError("{}{}: no layout.format_type set", baseConfigName, options);
}

static void validateToplevelConfig(std::string name)
{
    ConfigProto config = findConfig(name);

    /* Don't test extension configs. */

    if (config.is_extension())
        return;

    /* Collate options. */

    std::map<std::string, const OptionProto*> optionProtos;
    std::vector<std::vector<std::string>> optionGroups;
    auto checkOption = [&](const auto& option)
    {
        if (optionProtos.find(option.name()) != optionProtos.end())
            configError("{}: option name '{}' is used more than once",
                name,
                option.name());

        if (!option.has_comment())
            configError("{}: option '{}' has no comment", name, option.name());

        optionProtos[option.name()] = &option;
    };

    for (const auto& group : config.option_group())
    {
        auto& v = optionGroups.emplace_back();
        for (const auto& option : group.option())
        {
            checkOption(option);
            v.push_back(option.name());
        }
    }

    for (const auto& option : config.option())
    {
        checkOption(option);
        optionGroups.emplace_back(std::vector<std::string>{"", option.name()});
    }

    /* For each permutation of options, verify the complete config. */

    auto combinations = generateCombinations(optionGroups);
    if (combinations.empty())
        validateConfigWithOptions(name, {}, config);
    else
        for (const auto& group : combinations)
        {
            ConfigProto configWithOption = config;
            for (const auto& optionName : group)
            {
                if (!optionName.empty())
                    configWithOption.MergeFrom(
                        optionProtos.at(optionName)->config());
            }
            validateConfigWithOptions(name, group, configWithOption);
        }
}

int main(int argc, const char* argv[])
{
    for (auto [name, data] : formats)
        validateToplevelConfig(name);

    return failed;
}
