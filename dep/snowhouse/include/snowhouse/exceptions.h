//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_EXCEPTIONS_H
#define SNOWHOUSE_EXCEPTIONS_H

#include "assert.h"

namespace snowhouse
{
    template <typename ExceptionType>
    struct ExceptionStorage
    {
        static void last_exception(ExceptionType*** e, bool clear = false)
        {
            static ExceptionType* last = nullptr;
            if (clear && last)
            {
                delete last;
                return;
            }

            *e = &last;
            silly_warning_about_unused_arg(e);
        }

        static ExceptionType*** silly_warning_about_unused_arg(
            ExceptionType*** e)
        {
            return e;
        }

        static void store(const ExceptionType& e)
        {
            ExceptionType** last = nullptr;
            last_exception(&last);
            if (*last)
            {
                delete *last;
                *last = nullptr;
            }

            *last = new ExceptionType(e);
        }

        void compiler_thinks_i_am_unused() {}

        ~ExceptionStorage()
        {
            ExceptionType** e = nullptr;
            last_exception(&e);
            if (*e)
            {
                delete *e;
                *e = nullptr;
            }
        }
    };

    template <typename ExceptionType>
    inline ExceptionType& LastException()
    {
        ExceptionType** e = nullptr;
        ExceptionStorage<ExceptionType>::last_exception(&e);
        if (*e == nullptr)
        {
            Assert::Failure("No exception was stored");
        }

        return **e;
    }
}

// clang-format off
#define SNOWHOUSE_CONCAT2(a, b) a##b
#define SNOWHOUSE_CONCAT(a, b) SNOWHOUSE_CONCAT2(a, b)
#define SNOWHOUSE_LINESUFFIX(a) SNOWHOUSE_CONCAT(a, __LINE__)
#define SNOWHOUSE_TEMPVAR(a) SNOWHOUSE_CONCAT(SNOWHOUSE_, SNOWHOUSE_LINESUFFIX(a ## _))

#define SNOWHOUSE_ASSERT_THROWS(EXCEPTION_TYPE, METHOD, FAILURE_HANDLER_TYPE) \
  ::snowhouse::ExceptionStorage<EXCEPTION_TYPE> SNOWHOUSE_TEMPVAR(storage); \
  SNOWHOUSE_TEMPVAR(storage).compiler_thinks_i_am_unused(); \
  bool SNOWHOUSE_TEMPVAR(wrong_exception) = false; \
  bool SNOWHOUSE_TEMPVAR(no_exception) = false; \
  bool SNOWHOUSE_TEMPVAR(more_info) = true; \
  std::string SNOWHOUSE_TEMPVAR(info_string); \
  try \
  { \
    METHOD; \
    SNOWHOUSE_TEMPVAR(no_exception) = true; \
  } \
  catch (const EXCEPTION_TYPE& SNOWHOUSE_TEMPVAR(catched_exception)) \
  { \
    ::snowhouse::ExceptionStorage<EXCEPTION_TYPE>::store(SNOWHOUSE_TEMPVAR(catched_exception)); \
  } \
  catch (...) \
  { \
    SNOWHOUSE_TEMPVAR(wrong_exception) = true; \
    if (auto eptr = std::current_exception()) { \
      try { \
        std::rethrow_exception(eptr); \
      } catch (const std::exception& e) { \
        SNOWHOUSE_TEMPVAR(more_info) = true; \
        SNOWHOUSE_TEMPVAR(info_string) = e.what(); \
      } catch (...) {} \
    } \
  } \
  if (SNOWHOUSE_TEMPVAR(no_exception)) \
  { \
    ::std::ostringstream SNOWHOUSE_TEMPVAR(stm); \
    SNOWHOUSE_TEMPVAR(stm) << "Expected " #EXCEPTION_TYPE ". No exception was thrown."; \
    ::snowhouse::ConfigurableAssert<FAILURE_HANDLER_TYPE>::Failure(SNOWHOUSE_TEMPVAR(stm).str()); \
  } \
  if (SNOWHOUSE_TEMPVAR(wrong_exception)) \
  { \
    ::std::ostringstream SNOWHOUSE_TEMPVAR(stm); \
    SNOWHOUSE_TEMPVAR(stm) << "Expected " #EXCEPTION_TYPE ". Wrong exception was thrown."; \
    if (SNOWHOUSE_TEMPVAR(more_info)) { \
      SNOWHOUSE_TEMPVAR(stm) << " Description of unwanted exception: " << SNOWHOUSE_TEMPVAR(info_string); \
    } \
    ::snowhouse::ConfigurableAssert<FAILURE_HANDLER_TYPE>::Failure(SNOWHOUSE_TEMPVAR(stm).str()); \
  } \
  do {} while (false)

#ifndef SNOWHOUSE_NO_MACROS
# define AssertThrows(EXCEPTION_TYPE, METHOD) \
  SNOWHOUSE_ASSERT_THROWS(EXCEPTION_TYPE, (METHOD), ::snowhouse::DefaultFailureHandler)
#endif
// clang-format on

#endif
