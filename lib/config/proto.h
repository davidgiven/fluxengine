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

class ProtoField
{
public:
    ProtoField(const std::string& path,
        google::protobuf::Message* message,
        const google::protobuf::FieldDescriptor* field,
        int index = -1):
        _path(path),
        _message(message),
        _field(field),
        _index(index)
    {
    }

    void set(const std::string& value);
    std::string get() const;
    google::protobuf::Message* getMessage() const;
    std::string getBytes() const;

    bool operator==(const ProtoField& other) const = default;
    std::strong_ordering operator<=>(const ProtoField& other) const = default;

    const std::string& path() const
    {
        return _path;
    }

    const google::protobuf::FieldDescriptor* descriptor() const
    {
        return _field;
    }

private:
    std::string _path;
    google::protobuf::Message* _message;
    const google::protobuf::FieldDescriptor* _field;
    int _index;
};

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

extern std::set<unsigned> iterate(unsigned start, unsigned count);

extern std::map<std::string, const google::protobuf::FieldDescriptor*>
findAllPossibleProtoFields(const google::protobuf::Descriptor* descriptor);

extern std::vector<ProtoField> findAllProtoFields(
    google::protobuf::Message* message);

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
