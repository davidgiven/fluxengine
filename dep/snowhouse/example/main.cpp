#include "tests.h"

using namespace snowhouse;

void BooleanOperators();
void BasicAssertions();
void ContainerConstraints();
void CustomMatchers();
void ExceptionTests();
void ExpressionErrorHandling();
void MapTests();
void OperatorTests();
void SequenceContainerTests();
void StringLineTests();
void StringTests();
void StringizeTests();

int main()
{
  std::cout << "Spec for Snowhouse " SNOWHOUSE_VERSION << std::endl;

  try
  {
    BasicAssertions();
    BooleanOperators();
    ContainerConstraints();
    CustomMatchers();
    ExceptionTests();
    ExpressionErrorHandling();
    MapTests();
    OperatorTests();
    SequenceContainerTests();
    StringLineTests();
    StringTests();
    StringizeTests();
  }
  catch (const AssertionException& e)
  {
    std::cout << "Tests failed!" << std::endl;
    std::cout << e.what() << std::endl;
    return 1;
  }

  return 0;
}
