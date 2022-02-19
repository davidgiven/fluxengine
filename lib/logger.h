#ifndef LOGGER_H
#define LOGGER_H

#include "fmt/format.h"

class TrackDataFlux;
class TrackFlux;
class Sector;

struct DiskContextLogMessage
{
    unsigned cylinder;
    unsigned head;
};

struct SingleReadLogMessage
{
	std::shared_ptr<TrackDataFlux> trackDataFlux;
	std::set<std::shared_ptr<Sector>> sectors;
};

struct TrackReadLogMessage
{
	std::shared_ptr<TrackFlux> track;
};

struct BeginReadOperationLogMessage { };
struct EndReadOperationLogMessage { };
struct BeginWriteOperationLogMessage { };
struct EndWriteOperationLogMessage { };

class TrackFlux;

typedef std::variant<std::string,
	SingleReadLogMessage,
	TrackReadLogMessage,
    DiskContextLogMessage,
    BeginReadOperationLogMessage,
    EndReadOperationLogMessage,
	BeginWriteOperationLogMessage,
	EndWriteOperationLogMessage>
    AnyLogMessage;

class Logger
{
public:
    Logger& operator<<(std::shared_ptr<AnyLogMessage> message);

    template <class T>
    Logger& operator<<(const T& message)
    {
        return *this << std::make_shared<AnyLogMessage>(message);
    }
};

#endif
