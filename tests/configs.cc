#include "lib/globals.h"
#include "lib/proto.h"
#include "lib/flags.h"
#include "lib/bytes.h"
#include <fmt/format.h>

extern const std::map<std::string, std::string> formats;

bool failed = false;

template <typename... Args>
void error(fmt::string_view format_str, const Args&... args)
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

static ConfigProto findConfig(std::string name)
{
    const auto data = formats.at(name);
    ConfigProto config;
    if (!config.ParseFromString(data))
        error("{}: couldn't load", name);
    return config;
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

    if (!config.has_tpi())
        error("{}{}: no tpi set", baseConfigName, options);
}

static void validateToplevelConfig(std::string name)
{
    ConfigProto toplevel = findConfig(name);

    /* Don't test extension configs. */

    if (toplevel.is_extension())
        return;

    /* Apply any includes. */

    ConfigProto config;
    for (const auto& include : toplevel.include())
    {
        ConfigProto included = findConfig(include);
        if (included.include_size() != 0)
            error("{}: extension config contains _includes", include);
        config.MergeFrom(included);
    }
    config.MergeFrom(toplevel);

    /* Collate options. */

    std::map<std::string, const OptionProto*> optionProtos;
    std::vector<std::vector<std::string>> optionGroups;
    auto checkOption = [&](const auto& option)
    {
        if (optionProtos.find(option.name()) != optionProtos.end())
            error("{}: option name '{}' is used more than once",
                name,
                option.name());

        if (!option.has_comment())
            error("{}: option '{}' has no comment", name, option.name());

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
