#ifndef LOGGER_H
#define LOGGER_H

#include "fmt/format.h"

struct LogMessage
{
	virtual const std::string toString() const = 0;
};

struct DiskContextLogMessage : public LogMessage
{
	unsigned cylinder;
	unsigned head;

	DiskContextLogMessage(unsigned cylinder, unsigned head):
		cylinder(cylinder),
		head(head)
	{}

	const std::string toString() const override;
};

struct StringLogMessage : public LogMessage
{
	const std::string message;

	StringLogMessage(const std::string& message):
		message(message)
	{}

	const std::string toString() const override
	{ return message; }
};

extern void log(std::unique_ptr<LogMessage> message);
extern void logString(fmt::string_view format, fmt::format_args args);

template <typename... Args>
static inline void log(const std::string& format, Args&&... args)
{ logString(format, fmt::make_format_args(args...)); }

template <typename T, typename... Args>
static inline void log(Args&&... args)
{ log(std::make_unique<T>(args...)); }

#endif

