#include "tests.h"

using namespace snowhouse;

void BooleanOperators()
{
  describe("Boolean operators");

  it("handles IsFalse()");
  {
    AssertThat(false, IsFalse());
  }

  it("handles failing IsFalse()");
  {
    AssertTestFails(AssertThat(true, IsFalse()), "Expected: false");
  }

  it("handles IsTrue()");
  {
    AssertThat(true, IsTrue());
  }

  it("handles failing IsTrue()");
  {
    AssertTestFails(AssertThat(false, IsTrue()), "Expected: true");
  }

  it("handles Is().True()");
  {
    AssertThat(true, Is().True());
    AssertTestFails(AssertThat(false, Is().True()), "Expected: true");
  }

  it("handles Is().False()");
  {
    AssertThat(false, Is().False());
    AssertTestFails(AssertThat(true, Is().False()), "Expected: false");
  }

  it("treats assert without constraint as boolean constrains");
  {
    Assert::That(true);
  }
}
