#include "lib/core/globals.h"
#include "lib/config/proto.h"
#include "lib/config/common.pb.h"
#include "google/protobuf/reflection.h"
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

static float toFloat(const std::string& value)
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

static int splitIndexedField(std::string& item)
{
    static const std::regex INDEX_REGEX("(\\w+)\\[([0-9]+)\\]");
    int index = -1;

    std::smatch dmatch;
    if (std::regex_match(item, dmatch, INDEX_REGEX))
    {
        item = dmatch[1];
        index = std::stoi(dmatch[2]);
    }

    return index;
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
        static const std::regex INDEX_REGEX("(\\w+)\\[([0-9]+)\\]");

        int index = splitIndexedField(item);

        const auto* field = descriptor->FindFieldByName(item);
        if (!field)
            throw ProtoPathNotFoundException(
                fmt::format("no such config field '{}' in '{}'", item, path));
        if (field->type() != google::protobuf::FieldDescriptor::TYPE_MESSAGE)
            throw ProtoPathNotFoundException(fmt::format(
                "config field '{}' in '{}' is not a message", item, path));

        const auto* reflection = message->GetReflection();
        if ((field->label() !=
                google::protobuf::FieldDescriptor::LABEL_REPEATED) &&
            (index != -1))
            throw ProtoPathNotFoundException(fmt::format(
                "config field '{}[{}]' is indexed, but not repeated",
                item,
                index));

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
                if (index == -1)
                    throw ProtoPathNotFoundException(fmt::format(
                        "config field '{}' is repeated and must be indexed",
                        item));
                while (reflection->FieldSize(*message, field) <= index)
                    reflection->AddMessage(message, field);

                message =
                    reflection->MutableRepeatedMessage(message, field, index);
                break;

            default:
                error("bad proto label for field '{}' in '{}'", item, path);
        }

        descriptor = message->GetDescriptor();
    }

    int index = splitIndexedField(trailing);
    const auto* field = descriptor->FindFieldByName(trailing);
    if (!field)
        fail();

    return std::make_tuple(message, field, index);
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

static bool parseBoolean(const std::string& value)
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
    return it->second;
}

static int32_t parseEnum(
    const google::protobuf::FieldDescriptor* field, const std::string& value)
{
    const auto* enumfield = field->enum_type();
    const auto* enumvalue = enumfield->FindValueByName(value);
    if (!enumvalue)
        error("unrecognised enum value '{}'", value);
    return enumvalue->number();
}

template <typename T>
static void updateRepeatedField(
    google::protobuf::MutableRepeatedFieldRef<T> mrfr, int index, T value)
{
    mrfr.Set(index, value);
}

void setProtoFieldFromString(ProtoField& protoField, const std::string& value)
{
    auto& [message, field, index] = protoField;

    const auto* reflection = message->GetReflection();
    if (field->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED)
    {
        if (index == -1)
            error("field '{}' is repeated but no index is provided");

        switch (field->type())
        {
            case google::protobuf::FieldDescriptor::TYPE_FLOAT:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<float>(
                        message, field),
                    index,
                    toFloat(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<double>(
                        message, field),
                    index,
                    toDouble(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_INT32:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<int32_t>(
                        message, field),
                    index,
                    (int32_t)toInt64(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_INT64:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<int64_t>(
                        message, field),
                    index,
                    toInt64(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<uint32_t>(
                        message, field),
                    index,
                    (uint32_t)toUint64(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_UINT64:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<uint64_t>(
                        message, field),
                    index,
                    toUint64(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_STRING:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<std::string>(
                        message, field),
                    index,
                    value);
                break;

            case google::protobuf::FieldDescriptor::TYPE_BOOL:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<bool>(
                        message, field),
                    index,
                    parseBoolean(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_ENUM:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<int32_t>(
                        message, field),
                    index,
                    parseEnum(field, value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
                if (field->message_type() == RangeProto::descriptor())
                {
                    setRange((RangeProto*)reflection->MutableRepeatedMessage(
                                 message, field, index),
                        value);
                    break;
                }
                /* fall through */
            default:
                error("can't set this config value type");
        }
    }
    else
    {
        if (index != -1)
            error("field '{}' is not repeated but an index is provided");
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
                reflection->SetBool(message, field, parseBoolean(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_ENUM:
                reflection->SetEnumValue(message, field, parseEnum(field, value));
                break;

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
}

std::string getProtoFieldValue(ProtoField& protoField)
{
    auto& [message, field, index] = protoField;

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

std::set<unsigned> iterate(unsigned start, unsigned count)
{
    std::set<unsigned> set;
    for (unsigned i = 0; i < count; i++)
        set.insert(start + i);
    return set;
}

static bool shouldRecurse(const google::protobuf::FieldDescriptor* f)
{
    if (f->type() != google::protobuf::FieldDescriptor::TYPE_MESSAGE)
        return false;
    return f->message_type()->options().GetExtension(::recurse);
}

std::map<std::string, const google::protobuf::FieldDescriptor*>
findAllPossibleProtoFields(const google::protobuf::Descriptor* descriptor)
{
    std::map<std::string, const google::protobuf::FieldDescriptor*> fields;

    std::function<void(const google::protobuf::Descriptor*, const std::string&)>
        recurse = [&](auto* d, const auto& s)
    {
        for (int i = 0; i < d->field_count(); i++)
        {
            const google::protobuf::FieldDescriptor* f = d->field(i);
            std::string n = s + f->name();

            if (f->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED)
                n += "[]";

            if (shouldRecurse(f))
                recurse(f->message_type(), n + ".");

            fields[n] = f;
        }
    };

    recurse(descriptor, "");
    return fields;
}

std::map<std::string, const google::protobuf::FieldDescriptor*>
findAllProtoFields(const google::protobuf::Message& message)
{
    std::map<std::string, const google::protobuf::FieldDescriptor*> allFields;

    std::function<void(const google::protobuf::Message&, const std::string&)>
        recurse = [&](auto& message, const auto& name)
    {
        const auto* reflection = message.GetReflection();
        std::vector<const google::protobuf::FieldDescriptor*> fields;
        reflection->ListFields(message, &fields);

        for (const auto* f : fields)
        {
            auto basename = name;
            if (!basename.empty())
                basename += '.';
            basename += f->name();

            if (f->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED)
            {
                for (int i = 0; i < reflection->FieldSize(message, f); i++)
                {
                    const auto n = fmt::format("{}[{}]", basename, i);
                    if (shouldRecurse(f))
                        recurse(
                            reflection->GetRepeatedMessage(message, f, i), n);
                    else
                        allFields[n] = f;
                }
            }
            else
            {
                if (shouldRecurse(f))
                    recurse(reflection->GetMessage(message, f), basename);
                else
                    allFields[basename] = f;
            }
        }
    };

    recurse(message, "");
    return allFields;
}
