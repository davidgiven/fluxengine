#include "lib/core/globals.h"
#include "lib/config/proto.h"
#include "lib/config/common.pb.h"
#include <regex>

static ConfigProto config = []()
{
    ConfigProto config;
    config.mutable_drive()->set_drive(0);
    config.mutable_drive()->set_drive(0);
    return config;
}();

ConfigProto& globalConfigProto()
{
    return config;
}

static double toFloat(const std::string& value)
{
    try
    {
        size_t idx;
        float f = std::stof(value, &idx);
        if (value[idx] != '\0')
            throw std::invalid_argument("trailing garbage");
        return f;
    }
    catch (const std::invalid_argument& e)
    {
        error("invalid number '{}'", value);
    }
}

static double toDouble(const std::string& value)
{
    try
    {
        size_t idx;
        double d = std::stod(value, &idx);
        if (value[idx] != '\0')
            throw std::invalid_argument("trailing garbage");
        return d;
    }
    catch (const std::invalid_argument& e)
    {
        error("invalid number '{}'", value);
    }
}

static int64_t toInt64(const std::string& value)
{
    size_t idx;
    int64_t d = std::stoll(value, &idx);
    if (value[idx] != '\0')
        error("invalid number '{}'", value);
    return d;
}

static uint64_t toUint64(const std::string& value)
{
    size_t idx;
    uint64_t d = std::stoull(value, &idx);
    if (value[idx] != '\0')
        error("invalid number '{}'", value);
    return d;
}

void setRange(RangeProto* range, const std::string& data)
{
    static const std::regex DATA_REGEX(
        "([0-9]+)(?:(?:-([0-9]+))|(?:\\+([0-9]+)))?(?:x([0-9]+))?");

    std::smatch dmatch;
    if (!std::regex_match(data, dmatch, DATA_REGEX))
        error("invalid range '{}'", data);

    int start = std::stoi(dmatch[1]);
    range->set_start(start);
    range->set_end(start);
    range->clear_step();
    if (!dmatch[2].str().empty())
        range->set_end(std::stoi(dmatch[2]));
    if (!dmatch[3].str().empty())
        range->set_end(std::stoi(dmatch[3]) - range->start());
    if (!dmatch[4].str().empty())
        range->set_step(std::stoi(dmatch[4]));
}

static ProtoField resolveProtoPath(
    google::protobuf::Message* message, const std::string& path, bool create)
{
    std::string::size_type dot = path.rfind('.');
    std::string leading = (dot == std::string::npos) ? "" : path.substr(0, dot);
    std::string trailing =
        (dot == std::string::npos) ? path : path.substr(dot + 1);

    auto fail = [&]()
    {
        throw ProtoPathNotFoundException(
            fmt::format("no such config field '{}' in '{}'", trailing, path));
    };

    const auto* descriptor = message->GetDescriptor();

    std::string item;
    std::stringstream ss(leading);
    while (std::getline(ss, item, '.'))
    {
        const auto* field = descriptor->FindFieldByName(item);
        if (!field)
            throw ProtoPathNotFoundException(
                fmt::format("no such config field '{}' in '{}'", item, path));
        if (field->type() != google::protobuf::FieldDescriptor::TYPE_MESSAGE)
            throw ProtoPathNotFoundException(fmt::format(
                "config field '{}' in '{}' is not a message", item, path));

        const auto* reflection = message->GetReflection();
        switch (field->label())
        {
            case google::protobuf::FieldDescriptor::LABEL_OPTIONAL:
            case google::protobuf::FieldDescriptor::LABEL_REQUIRED:
                if (!create && !reflection->HasField(*message, field))
                    throw ProtoPathNotFoundException(fmt::format(
                        "could not find config field '{}'", field->name()));
                message = reflection->MutableMessage(message, field);
                break;

            case google::protobuf::FieldDescriptor::LABEL_REPEATED:
                if (reflection->FieldSize(*message, field) == 0)
                {
                    if (create)
                        message = reflection->AddMessage(message, field);
                    else
                        fail();
                }
                else
                    message =
                        reflection->MutableRepeatedMessage(message, field, 0);
                break;

            default:
                error("bad proto label for field '{}' in '{}'", item, path);
        }

        descriptor = message->GetDescriptor();
    }

    const auto* field = descriptor->FindFieldByName(trailing);
    if (!field)
        fail();

    return std::make_pair(message, field);
}

ProtoField makeProtoPath(
    google::protobuf::Message* message, const std::string& path)
{
    return resolveProtoPath(message, path, /* create= */ true);
}

ProtoField findProtoPath(
    google::protobuf::Message* message, const std::string& path)
{
    return resolveProtoPath(message, path, /* create= */ false);
}

