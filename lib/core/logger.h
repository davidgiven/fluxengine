#ifndef LOGGER_H
#define LOGGER_H

#include "fmt/format.h"
#include <type_traits>

class DiskFlux;
class TrackDataFlux;
class TrackFlux;
class Sector;
class LogRenderer;

struct ErrorLogMessage;
struct EmergencyStopMessage;
struct BeginSpeedOperationLogMessage;
struct EndSpeedOperationLogMessage;
struct TrackReadLogMessage;
struct DiskReadLogMessage;
struct BeginReadOperationLogMessage;
struct EndReadOperationLogMessage;
struct BeginWriteOperationLogMessage;
struct EndWriteOperationLogMessage;
struct BeginOperationLogMessage;
struct EndOperationLogMessage;
struct OperationProgressLogMessage;
struct OptionLogMessage;

struct ErrorLogMessage
{
    std::string message;
};

struct EmergencyStopMessage
{
};

extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const ErrorLogMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const EmergencyStopMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const TrackReadLogMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const DiskReadLogMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const BeginSpeedOperationLogMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const EndSpeedOperationLogMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const BeginReadOperationLogMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const EndReadOperationLogMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const BeginWriteOperationLogMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const EndWriteOperationLogMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const BeginOperationLogMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const EndOperationLogMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const OperationProgressLogMessage> m);
extern void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const OptionLogMessage> m);

typedef std::variant<std::shared_ptr<const std::string>,
    std::shared_ptr<const ErrorLogMessage>,
    std::shared_ptr<const EmergencyStopMessage>,
    std::shared_ptr<const TrackReadLogMessage>,
    std::shared_ptr<const DiskReadLogMessage>,
    std::shared_ptr<const BeginSpeedOperationLogMessage>,
    std::shared_ptr<const EndSpeedOperationLogMessage>,
    std::shared_ptr<const BeginReadOperationLogMessage>,
    std::shared_ptr<const EndReadOperationLogMessage>,
    std::shared_ptr<const BeginWriteOperationLogMessage>,
    std::shared_ptr<const EndWriteOperationLogMessage>,
    std::shared_ptr<const BeginOperationLogMessage>,
    std::shared_ptr<const EndOperationLogMessage>,
    std::shared_ptr<const OperationProgressLogMessage>,
    std::shared_ptr<const OptionLogMessage>>
    AnyLogMessage;

extern void log(const char* ptr);
extern void log(const AnyLogMessage& message);

template <class T>
inline void log(const T& message)
{
    log(AnyLogMessage(std::make_shared<T>(message)));
}

template <typename... Args>
inline void log(fmt::string_view fstr, const Args&... args)
{
    log(fmt::format(fstr, args...));
}

class LogRenderer
{
public:
    static std::unique_ptr<LogRenderer> create(std::ostream& stream);
    virtual ~LogRenderer() {}

public:
    LogRenderer& add(const AnyLogMessage& message);

public:
    virtual LogRenderer& add(const std::string& m) = 0;
    virtual LogRenderer& comma() = 0;
    virtual LogRenderer& header(const std::string& h) = 0;
    virtual LogRenderer& newline() = 0;
};

namespace Logger
{
    extern void setLogger(std::function<void(const AnyLogMessage&)> cb);
}

#endif
