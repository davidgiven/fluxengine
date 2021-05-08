//          Copyright Joakim Karlsson & Kim Gräsman 2010-2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "tests.h"

using namespace snowhouse;

struct IsEvenNumberNoStreamOperator
{
  bool Matches(const int actual) const
  {
    return (actual % 2) == 0;
  }
};

struct IsEvenNumberWithStreamOperator
{
  bool Matches(const int actual) const
  {
    return (actual % 2) == 0;
  }

  friend std::ostream& operator<<(std::ostream& stm,
      const IsEvenNumberWithStreamOperator&);
};

std::ostream& operator<<(std::ostream& stm,
    const IsEvenNumberWithStreamOperator&)
{
  stm << "An even number";
  return stm;
}

void CustomMatchers()
{
  describe("Custom matchers");

  it("handles custom matcher");
  {
    AssertThat(2, Fulfills(IsEvenNumberNoStreamOperator()));
  }

  it("handles custom matcher with fluent");
  {
    AssertThat(2, Is().Fulfilling(IsEvenNumberNoStreamOperator()));
  }

  it("outputs correct message when fails");
  {
    AssertTestFails(AssertThat(3, Fulfills(IsEvenNumberNoStreamOperator())),
        "Expected: [unsupported type]\nActual: 3");
  }

  it("outputs correct message when custom stream operator is defined");
  {
    AssertTestFails(AssertThat(3, Fulfills(IsEvenNumberWithStreamOperator())),
        "Expected: An even number\nActual: 3");
  }
}
