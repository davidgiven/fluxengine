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
#include <ranges>
#include "fmt/format.h"

#if defined(_WIN32) || defined(__WIN32__)
#include <direct.h>
#define mkdir(A, B) _mkdir(A)
#endif

#define STRINGIFY(a) XSTRINGIFY(a)
#define XSTRINGIFY(a) #a

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

struct ErrorException : public std::exception
{
    ErrorException(const std::string& message): message(message) {}

    const std::string message;

    const char* what() const throw();
    void print() const;
};

struct OutOfRangeException : public ErrorException
{
    OutOfRangeException(const std::string& message): ErrorException(message) {}
};

template <typename... Args>
[[noreturn]] inline void error(fmt::string_view fstr, const Args&... args)
{
    throw ErrorException{fmt::format(fmt::runtime(fstr), args...)};
}

extern void warning(const std::string msg);

template <typename... Args>
inline void warning(fmt::string_view fstr, const Args&... args)
{
    warning(fmt::format(fmt::runtime(fstr), args...));
}

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename K, typename V>
inline const V& findOrDefault(
    const std::map<K, V>& map, const K& key, const V& defaultValue = V())
{
    auto it = map.find(key);
    if (it == map.end())
        return defaultValue;
    return it->second;
}

template <typename K, typename V>
inline const std::optional<V> findOptionally(
    const std::map<K, V>& map, const K& key, const V& defaultValue = V())
{
    auto it = map.find(key);
    if (it == map.end())
        return std::nullopt;
    return std::make_optional(it->second);
}

#endif
