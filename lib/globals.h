#ifndef GLOBALS_H
#define GLOBALS_H

#include <stddef.h>
#include <functional>
#include <numeric>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <cassert>
#include <climits>
#include <variant>
#include <optional>

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

struct ErrorException
{
	const std::string message;

	void print() const;
};

class Error
{
public:
	Error()
	{
		_stream << "Error: ";
	}

    [[ noreturn ]] ~Error() noexcept(false)
    {
		throw ErrorException { _stream.str() };
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

template <class... Ts> struct overloaded : Ts...  { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

#endif
