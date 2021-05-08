//          Copyright Joakim Karlsson & Kim Gräsman 2010-2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <stdexcept>

#include "tests.h"

using namespace snowhouse;

struct ClassWithExceptions
{
  int LogicError()
  {
    throw std::logic_error("not logical!");
  }

  double RangeError()
  {
    throw std::range_error("range error!");
  }

  void NoError()
  {
  }
};

struct ExpectedException : public std::exception
{
  const char* what() const noexcept override
  {
    return "Description of the exception we expected";
  }
};

void ExceptionTests()
{
  ClassWithExceptions objectUnderTest;

  describe("Exceptions");

  it("detects exceptions");
  {
    AssertThrows(std::exception, objectUnderTest.LogicError());
  }

  it("asserts on LastException()");
  {
    AssertThrows(std::logic_error, objectUnderTest.LogicError());
    AssertThat(LastException<std::logic_error>().what(), Contains("not logical!"));
  }

  it("detects when wrong exception is thrown");
  {
    AssertTestFails(AssertThrows(std::logic_error, objectUnderTest.RangeError()), "Wrong exception");
  }

  it("prints expected exception type when wrong exception is thrown");
  {
    AssertTestFails(AssertThrows(std::logic_error, objectUnderTest.RangeError()), "Expected std::logic_error");
  }

  it("has several exception assertions in same spec");
  {
    AssertThrows(std::logic_error, objectUnderTest.LogicError());
    AssertThat(LastException<std::logic_error>().what(), Contains("not logical!"));

    AssertThrows(std::range_error, objectUnderTest.RangeError());
    AssertThat(LastException<std::range_error>().what(), Contains("range error!"));
  }

  it("has several exception assertion for the same exception in same spec");
  {
    AssertThrows(std::logic_error, objectUnderTest.LogicError());
    AssertThat(LastException<std::logic_error>().what(), Contains("not logical!"));

    AssertThrows(std::logic_error, objectUnderTest.LogicError());
    AssertThat(LastException<std::logic_error>().what(), Contains("not logical!"));
  }

  it("detects when no exception is thrown");
  {
    AssertTestFails(AssertThrows(std::logic_error, objectUnderTest.NoError()), "No exception");
  }

  it("prints expected exception when no exception is thrown");
  {
    AssertTestFails(AssertThrows(std::logic_error, objectUnderTest.NoError()), "Expected std::logic_error");
  }

  it("destroys exceptions when out-of-scope");
  {
    {
      AssertThrows(std::logic_error, objectUnderTest.LogicError());
    }
    AssertThrows(AssertionException, LastException<std::logic_error>());
    AssertThat(LastException<AssertionException>().what(), Contains("No exception was stored"));
  }

  it("prints description of unwanted exception");
  {
    AssertTestFails(AssertThrows(ExpectedException, objectUnderTest.LogicError()), "Expected ExpectedException. Wrong exception was thrown. Description of unwanted exception: not logical!");
  }
}
