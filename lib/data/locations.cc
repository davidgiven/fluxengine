#include "lib/core/globals.h"
#include "lib/data/locations.h"
#include "lib/core/logger.h"
#include <lexy/dsl.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy/callback.hpp>
#include <lexy/callback/container.hpp>
#include <lexy/callback/fold.hpp>
#include <lexy/action/parse.hpp>
#include <lexy_ext/report_error.hpp>
#include "fmt/ranges.h"

namespace
{
    struct Member
    {
        unsigned start, end, step;
    };
}

namespace grammar
{
    namespace dsl = lexy::dsl;

    struct member
    {
        static constexpr auto rule = []()
        {
            auto integer = dsl::integer<unsigned>(dsl::digits<>);
            auto step = dsl::lit_c<'x'> >> integer;
            auto end = dsl::lit_c<'-'> >> integer;
            return integer + dsl::opt(end) + dsl::opt(step);
        }();
        static constexpr auto value = lexy::callback<Member>(
            [](unsigned start,
                std::optional<unsigned> end,
                std::optional<unsigned> step)
            {
                return Member{start, end.value_or(start), step.value_or(1)};
            });
    };

    struct members
    {
        static constexpr auto rule =
            dsl::list(dsl::p<member>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = lexy::fold_inplace<std::vector<unsigned>>(
            {},
            [](std::vector<unsigned>& result, const Member& item)
            {
                if (item.start < 0)
                    error("range start {} must be at least 0", item.start);
                if (item.end < item.start)
                    error("range end {} must be at least the start", item.end);
                if (item.step < 1)
                    error("range step {} must be at least one", item.step);

                for (int i = item.start; i <= item.end; i += item.step)
                    result.push_back(i);
            });
    };

    struct ch
    {
        static constexpr auto rule = dsl::lit_c<'c'> + dsl::p<members> +
                                     dsl::lit_c<'h'> + dsl::p<members>;
        static constexpr auto value = lexy::callback<std::vector<CylinderHead>>(
            [](const std::vector<unsigned>& cs, const std::vector<unsigned>& hs)
            {
                std::vector<CylinderHead> result;

                for (unsigned c : cs)
                    for (unsigned h : hs)
                        result.push_back(CylinderHead{c, h});

                return result;
            });
    };

    struct chs
    {
        static constexpr auto rule =
            dsl::list(dsl::p<ch>, dsl::sep(dsl::ascii::space));
        static constexpr auto value =
            lexy::fold_inplace<std::vector<CylinderHead>>({},
                [](std::vector<CylinderHead>& result,
                    const std::vector<CylinderHead>& item)
                {
                    result.insert(result.end(), item.begin(), item.end());
                });
    };
};

struct _error_collector
{
    struct _sink
    {
        typedef std::vector<std::string> return_type;

        return_type _results;

        template <typename Input, typename Reader, typename Tag>
        void operator()(const lexy::error_context<Input>& context,
            const lexy::error<Reader, Tag>& error)
        {
            auto context_location =
                lexy::get_input_location(context.input(), context.position());
            auto location = lexy::get_input_location(
                context.input(), error.position(), context_location.anchor());

            std::string s;
            if constexpr (std::is_same_v<Tag, lexy::expected_literal> ||
                          std::is_same_v<Tag, lexy::expected_keyword>)
                s = fmt::format("expected '{}'", error.string());
            else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>)
                s = fmt::format("expected {}", error.name());
            else
                s = error.message();

            _results.push_back(fmt::format("{} at '{}'", s, error.position()));
        }

        return_type finish() &&
        {
            return _results;
        }
    };

    constexpr auto sink() const
    {
        return _sink{};
    }
};
constexpr auto error_collector = _error_collector();

std::vector<CylinderHead> parseCylinderHeadsString(const std::string& s)
{
    auto input = lexy::string_input(s);
    auto result = lexy::parse<grammar::chs>(input, error_collector);
    if (result.is_error())
    {
        error(fmt::format("track descriptor parse error: {}",
            fmt::join(result.errors(), "; ")));
    }
    return result.value();
}