void setProtoFieldFromString(ProtoField& protoField, const std::string& value)
{
    google::protobuf::Message* message = protoField.first;
    const google::protobuf::FieldDescriptor* field = protoField.second;

    const auto* reflection = message->GetReflection();
    switch (field->type())
    {
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            reflection->SetFloat(message, field, toFloat(value));
            break;

        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            reflection->SetDouble(message, field, toDouble(value));
            break;

        case google::protobuf::FieldDescriptor::TYPE_INT32:
            reflection->SetInt32(message, field, toInt64(value));
            break;

        case google::protobuf::FieldDescriptor::TYPE_INT64:
            reflection->SetInt64(message, field, toInt64(value));
            break;

        case google::protobuf::FieldDescriptor::TYPE_UINT32:
            reflection->SetUInt32(message, field, toUint64(value));
            break;

        case google::protobuf::FieldDescriptor::TYPE_UINT64:
            reflection->SetUInt64(message, field, toUint64(value));
            break;

        case google::protobuf::FieldDescriptor::TYPE_STRING:
            reflection->SetString(message, field, value);
            break;

        case google::protobuf::FieldDescriptor::TYPE_BOOL:
        {
            static const std::map<std::string, bool> boolvalues = {
                {"false", false},
                {"f",     false},
                {"no",    false},
                {"n",     false},
                {"0",     false},
                {"true",  true },
                {"t",     true },
                {"yes",   true },
                {"y",     true },
                {"1",     true },
            };

            const auto& it = boolvalues.find(value);
            if (it == boolvalues.end())
                error("invalid boolean value");
            reflection->SetBool(message, field, it->second);
            break;
        }

        case google::protobuf::FieldDescriptor::TYPE_ENUM:
        {
            const auto* enumfield = field->enum_type();
            const auto* enumvalue = enumfield->FindValueByName(value);
            if (!enumvalue)
                error("unrecognised enum value '{}'", value);

            reflection->SetEnum(message, field, enumvalue);
            break;
        }

        case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
            if (field->message_type() == RangeProto::descriptor())
            {
                setRange(
                    (RangeProto*)reflection->MutableMessage(message, field),
                    value);
                break;
            }
            if (field->containing_oneof() && value.empty())
            {
                reflection->MutableMessage(message, field);
                break;
            }
            /* fall through */
        default:
            error("can't set this config value type");
    }
}

std::string getProtoFieldValue(ProtoField& protoField)
{
    google::protobuf::Message* message = protoField.first;
    const google::protobuf::FieldDescriptor* field = protoField.second;

    const auto* reflection = message->GetReflection();
    switch (field->type())
    {
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            return fmt::format("{}", reflection->GetFloat(*message, field));

        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            return fmt::format("{}", reflection->GetDouble(*message, field));

        case google::protobuf::FieldDescriptor::TYPE_INT32:
            return std::to_string(reflection->GetInt32(*message, field));

        case google::protobuf::FieldDescriptor::TYPE_INT64:
            return std::to_string(reflection->GetInt64(*message, field));

        case google::protobuf::FieldDescriptor::TYPE_UINT32:
            return std::to_string(reflection->GetUInt32(*message, field));

        case google::protobuf::FieldDescriptor::TYPE_UINT64:
            return std::to_string(reflection->GetUInt64(*message, field));

        case google::protobuf::FieldDescriptor::TYPE_STRING:
            return reflection->GetString(*message, field);

        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            return std::to_string(reflection->GetBool(*message, field));

        case google::protobuf::FieldDescriptor::TYPE_ENUM:
        {
            const auto* enumvalue = reflection->GetEnum(*message, field);
            return enumvalue->name();
        }

        case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
            if (field->message_type() == RangeProto::descriptor())
            {
                const RangeProto* range = dynamic_cast<const RangeProto*>(
                    &reflection->GetMessage(*message, field));
                if (range->step() == 1)
                    return fmt::format("{}-{}", range->start(), range->end());
                else
                    return fmt::format("{}-{}x{}",
                        range->start(),
                        range->end(),
                        range->step());
            }
            error("cannot fetch message value");

        default:
            error("unknown field type when fetching");
    }
}

void setProtoByString(google::protobuf::Message* message,
    const std::string& path,
    const std::string& value)
{
    ProtoField protoField = makeProtoPath(message, path);
    setProtoFieldFromString(protoField, value);
}

std::string getProtoByString(
    google::protobuf::Message* message, const std::string& path)
{
    ProtoField protoField = findProtoPath(message, path);
    return getProtoFieldValue(protoField);
}

std::set<unsigned> iterate(const RangeProto& range)
{
    std::set<unsigned> set;
    int end = range.has_end() ? range.end() : range.start();
    for (unsigned i = range.start(); i <= end; i += range.step())
        set.insert(i);
    return set;
}

std::set<unsigned> iterate(unsigned start, unsigned count)
{
    std::set<unsigned> set;
    for (unsigned i = 0; i < count; i++)
        set.insert(start + i);
    return set;
}

std::map<std::string, const google::protobuf::FieldDescriptor*>
findAllProtoFields(google::protobuf::Message* message)
{
    std::map<std::string, const google::protobuf::FieldDescriptor*> fields;
    const auto* descriptor = message->GetDescriptor();

    std::function<void(const google::protobuf::Descriptor*, const std::string&)>
        recurse = [&](auto* d, const auto& s)
    {
        for (int i = 0; i < d->field_count(); i++)
        {
            const google::protobuf::FieldDescriptor* f = d->field(i);
            std::string n = s + f->name();

            if (f->options().GetExtension(::recurse) &&
                (f->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE))
                recurse(f->message_type(), n + ".");

            fields[n] = f;
        }
    };

    recurse(descriptor, "");
    return fields;
}

ConfigProto parseConfigBytes(const std::string_view& data)
{
    ConfigProto proto;
    if (!proto.ParseFromArray(data.begin(), data.size()))
        error("invalid internal config data");
    return proto;
}
