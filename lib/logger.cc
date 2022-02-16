#include "globals.h"
#include "fmt/format.h"
#include "logger.h"

const std::string DiskContextLogMessage::toString() const
{
	return fmt::format("{}.{}", cylinder, head);
}

void logString(fmt::string_view format, fmt::format_args args)
{
	log<StringLogMessage>(fmt::vformat(format, args));
}

void log(std::unique_ptr<LogMessage> message)
{
	std::cout << message->toString() << '\n';
}

