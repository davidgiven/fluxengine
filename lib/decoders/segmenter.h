#ifndef SEGMENTER_H
#define SEGMENTER_H

#include "record.h"

class AbstractSegmenter
{
public:
    virtual ~AbstractSegmenter() {}

    virtual int recordMatcher(uint64_t fifo) const = 0;

    virtual RawRecordVector extractRecords(std::vector<bool> bits) const;
};

#endif
