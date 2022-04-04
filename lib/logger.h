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

class TrackFlux;

typedef std::variant<
	std::string,
	ErrorLogMessage,
    TrackReadLogMessage,
	DiskReadLogMessage,
    BeginSpeedOperationLogMessage,
    EndSpeedOperationLogMessage,
    BeginReadOperationLogMessage,
    EndReadOperationLogMessage,
    BeginWriteOperationLogMessage,
    EndWriteOperationLogMessage>
    AnyLogMessage;

class Logger
{
public:
    Logger& operator<<(std::shared_ptr<const AnyLogMessage> message);

    template <class T>
    Logger& operator<<(const T& message)
    {
        return *this << std::make_shared<const AnyLogMessage>(message);
    }

    static void setLogger(
        std::function<void(std::shared_ptr<const AnyLogMessage>)> cb);

    static std::string toString(const AnyLogMessage&);
    static void textLogger(std::shared_ptr<const AnyLogMessage>);
};

#endif
