#ifndef PROTO_H
#define PROTO_H

#include <google/protobuf/message.h>
#include "lib/config.pb.h"

extern void setProtoByString(google::protobuf::Message* message, const std::string& path, const std::string& value);

extern std::set<unsigned> iterate(const RangeProto& range);

extern ConfigProto config;

#endif

