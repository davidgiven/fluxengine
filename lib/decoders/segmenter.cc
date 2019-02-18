#include "globals.h"
#include "record.h"
#include "segmenter.h"
#include <fmt/format.h>

RawRecordVector AbstractSegmenter::extractRecords(std::vector<bool> bits) const
{
    RawRecordVector records;
    uint64_t fifo = 0;
    size_t cursor = 0;
    int matchStart = -1;

    auto pushRecord = [&](size_t end)
    {
        if (matchStart == -1)
            return;

        records.push_back(
            std::unique_ptr<RawRecord>(
                new RawRecord(
                    matchStart,
                    bits.begin() + matchStart,
                    bits.begin() + end)
            )
        );
    };

    while (cursor < bits.size())
    {
        fifo = (fifo << 1) | bits[cursor++];
        
        int match = recordMatcher(fifo);
        if (match > 0)
        {
            pushRecord(cursor - match);
            matchStart = cursor - match;
        }
    }
    pushRecord(cursor);
    
    return records;
}
