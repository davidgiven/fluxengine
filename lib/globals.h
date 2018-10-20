#ifndef GLOBALS_H
#define GLOBALS_H

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>

typedef int nanoseconds_t;

extern double getCurrentTime();

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
