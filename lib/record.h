#ifndef RECORD_H
#define RECORD_H

class Record
{
public:
	Record(nanoseconds_t position, const std::vector<uint8_t>& data):
		position(position), data(data)
	{}

	size_t position; // in bits
	std::vector<uint8_t> data;
};

typedef std::vector<std::unique_ptr<Record>> RecordVector;

#endif

