#ifndef LOGGER_H
#define LOGGER_H

#include "fmt/format.h"

class DiskFlux;
class TrackDataFlux;
class TrackFlux;
class Sector;

struct ErrorLogMessage
{
    std::string message;
};

struct EmergencyStopMessage
{
};

struct BeginSpeedOperationLogMessage
{
};

struct EndSpeedOperationLogMessage
{
    nanoseconds_t rotationalPeriod;
};

struct TrackReadLogMessage
{
    std::shared_ptr<const TrackFlux> track;
};

struct DiskReadLogMessage
{
    std::shared_ptr<const DiskFlux> disk;
};

struct BeginReadOperationLogMessage
{
    unsigned track;
    unsigned head;
};

struct EndReadOperationLogMessage
{
    std::shared_ptr<const TrackDataFlux> trackDataFlux;
    std::set<std::shared_ptr<const Sector>> sectors;
};

struct BeginWriteOperationLogMessage
{
    unsigned track;
    unsigned head;
};

struct EndWriteOperationLogMessage
{
};

struct BeginOperationLogMessage
{
    std::string message;
};

struct EndOperationLogMessage
{
    std::string message;
};

struct OperationProgressLogMessage
{
    unsigned progress;
};

class TrackFlux;

typedef std::variant<std::string,
    ErrorLogMessage,
    EmergencyStopMessage,
    TrackReadLogMessage,
    DiskReadLogMessage,
    BeginSpeedOperationLogMessage,
    EndSpeedOperationLogMessage,
    BeginReadOperationLogMessage,
    EndReadOperationLogMessage,
    BeginWriteOperationLogMessage,
    EndWriteOperationLogMessage,
    BeginOperationLogMessage,
    EndOperationLogMessage,
    OperationProgressLogMessage>
    AnyLogMessage;

template <class T>
inline void log(const T& message)
{
    log(std::make_shared<const AnyLogMessage>(message));
}

extern void log(std::shared_ptr<const AnyLogMessage> message);

template <typename... Args>
inline void log(fmt::string_view fstr, const Args&... args)
{
    log(fmt::format(fstr, args...));
}

namespace Logger
{
    extern void setLogger(
        std::function<void(std::shared_ptr<const AnyLogMessage>)> cb);

    extern std::string toString(const AnyLogMessage&);
}

#endif
