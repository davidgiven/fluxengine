#include <stdexcept>

#include "tests.h"

using namespace snowhouse;

static void throwRuntimeError()
{
  throw std::runtime_error("This is expected");
}

struct IgnoreErrors
{
  template<typename ExpectedType, typename ActualType>
  static void Handle(const ExpectedType&, const ActualType&, const char*, int)
  {
  }

  static void Handle(const std::string&)
  {
  }
};

void BasicAssertions()
{
  describe("Basic assertions");

  it("handles integer equality");
  {
    AssertThat(5, Is().EqualTo(5));
  }

  it("detects integer inequality");
  {
    AssertTestFails(AssertThat(5, Is().EqualTo(4)), "equal to 4");
  }

  it("detects if Not() fails");
  {
    AssertTestFails(AssertThat(5, Is().Not().EqualTo(5)), "Expected: not equal to 5\nActual: 5\n");
  }

  it("handles strings");
  {
    AssertThat(std::string("joakim"), Is().EqualTo(std::string("joakim")));
  }

  it("handles strings without explicit template specialization");
  {
    AssertThat("kim", Is().EqualTo("kim"));
  }

  it("handles GreaterThan()");
  {
    AssertThat(5, Is().GreaterThan(4));
  }

  it("detects when GreaterThan() fails");
  {
    AssertTestFails(AssertThat(5, Is().GreaterThan(5)),
        "Expected: greater than 5\nActual: 5\n");
  }

  it("handles LessThan()");
  {
    AssertThat(5, Is().LessThan(6));
  }

  it("detects when LessThan() fails");
  {
    AssertTestFails(AssertThat(6, Is().LessThan(5)),
        "Expected: less than 5\nActual: 6\n");
  }

  it("throws explicit failure message");
  {
    AssertTestFails(Assert::Failure("foo"), "foo");
  }

  it("contains location information");
  {
    int line;
    std::string file;

    try
    {
      Assert::That(5, Equals(2), "filename", 32);
    }
    catch (const AssertionException& e)
    {
      line = e.line();
      file = e.file();
    }

    AssertThat(line, Equals(32));
    AssertThat(file, Equals("filename"));
  }

  it("ensures exception is thrown");
  {
    AssertThrows(std::runtime_error, throwRuntimeError());
  }

  it("ignores the error");
  {
    ConfigurableAssert<IgnoreErrors>::That(1, Equals(2));
  }

  describe("Assertion expression templates");

  it("handles integer equality");
  {
    AssertThat(5, Equals(5));
  }

  it("detects integer inequality");
  {
    AssertTestFails(AssertThat(5, Equals(4)), "equal to 4");
  }

  it("detects if !Equals() fails");
  {
    AssertTestFails(AssertThat(5, !Equals(5)),
        "Expected: not equal to 5\nActual: 5\n");
  }

  it("handles strings");
  {
    AssertThat(std::string("joakim"), Equals(std::string("joakim")));
  }

  it("handles strings without explicit template specialization");
  {
    AssertThat("kim", Equals("kim"));
  }

  it("handles IsGreaterThan()");
  {
    AssertThat(5, IsGreaterThan(4));
  }

  it("handles IsGreaterThanOrEqualTo()");
  {
    AssertThat(4, IsGreaterThanOrEqualTo(4));
    AssertThat(5, IsGreaterThanOrEqualTo(4));
  }

  it("detects when IsGreaterThan() fails");
  {
    AssertTestFails(AssertThat(5, IsGreaterThan(5)),
        "Expected: greater than 5\nActual: 5\n");
  }

  it("detects when IsGreaterThanOrEqualTo() fails");
  {
    AssertTestFails(AssertThat(4, IsGreaterThanOrEqualTo(5)),
        "Expected: greater than or equal to 5\nActual: 4\n");
  }

  it("handles IsLessThan()");
  {
    AssertThat(5, IsLessThan(6));
  }

  it("handles IsLessThanOrEqualTo()");
  {
    AssertThat(5, IsLessThanOrEqualTo(6));
    AssertThat(6, IsLessThanOrEqualTo(6));
  }

  it("detects when IsLessThan() fails");
  {
    AssertTestFails(AssertThat(6, IsLessThan(5)),
        "Expected: less than 5\nActual: 6\n");
  }

  it("detects when IsLessThanOrEqualTo() fails");
  {
    AssertTestFails(AssertThat(6, IsLessThanOrEqualTo(5)),
        "Expected: less than or equal to 5\nActual: 6\n");
  }

  it("handles IsNull()");
  {
    AssertThat(nullptr, IsNull());
  }

  it("handles Is().Null()");
  {
    AssertThat(nullptr, Is().Null());
  }

  it("handles !IsNull()");
  {
    int anInt = 0;
    AssertThat(&anInt, !IsNull());
  }

  it("detects when IsNull() fails (real address)");
  {
    int anInt = 0;
    std::ostringstream message;
    message << "Expected: equal to nullptr\nActual: " << &anInt << "\n";
    AssertTestFails(AssertThat(&anInt, IsNull()), message.str());
  }

  it("detects when Is().Null() fails");
  {
    int anInt = 0;
    std::ostringstream message;
    message << "Expected: equal to nullptr\nActual: " << &anInt << "\n";
    AssertTestFails(AssertThat(&anInt, Is().Null()), message.str());
  }

  it("detects when !IsNull() fails (nullptr)");
  {
    std::ostringstream message;
    message << "Expected: not equal to nullptr\nActual: nullptr\n";

    AssertTestFails(AssertThat(nullptr, !IsNull()), message.str());
  }
}
