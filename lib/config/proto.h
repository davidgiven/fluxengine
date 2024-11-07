#ifndef PROTO_H
#define PROTO_H

#include <google/protobuf/message.h>
#include "lib/config/common.pb.h"
#include "lib/config/config.pb.h"

class ProtoPathNotFoundException : public ErrorException
{
public:
    ProtoPathNotFoundException(const std::string& message):
        ErrorException(message)
    {
    }
};

extern void setRange(RangeProto* range, const std::string& data);

typedef std::pair<google::protobuf::Message*,
    const google::protobuf::FieldDescriptor*>
    ProtoField;

extern ProtoField makeProtoPath(
    google::protobuf::Message* message, const std::string& path);
extern ProtoField findProtoPath(
    google::protobuf::Message* message, const std::string& path);
extern void setProtoFieldFromString(
    ProtoField& protoField, const std::string& value);
extern std::string getProtoFieldValue(ProtoField& protoField);
extern void setProtoByString(google::protobuf::Message* message,
    const std::string& path,
    const std::string& value);
extern std::string getProtoByString(
    google::protobuf::Message* message, const std::string& path);

extern std::set<unsigned> iterate(const RangeProto& range);
extern std::set<unsigned> iterate(unsigned start, unsigned count);

extern std::map<std::string, const google::protobuf::FieldDescriptor*>
findAllProtoFields(google::protobuf::Message* message);

extern ConfigProto parseConfigBytes(const std::string_view& bytes);

extern const std::map<std::string, const ConfigProto*> formats;

extern ConfigProto& globalConfigProto();

#endif
