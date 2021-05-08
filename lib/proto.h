#ifndef PROTO_H
#define PROTO_H

#include <google/protobuf/message.h>

extern void setProtoByString(google::protobuf::Message* message, const std::string& path, const std::string& value);

#endif

