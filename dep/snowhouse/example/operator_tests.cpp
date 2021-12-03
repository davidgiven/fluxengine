#include "tests.h"

using namespace snowhouse;

void OperatorTests()
{
  describe("Operators");

  it("handles &&");
  {
    AssertThat(5, IsLessThan(6) && IsGreaterThan(4));
  }

  it("handles And()");
  {
    AssertThat(5, Is().LessThan(6).And().GreaterThan(4));
  }

  it("handles failing &&");
  {
    AssertTestFails(AssertThat(5, IsLessThan(7) && IsGreaterThan(5)),
        "less than 7 and greater than 5");
  }

  it("handles failing And()");
  {
    AssertTestFails(AssertThat(5, Is().LessThan(7).And().GreaterThan(5)),
        "less than 7 and greater than 5");
  }

  it("handles Or()");
  {
    AssertThat(12, Is().LessThan(7).Or().GreaterThan(5));
  }

  it("handles ||");
  {
    AssertThat(12, IsLessThan(7) || IsGreaterThan(5));
  }

  it("handles failing Or()");
  {
    AssertTestFails(AssertThat(67, Is().LessThan(12).Or().GreaterThan(99)),
        "less than 12 or greater than 99");
  }

  it("handles failing ||");
  {
    AssertTestFails(AssertThat(67, IsLessThan(12) || IsGreaterThan(99)),
        "less than 12 or greater than 99");
  }

  it("handles Not()");
  {
    AssertThat(5, Is().Not().EqualTo(4));
  }

  it("handles !");
  {
    AssertThat(5, !Equals(4));
  }

  it("handles failing Not()");
  {
    AssertTestFails(AssertThat(12, Is().Not().EqualTo(12)), "not equal to 12");
  }

  it("handles failing !");
  {
    AssertTestFails(AssertThat(12, !Equals(12)), "not equal to 12");
  }

  it("handles Not() for strings");
  {
    AssertThat("joakim", Is().Not().EqualTo("harry"));
  }

  it("handles failing ! for strings");
  {
    AssertThat("joakim", !Equals("harry"));
  }

  it("handles both left and right associative operators");
  {
    AssertThat(5, Is().GreaterThan(4).And().Not().LessThan(3));
  }

  it("handles both left and right associative operators expression templates");
  {
    AssertThat(5, IsGreaterThan(4) && !IsLessThan(3));
  }

  it("yields error on malformed expression");
  {
    AssertTestFails(AssertThat(4, Is().Not()),
        "The expression contains a \"not\" operator without any operand");
  }

  it("handles failing EqualsWithDelta() when larger than delta");
  {
    AssertTestFails(AssertThat(3.9, EqualsWithDelta(3, 0.5)),
        "Expected: equal to 3 (+/- 0.5)");
  }

  it("handles failing EqualsWithDelta() when less than delta");
  {
    AssertTestFails(AssertThat(2.49, EqualsWithDelta(3, 0.5)),
        "Expected: equal to 3 (+/- 0.5)");
  }

  it("handles EqualsWithDelta()");
  {
    AssertThat(2, EqualsWithDelta(1.9, 0.1));
  }

  it("handles failing Is().EqualToWithDelta() when larger than delta");
  {
    AssertTestFails(AssertThat(3.9, Is().EqualToWithDelta(3, 0.5)),
        "Expected: equal to 3 (+/- 0.5)");
  }

  it("handles failing Is().EqualToWithDelta() when less than delta");
  {
    AssertTestFails(AssertThat(2.49, Is().EqualToWithDelta(3, 0.5)),
        "Expected: equal to 3 (+/- 0.5)");
  }

  it("handles Is().EqualToWithDelta()");
  {
    AssertThat(2, Is().EqualToWithDelta(1.9, 0.1));
  }
}
