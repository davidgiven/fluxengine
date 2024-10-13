#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/core/logger.h"

static bool indented = false;

static std::function<void(const AnyLogMessage&)> loggerImpl =
    [](const auto& message)
{
    static auto r = LogRenderer::create(std::cout);
    r->add(message);
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

LogRenderer& LogRenderer::add(const AnyLogMessage& message)
{
    std::visit(
        [&](const auto& arg)
        {
            renderLogMessage(*this, arg);
        },
        message);

    return *this;
}
