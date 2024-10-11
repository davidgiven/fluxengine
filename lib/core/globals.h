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
#include <regex>
#include "fmt/format.h"

#if defined(_WIN32) || defined(__WIN32__)
#include <direct.h>
#define mkdir(A, B) _mkdir(A)
#endif

template <class T>
static inline std::vector<T> vector_of(T item)
{
    return std::vector<T>{item};
}

typedef double nanoseconds_t;
class Bytes;

extern double getCurrentTime();
extern void hexdump(std::ostream& stream, const Bytes& bytes);
extern void hexdumpForSrp16(std::ostream& stream, const Bytes& bytes);

struct ErrorException
{
    ErrorException(const std::string& message): message(message) {}

    const std::string message;

    void print() const;
};

template <typename... Args>
[[noreturn]] inline void error(fmt::string_view fstr, const Args&... args)
{
    throw ErrorException{fmt::format(fstr, args...)};
}

extern void warning(const std::string msg);

template <typename... Args>
inline void warning(fmt::string_view fstr, const Args&... args)
{
    warning(fmt::format(fstr, args...));
}

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

#endif
