#include "globals.h"
#include "proto.h"
#include "fmt/format.h"

static double toDouble(const std::string& value)
{
	size_t idx;
	double d = std::stod(value, &idx);
	if (value[idx] != '\0')
		Error() << fmt::format("invalid number '{}'", value);
	return d;
}

static int64_t toInt64(const std::string& value)
{
	size_t idx;
	int64_t d = std::stoll(value, &idx);
	if (value[idx] != '\0')
		Error() << fmt::format("invalid number '{}'", value);
	return d;
}

static uint64_t toUint64(const std::string& value)
{
	size_t idx;
	uint64_t d = std::stoull(value, &idx);
	if (value[idx] != '\0')
		Error() << fmt::format("invalid number '{}'", value);
	return d;
}

void setProtoByString(google::protobuf::Message* message, const std::string& path, const std::string& value)
{
	std::string::size_type dot = path.rfind('.');
	std::string leading = (dot == std::string::npos) ? "" : path.substr(0, dot);
	std::string trailing = (dot == std::string::npos) ? path : path.substr(dot+1);

	const auto* descriptor = message->GetDescriptor();

	std::string item;
    std::stringstream ss(leading);
    while (std::getline(ss, item, '.'))
	{
		const auto* field = descriptor->FindFieldByName(item);
		if (!field)
			Error() << fmt::format("no such config field '{}' in '{}'", item, path);
		if (field->type() != google::protobuf::FieldDescriptor::TYPE_MESSAGE)
			Error() << fmt::format("config field '{}' in '{}' is not a message", item, path);

		const auto* reflection = message->GetReflection();
		message = reflection->MutableMessage(message, field);
		descriptor = message->GetDescriptor();
    }

	const auto* field = descriptor->FindFieldByName(trailing);
	if (!field)
		Error() << fmt::format("no such config field '{}' in '{}'", item, path);

	const auto* reflection = message->GetReflection();
	switch (field->type())
	{
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
	}
}

