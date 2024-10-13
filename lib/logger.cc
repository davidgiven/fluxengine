#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/fluxmap.h"
#include "lib/sector.h"
#include "lib/flux.h"
#include "lib/logger.h"

static bool indented = false;

static std::function<void(const AnyLogMessage&)> loggerImpl =
    [](const auto& message)
{
    std::cout << Logger::toString(message) << std::flush;
};

void log(const char* m)
{
    log(std::string(m));
}

void log(const AnyLogMessage& message)
{
    loggerImpl(message);
}

void Logger::setLogger(std::function<void(const AnyLogMessage&)> cb)
{
    loggerImpl = cb;
}

void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const ErrorLogMessage> msg)
{
    r.newline().add("Error:").add(msg->message).newline();
}

void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const EmergencyStopMessage> msg)
{
    r.newline().add("Stop!").newline();
}

void renderLogMessage(LogRenderer& r, std::shared_ptr<const std::string> msg)
{
    r.newline().add(*msg).newline();
}

std::string Logger::toString(const AnyLogMessage& message)
{
    std::stringstream stream;

    auto indent = [&]()
    {
        if (!indented)
            stream << "      ";
        indented = false;
    };

    auto r = LogRenderer::create();
    std::visit(
        [&](const auto& arg)
        {
            renderLogMessage(*r, arg);
        },
        message);

    return stream.str();
}
