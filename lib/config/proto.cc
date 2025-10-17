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
        auto stem = dmatch[1];
        index = std::stoi(dmatch[2]);

        item = stem;
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
        if (!field->is_repeated() && (index != -1))
            throw ProtoPathNotFoundException(fmt::format(
                "config field '{}[{}]' is indexed, but not repeated",
                item,
                index));

        if (field->is_repeated())
        {
            if (index == -1)
                throw ProtoPathNotFoundException(fmt::format(
                    "config field '{}' is repeated and must be indexed", item));
            while (reflection->FieldSize(*message, field) <= index)
                reflection->AddMessage(message, field);

            message = reflection->MutableRepeatedMessage(message, field, index);
        }
        else
        {
            if (!create && !reflection->HasField(*message, field))
                throw ProtoPathNotFoundException(fmt::format(
                    "could not find config field '{}'", field->name()));
            message = reflection->MutableMessage(message, field);
        }

        descriptor = message->GetDescriptor();
    }

    int index = splitIndexedField(trailing);
    const auto* field = descriptor->FindFieldByName(trailing);
    if (!field)
        fail();

    return ProtoField(path, message, field, index);
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

void ProtoField::set(const std::string& value)
{
    const auto* reflection = _message->GetReflection();
    if (_field->is_repeated())
    {
        if (_index == -1)
            error("field '{}' is repeated but no index is provided");

        switch (_field->type())
        {
            case google::protobuf::FieldDescriptor::TYPE_FLOAT:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<float>(
                        _message, _field),
                    _index,
                    toFloat(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<double>(
                        _message, _field),
                    _index,
                    toDouble(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_INT32:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<int32_t>(
                        _message, _field),
                    _index,
                    (int32_t)toInt64(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_INT64:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<int64_t>(
                        _message, _field),
                    _index,
                    toInt64(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<uint32_t>(
                        _message, _field),
                    _index,
                    (uint32_t)toUint64(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_UINT64:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<uint64_t>(
                        _message, _field),
                    _index,
                    toUint64(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_STRING:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<std::string>(
                        _message, _field),
                    _index,
                    value);
                break;

            case google::protobuf::FieldDescriptor::TYPE_BOOL:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<bool>(
                        _message, _field),
                    _index,
                    parseBoolean(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_ENUM:
                updateRepeatedField(
                    reflection->GetMutableRepeatedFieldRef<int32_t>(
                        _message, _field),
                    _index,
                    parseEnum(_field, value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
                error("'{}' is a message and can't be directly set",
                    _field->name());

            default:
                error("can't set this config value type");
        }
    }
    else
    {
        if (_index != -1)
            error("field '{}' is not repeated but an index is provided");
        switch (_field->type())
        {
            case google::protobuf::FieldDescriptor::TYPE_FLOAT:
                reflection->SetFloat(_message, _field, toFloat(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
                reflection->SetDouble(_message, _field, toDouble(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_INT32:
                reflection->SetInt32(_message, _field, toInt64(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_INT64:
                reflection->SetInt64(_message, _field, toInt64(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                reflection->SetUInt32(_message, _field, toUint64(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_UINT64:
                reflection->SetUInt64(_message, _field, toUint64(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_STRING:
                reflection->SetString(_message, _field, value);
                break;

            case google::protobuf::FieldDescriptor::TYPE_BOOL:
                reflection->SetBool(_message, _field, parseBoolean(value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_ENUM:
                reflection->SetEnumValue(
                    _message, _field, parseEnum(_field, value));
                break;

            case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
                error("'{}[{}]' is a message and can't be directly set",
                    _field->name(),
                    _index);

            default:
                error("can't set this config value type");
        }
    }
}

std::string ProtoField::get() const
{
    const auto* reflection = _message->GetReflection();
    if (_field->is_repeated())
    {
        if (_index == -1)
            error("field '{}' is repeated but no index is provided",
                _field->name());

        switch (_field->type())
        {
            case google::protobuf::FieldDescriptor::TYPE_FLOAT:
                return fmt::format("{:g}",
                    reflection->GetRepeatedFloat(*_message, _field, _index));

            case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
                return fmt::format("{:g}",
                    reflection->GetRepeatedDouble(*_message, _field, _index));

            case google::protobuf::FieldDescriptor::TYPE_INT32:
                return std::to_string(
                    reflection->GetRepeatedInt32(*_message, _field, _index));

            case google::protobuf::FieldDescriptor::TYPE_INT64:
                return std::to_string(
                    reflection->GetRepeatedInt64(*_message, _field, _index));

            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                return std::to_string(
                    reflection->GetRepeatedUInt32(*_message, _field, _index));

            case google::protobuf::FieldDescriptor::TYPE_UINT64:
                return std::to_string(
                    reflection->GetRepeatedUInt64(*_message, _field, _index));

            case google::protobuf::FieldDescriptor::TYPE_STRING:
                return reflection->GetRepeatedString(*_message, _field, _index);

            case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
                error("'{}' is a message and can't be directly fetched",
                    _field->name());

            default:
                error("unknown field type when fetching repeated field '{}'",
                    _field->name());
        }
    }
    else
    {
        if (_index != -1)
            error("field '{}' is not repeated but an index is provided",
                _field->name());
        switch (_field->type())
        {
            case google::protobuf::FieldDescriptor::TYPE_FLOAT:
                return fmt::format(
                    "{:g}", reflection->GetFloat(*_message, _field));

            case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
                return fmt::format(
                    "{:g}", reflection->GetDouble(*_message, _field));

            case google::protobuf::FieldDescriptor::TYPE_INT32:
                return std::to_string(reflection->GetInt32(*_message, _field));

            case google::protobuf::FieldDescriptor::TYPE_INT64:
                return std::to_string(reflection->GetInt64(*_message, _field));

            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                return std::to_string(reflection->GetUInt32(*_message, _field));

            case google::protobuf::FieldDescriptor::TYPE_UINT64:
                return std::to_string(reflection->GetUInt64(*_message, _field));

            case google::protobuf::FieldDescriptor::TYPE_STRING:
                return reflection->GetString(*_message, _field);

            case google::protobuf::FieldDescriptor::TYPE_BOOL:
                return std::to_string(reflection->GetBool(*_message, _field));

            case google::protobuf::FieldDescriptor::TYPE_ENUM:
            {
                const auto* enumvalue = reflection->GetEnum(*_message, _field);
                return (std::string)enumvalue->name();
            }

            case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
                error("'{}[{}]' is a message and can't be directly set",
                    _field->name(),
                    _index);

            default:
                error("unknown field type when fetching '{}'", _field->name());
        }
    }
}

google::protobuf::Message* ProtoField::getMessage() const
{
    const auto* reflection = _message->GetReflection();
    if (_field->is_repeated())
    {
        if (_index == -1)
            error("field '{}' is repeated but no index is provided",
                _field->name());

        return reflection->MutableRepeatedMessage(_message, _field, _index);
    }
    else
    {
        if (_index != -1)
            error("field '{}' is not repeated but an index is provided",
                _field->name());

        return reflection->MutableMessage(_message, _field);
    }
}

std::string ProtoField::getBytes() const
{
    const auto* reflection = _message->GetReflection();
    if (_field->is_repeated())
    {
        if (_index == -1)
            error("field '{}' is repeated but no index is provided",
                _field->name());

        return reflection->GetRepeatedString(*_message, _field, _index);
    }
    else
    {
        if (_index != -1)
            error("field '{}' is not repeated but an index is provided",
                _field->name());

        return reflection->GetString(*_message, _field);
    }
}

void setProtoByString(google::protobuf::Message* message,
    const std::string& path,
    const std::string& value)
{
    makeProtoPath(message, path).set(value);
}

std::string getProtoByString(
    google::protobuf::Message* message, const std::string& path)
{
    return findProtoPath(message, path).get();
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
            std::string n = s + (std::string)f->name();

            if (f->is_repeated())
                n += "[]";

            if (shouldRecurse(f))
                recurse(f->message_type(), n + ".");

            fields[n] = f;
        }
    };

    recurse(descriptor, "");
    return fields;
}

std::vector<ProtoField> findAllProtoFields(
    const google::protobuf::Message* message)
{
    std::vector<ProtoField> allFields;

    std::function<void(const google::protobuf::Message*, const std::string&)>
        recurse = [&](const auto* message, const auto& name)
    {
        const auto* reflection = message->GetReflection();
        std::vector<const google::protobuf::FieldDescriptor*> fields;
        reflection->ListFields(*message, &fields);

        for (const auto* f : fields)
        {
            auto basename = name;
            if (!basename.empty())
                basename += '.';
            basename += f->name();

            if (f->is_repeated())
            {
                for (int i = 0; i < reflection->FieldSize(*message, f); i++)
                {
                    const auto n = fmt::format("{}[{}]", basename, i);
                    if (shouldRecurse(f))
                        recurse(
                            &reflection->GetRepeatedMessage(*message, f, i), n);
                    else
                        allFields.push_back(ProtoField(
                            n, (google::protobuf::Message*)message, f, i));
                }
            }
            else
            {
                if (shouldRecurse(f))
                    recurse(&reflection->GetMessage(*message, f), basename);
                else
                    allFields.push_back(ProtoField(
                        basename, (google::protobuf::Message*)message, f));
            }
        }
    };

    recurse(message, "");
    return allFields;
}

std::string renderProtoAsConfig(const google::protobuf::Message* message)
{
    auto allFields = findAllProtoFields(message);
    std::stringstream ss;
    for (const auto& field : allFields)
        ss << field.path() << "=" << field.get() << "\n";
    return ss.str();
}
