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

typedef std::tuple<google::protobuf::Message*,
    const google::protobuf::FieldDescriptor*, int>
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
findAllPossibleProtoFields(const google::protobuf::Descriptor* descriptor);

extern std::map<std::string, const google::protobuf::FieldDescriptor*>
findAllProtoFields(const google::protobuf::Message& message);

template <class T>
static inline const T parseProtoBytes(const std::string_view& bytes)
{
    T proto;
    if (!proto.ParseFromArray(bytes.begin(), bytes.size()))
        error("invalid internal proto data");
    return proto;
}

extern const std::map<std::string, const ConfigProto*> formats;

extern ConfigProto& globalConfigProto();

#endif
