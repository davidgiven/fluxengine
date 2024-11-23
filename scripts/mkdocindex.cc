#include "lib/core/globals.h"
#include "lib/config/proto.h"
#include "lib/config/flags.h"
#include "fmt/format.h"

extern const std::map<std::string, const ConfigProto*> formats;

static std::string supportStatus(SupportStatus status)
{
    switch (status)
    {
        case SupportStatus::DINOSAUR:
            return "🦖";

        case SupportStatus::UNICORN:
            return "🦄";

        case SupportStatus::UNSUPPORTED:
            return "";
    }

	return "";
}

int main(int argc, const char* argv[])
{
    fmt::print("<!-- FORMATSSTART -->\n");
    fmt::print(
        "<!-- This section is automatically generated. Do not edit. -->\n");
    fmt::print("\n");
    fmt::print("| Profile | Format | Read? | Write? | Filesystem? |\n");
    fmt::print("|:--------|:-------|:-----:|:------:|:------------|\n");

    for (auto [name, config] : formats)
    {
        if (config->is_extension())
            continue;

        std::set<std::string> filesystems;
        auto addFilesystem = [&](const FilesystemProto& fs)
        {
            if (fs.type() != FilesystemProto::NOT_SET)
            {
                const auto* descriptor =
                    FilesystemProto::FilesystemType_descriptor();
                auto name = descriptor->FindValueByNumber(fs.type())->name();

                filesystems.insert(name);
            }
        };

        addFilesystem(config->filesystem());
        for (const auto& group : config->option_group())
        {
            for (const auto& option : group.option())
                addFilesystem(option.config().filesystem());
        }
        for (const auto& option : config->option())
            addFilesystem(option.config().filesystem());

        std::stringstream ss;
        for (auto& fs : filesystems)
            ss << fs << " ";

        fmt::print("| [`{0}`](doc/disk-{0}.md) | {1} | {2} | {3} | {4} |\n",
            name,
            config->shortname() + ": " + config->comment(),
            supportStatus(config->read_support_status()),
            supportStatus(config->write_support_status()),
            ss.str());
    }

    std::cout << "{: .datatable }\n\n";
    return 0;
}
