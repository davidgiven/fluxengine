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

class RawRecord
{
public:
	RawRecord(
			size_t position,
			const std::vector<bool>::const_iterator start,
			const std::vector<bool>::const_iterator end):
		position(position), data(start, end)
	{}

	size_t position; // in bits
	std::vector<bool> data;
};

typedef std::vector<std::unique_ptr<RawRecord>> RawRecordVector;

#endif

