#ifndef LOGGER_H
#define LOGGER_H

#include "fmt/format.h"
#include <type_traits>

class Disk;
class Track;
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

extern void renderLogMessage(LogRenderer& r, const ErrorLogMessage* m);
extern void renderLogMessage(LogRenderer& r, const EmergencyStopMessage* m);
extern void renderLogMessage(LogRenderer& r, const TrackReadLogMessage* m);
extern void renderLogMessage(LogRenderer& r, const DiskReadLogMessage* m);
extern void renderLogMessage(
    LogRenderer& r, const BeginSpeedOperationLogMessage* m);
extern void renderLogMessage(
    LogRenderer& r, const EndSpeedOperationLogMessage* m);
extern void renderLogMessage(
    LogRenderer& r, const BeginReadOperationLogMessage* m);
extern void renderLogMessage(
    LogRenderer& r, const EndReadOperationLogMessage* m);
extern void renderLogMessage(
    LogRenderer& r, const BeginWriteOperationLogMessage* m);
extern void renderLogMessage(
    LogRenderer& r, const EndWriteOperationLogMessage* m);
extern void renderLogMessage(LogRenderer& r, const BeginOperationLogMessage* m);
extern void renderLogMessage(LogRenderer& r, const EndOperationLogMessage* m);
extern void renderLogMessage(
    LogRenderer& r, const OperationProgressLogMessage* m);
extern void renderLogMessage(LogRenderer& r, const OptionLogMessage* m);

typedef std::variant<const std::string*,
    const ErrorLogMessage*,
    const EmergencyStopMessage*,
    const TrackReadLogMessage*,
    const DiskReadLogMessage*,
    const BeginSpeedOperationLogMessage*,
    const EndSpeedOperationLogMessage*,
    const BeginReadOperationLogMessage*,
    const EndReadOperationLogMessage*,
    const BeginWriteOperationLogMessage*,
    const EndWriteOperationLogMessage*,
    const BeginOperationLogMessage*,
    const EndOperationLogMessage*,
    const OperationProgressLogMessage*,
    const OptionLogMessage*>
    AnyLogMessage;

extern void log(const char* ptr);
extern void logImpl(const AnyLogMessage* message);

template <class T>
inline void log(const T& message)
{
    logImpl(new AnyLogMessage(new T(message)));
}

template <typename... Args>
inline void log(fmt::string_view fstr, const Args&... args)
{
    log(fmt::format(fmt::runtime(fstr), args...));
}

class LogRenderer
{
public:
    static std::unique_ptr<LogRenderer> create(std::ostream& stream);
    virtual ~LogRenderer() {}

public:
    LogRenderer& add(const AnyLogMessage* message);

public:
    virtual LogRenderer& add(const std::string& m) = 0;
    virtual LogRenderer& comma() = 0;
    virtual LogRenderer& header(const std::string& h) = 0;
    virtual LogRenderer& newline() = 0;
};

namespace Logger
{
    extern void setLogger(std::function<void(const AnyLogMessage*)> cb);
}

#endif
