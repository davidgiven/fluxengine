//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_INVALIDEXPRESSIONEXCEPTION_H
#define SNOWHOUSE_INVALIDEXPRESSIONEXCEPTION_H

#include <stdexcept>
#include <string>

namespace snowhouse
{
    struct InvalidExpressionException : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };
}

#endif
