#include "tests.h"

using namespace snowhouse;

void ExpressionErrorHandling()
{
  describe("Expression error handling");

  std::vector<int> collection;
  collection.push_back(1);
  collection.push_back(2);
  collection.push_back(3);

  it("reports an invalid All() properly");
  {
    AssertTestFails(AssertThat(collection, Has().All()),
        "The expression after \"all\" operator does not yield any result");
  }

  it("reports an invalid AtLeast() properly");
  {
    AssertTestFails(AssertThat(collection, Has().AtLeast(2)),
        "The expression after \"at least 2\" operator does not yield any result");
  }
}
