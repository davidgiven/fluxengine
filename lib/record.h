#ifndef RECORD_H
#define RECORD_H

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

