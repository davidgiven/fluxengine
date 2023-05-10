#ifndef PROTO_H
#define PROTO_H

#include <google/protobuf/message.h>
#include "lib/config.pb.h"

extern void setRange(RangeProto* range, const std::string& data);

typedef std::pair<google::protobuf::Message*,
    const google::protobuf::FieldDescriptor*>
    ProtoField;

extern ProtoField resolveProtoPath(
    google::protobuf::Message* message, const std::string& path);
extern void setProtoFieldFromString(
    ProtoField& protoField, const std::string& value);
extern void setProtoByString(google::protobuf::Message* message,
    const std::string& path,
    const std::string& value);

extern std::set<unsigned> iterate(const RangeProto& range);
extern std::set<unsigned> iterate(unsigned start, unsigned count);

extern std::map<std::string, const google::protobuf::FieldDescriptor*>
findAllProtoFields(google::protobuf::Message* message);

extern ConfigProto parseConfigBytes(const std::string_view& bytes);

extern const std::map<std::string, const ConfigProto*> formats;

extern ConfigProto& globalConfigProto();

#endif
