#ifndef GLOBALS_H
#define GLOBALS_H

#define _DEFAULT_SOURCE

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <cassert>

typedef int nanoseconds_t;

extern double getCurrentTime();
extern void hexdump(std::ostream& stream, const std::vector<uint8_t>& buffer);

class Error
{
public:
    ~Error()
    {
        std::cerr << "Error: " << _stream.str() << std::endl;
        exit(1);
    }

    template <typename T>
    Error& operator<<(T&& t)
    {
        _stream << t;
        return *this;
    }

private:
    std::stringstream _stream;
};

#endif
