#ifndef BETTERASSERT_H
#define BETTERASSERT_H

#include <assert.h>

namespace std {
    template <class T>
    static std::string to_string(const std::vector<T>& vector)
    {
        std::stringstream s;
        s << "vector{";
        bool first = true;
        for (const T& t : vector)
        {
            if (!first)
                s << ", ";
            first = false;
            s << t;
        }
        s << "}";
        return s.str();
    }
}

#undef assert
#define assertEquals(got, expected) assertImpl(__FILE__, __LINE__, got, expected)

template <class T>
static void assertImpl(const char filename[], int linenumber, T got, T expected)
{
    if (got != expected)
    {
        std::cerr << "assertion failure at "
                  << filename << ":" << linenumber
                  << ": got " << std::to_string(got)
                  << ", expected " << std::to_string(expected)
                  << std::endl;
        abort();
    }
}

#endif

