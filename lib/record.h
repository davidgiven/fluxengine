#ifndef RECORD_H
#define RECORD_H

class RawRecord
{
public:
	RawRecord() {}

	RawRecord(nanoseconds_t position, nanoseconds_t clock, const Bytes& bytes):
		position(position),
		clock(clock),
		bytes(bytes)
	{}

	nanoseconds_t position;
	nanoseconds_t clock;
	Bytes bytes;
};

#endif

