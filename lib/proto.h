#ifndef PROTO_H
#define PROTO_H

#include <google/protobuf/message.h>
#include "lib/config.pb.h"

typedef std::pair<google::protobuf::Message*, const google::protobuf::FieldDescriptor*> ProtoField;

extern ProtoField resolveProtoPath(google::protobuf::Message* message, const std::string& path);
extern void setProtoFieldFromString(ProtoField& protoField, const std::string& value);
extern void setProtoByString(google::protobuf::Message* message, const std::string& path, const std::string& value);

extern std::set<unsigned> iterate(const RangeProto& range);

extern ConfigProto config;

#endif

