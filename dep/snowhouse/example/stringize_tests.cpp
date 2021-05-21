#include "tests.h"

using namespace snowhouse;

namespace
{
  // No overload for operator<<(std::ostream&) or specialization of snowhouse::Stringizer
  struct WithoutStreamOperator
  {
    explicit WithoutStreamOperator(int id)
        : m_id(id)
    {
    }

    bool operator==(const WithoutStreamOperator& rhs) const
    {
      return m_id == rhs.m_id;
    }

    int m_id;
  };

  // Has operator<<(std::ostream&)
  struct WithStreamOperator : public WithoutStreamOperator
  {
    explicit WithStreamOperator(int id)
        : WithoutStreamOperator(id)
    {
    }
  };

  std::ostream& operator<<(std::ostream& stream, const WithStreamOperator& a)
  {
    stream << a.m_id;
    return stream;
  }

  // Has no operator<<(std::ostream&), but a specialization of snowhouse::Stringizer
  struct WithoutStreamOperatorButWithStringizer : public WithoutStreamOperator
  {
    explicit WithoutStreamOperatorButWithStringizer(int id)
        : WithoutStreamOperator(id)
    {
    }
  };
}

namespace snowhouse
{
  template<>
  struct Stringizer<WithoutStreamOperatorButWithStringizer>
  {
    static std::string ToString(const WithoutStreamOperatorButWithStringizer& value)
    {
      return snowhouse::Stringize(value.m_id);
    }
  };
}

void StringizeTests()
{
  describe("Stringize");

  it("handles types with stream operators");
  {
    WithStreamOperator a(12);
    WithStreamOperator b(13);
    AssertTestFails(AssertThat(a, Is().EqualTo(b)), "Expected: equal to 13\nActual: 12");
  }

  it("handles types without stream operators");
  {
    WithoutStreamOperator a(12);
    WithoutStreamOperator b(13);
    AssertTestFails(AssertThat(a, Is().EqualTo(b)), "Expected: equal to [unsupported type]\nActual: [unsupported type]");
  }

  it("handles types with traits");
  {
    WithoutStreamOperatorButWithStringizer a(12);
    WithoutStreamOperatorButWithStringizer b(13);
    AssertTestFails(AssertThat(a, Is().EqualTo(b)), "Expected: equal to 13\nActual: 12");
  }

  it("provides bools as true or false");
  {
    AssertTestFails(AssertThat(false, Is().True()), "Expected: true\nActual: false");
  }

  it("provides strings in quotation marks");
  {
    AssertTestFails(AssertThat("wrong", Is().EqualTo("right")), "Expected: equal to \"right\"\nActual: \"wrong\"");
  }

  describe("Stringize expression templates");

  it("handles types with stream operators");
  {
    WithStreamOperator a(12);
    WithStreamOperator b(13);
    AssertTestFails(AssertThat(a, Equals(b)), "Expected: equal to 13\nActual: 12");
  }

  it("handles types without stream operators");
  {
    WithoutStreamOperator a(12);
    WithoutStreamOperator b(13);
    AssertTestFails(AssertThat(a, Equals(b)), "Expected: equal to [unsupported type]\nActual: [unsupported type]");
  }

  it("handles types with traits");
  {
    WithoutStreamOperatorButWithStringizer a(12);
    WithoutStreamOperatorButWithStringizer b(13);
    AssertTestFails(AssertThat(a, Equals(b)), "Expected: equal to 13\nActual: 12");
  }

  it("provides bools as true or false");
  {
    AssertTestFails(AssertThat(true, IsFalse()), "Expected: false\nActual: true");
  }

  it("provides strings in quotation marks");
  {
    AssertTestFails(AssertThat("wrong", Equals("right")), "Expected: equal to \"right\"\nActual: \"wrong\"");
  }
}
