#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/core/logger.h"

static bool indented = false;

static std::function<void(const AnyLogMessage*)> currentLoggerCb =
    [](const auto& message)
{
    static auto r = LogRenderer::create(std::cout);
    r->add(message);
};

void logImpl(const AnyLogMessage* message)
{
    currentLoggerCb(message);
}

void log(const char* m)
{
    currentLoggerCb(new AnyLogMessage(new std::string(m)));
}

void Logger::setLogger(std::function<void(const AnyLogMessage*)> cb)
{
    currentLoggerCb = cb;
}

void renderLogMessage(LogRenderer& r, const ErrorLogMessage* msg)
{
    r.newline().add("Error:").add(msg->message).newline();
}

void renderLogMessage(LogRenderer& r, const EmergencyStopMessage* msg)
{
    r.newline().add("Stop!").newline();
}

void renderLogMessage(LogRenderer& r, const std::string* msg)
{
    r.newline().add(*msg).newline();
}

LogRenderer& LogRenderer::add(const AnyLogMessage* message)
{
    std::visit(
        [&](const auto* arg)
        {
            renderLogMessage(*this, arg);
        },
        *message);

    return *this;
}
