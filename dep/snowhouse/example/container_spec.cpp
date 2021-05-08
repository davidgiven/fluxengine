//          Copyright Joakim Karlsson & Kim Gräsman 2010-2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <list>

#include "tests.h"

using namespace snowhouse;

struct my_type
{
  explicit my_type(int my_val)
      : my_val_(my_val)
  {
  }

  friend bool operator==(const my_type&, const my_type&);
  friend bool operator!=(const my_type&, const my_type&);
  friend std::ostream& operator<<(std::ostream&, const my_type&);

  int my_val_;
};

bool operator==(const my_type& lhs, const my_type& rhs)
{
  return lhs.my_val_ == rhs.my_val_;
}

bool operator!=(const my_type& lhs, const my_type& rhs)
{
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& stream, const my_type& item)
{
  stream << "(my_type: my_val_=" << item.my_val_ << " )";
  return stream;
}

static bool are_my_types_equal(const my_type& lhs, const my_type& rhs)
{
  return lhs.my_val_ == rhs.my_val_;
}

void ContainerConstraints()
{
  describe("Container constraints");

  it("is able to compare containers of custom types");
  {
    const my_type e[] = {my_type(1), my_type(3)};
    const std::list<my_type> expected(e, e + sizeof(e) / sizeof(e[0]));
    std::list<my_type> my_container_;
    my_container_.push_back(my_type(1));
    my_container_.push_back(my_type(3));

    AssertThat(my_container_, EqualsContainer(expected));
  }

  it("handles failing comparisons");
  {
    const my_type e[] = {my_type(1), my_type(2)};
    const std::list<my_type> expected(e, e + sizeof(e) / sizeof(e[0]));
    std::list<my_type> my_container_;
    my_container_.push_back(my_type(1));
    my_container_.push_back(my_type(3));

    AssertTestFails(AssertThat(my_container_, EqualsContainer(expected)),
        "Expected: [ (my_type: my_val_=1 ), (my_type: my_val_=2 ) ]");
  }

  it("handles comparison with a predicate function");
  {
    const my_type e[] = {my_type(1), my_type(3)};
    const std::list<my_type> expected(e, e + sizeof(e) / sizeof(e[0]));
    std::list<my_type> my_container_;
    my_container_.push_back(my_type(1));
    my_container_.push_back(my_type(3));

    AssertThat(my_container_, EqualsContainer(expected, are_my_types_equal));
    AssertThat(my_container_, Is().EqualToContainer(expected, are_my_types_equal));
  }
}
