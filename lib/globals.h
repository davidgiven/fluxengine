#ifndef GLOBALS_H
#define GLOBALS_H

#include <stddef.h>
#include <functional>
#include <optional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <cassert>
#include <climits>

#if defined(_WIN32) || defined(__WIN32__)
#include <direct.h>
#define mkdir(A, B) _mkdir(A)
#endif

template <class T>
static inline std::vector<T> vector_of(T item)
{ return std::vector<T> { item }; }

typedef double nanoseconds_t;
class Bytes;

extern double getCurrentTime();
extern void hexdump(std::ostream& stream, const Bytes& bytes);
extern void hexdumpForSrp16(std::ostream& stream, const Bytes& bytes);

class Error
{
public:
    [[ noreturn ]] ~Error()
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

extern const std::map<std::string, std::string> formats;

#endif
