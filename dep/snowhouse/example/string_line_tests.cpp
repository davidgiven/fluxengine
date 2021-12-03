#include "tests.h"

using namespace snowhouse;

void StringLineTests()
{
  describe("String lines");

  it("asserts that at least one line in a stream matches");
  {
    AssertThat("First line\n", Has().AtLeast(1).EqualTo("First line"));
  }

  it("detects when assertion fails");
  {
    AssertTestFails(AssertThat("First line\n", Has().AtLeast(1).EqualTo("Second line")), "Expected: at least 1 equal to \"Second line\"");
  }

  it("handles line missing newline");
  {
    AssertThat("First line", Has().AtLeast(1).EqualTo("First line"));
  }

  it("handles several lines");
  {
    std::string lines = "First line\nSecond line";
    AssertThat(lines, Has().Exactly(2).EndingWith("line"));
  }

  it("handles Windows line endings");
  {
    std::string lines = "First line\r\nSecond line\r\nThird line";
    AssertThat(lines, Has().Exactly(3).EndingWith("line"));
  }

  it("matches beginning of lines with Windows line endings");
  {
    std::string lines = "First line\nSecond line\r\nThird line";
    AssertThat(lines, Has().Exactly(1).StartingWith("Second"));
  }

  it("handles empty lines when using Windows line endings");
  {
    std::string lines = "\r\nSecond line\r\n\r\n";
    AssertThat(lines, Has().Exactly(2).OfLength(0));
  }

  it("handles last line missing newline for Windows line endings");
  {
    std::string lines = "First line\r\nSecond line";
    AssertThat(lines, Has().Exactly(2).EndingWith("line"));
  }

  it("handles all empty lines");
  {
    AssertThat("\n\n\n\n\n\n", Has().Exactly(6).OfLength(0));
  }

  it("handles all empty lines with Windows line endings");
  {
    AssertThat("\r\n\r\n\r\n", Has().Exactly(3).OfLength(0));
  }

  describe("StringLineParser");

  it("parses an empty string");
  {
    std::vector<std::string> res;

    StringLineParser::Parse("", res);

    AssertThat(res, HasLength(0));
  }

  it("parses a single line");
  {
    std::vector<std::string> res;

    StringLineParser::Parse("Simple line", res);

    AssertThat(res, HasLength(1));
    AssertThat(res, Has().Exactly(1).EqualTo("Simple line"));
  }

  it("parses two lines");
  {
    std::vector<std::string> res;

    StringLineParser::Parse("One line\nTwo lines", res);

    AssertThat(res, HasLength(2));
    AssertThat(res, Has().Exactly(1).EqualTo("One line"));
    AssertThat(res, Has().Exactly(1).EqualTo("Two lines"));
  }

  it("parses three lines");
  {
    std::vector<std::string> res;

    StringLineParser::Parse("One line\nTwo lines\nThree lines", res);

    AssertThat(res, HasLength(3));
    AssertThat(res, Has().Exactly(1).EqualTo("One line"));
    AssertThat(res, Has().Exactly(1).EqualTo("Two lines"));
    AssertThat(res, Has().Exactly(1).EqualTo("Three lines"));
  }

  it("handles string ending with newline");
  {
    std::vector<std::string> res;
    StringLineParser::Parse("One line\n", res);
    AssertThat(res, HasLength(1));
    AssertThat(res, Has().Exactly(1).EqualTo("One line"));
  }

  it("handles single line with Windows line ending");
  {
    std::vector<std::string> res;
    StringLineParser::Parse("One line\r\n", res);
    AssertThat(res, HasLength(1));
    AssertThat(res, Has().Exactly(1).EqualTo("One line"));
  }

  it("handles two lines with Windows line endings");
  {
    std::vector<std::string> res;
    StringLineParser::Parse("One line\r\nTwo lines", res);
    AssertThat(res, HasLength(2));
    AssertThat(res, Has().Exactly(1).EqualTo("One line"));
    AssertThat(res, Has().Exactly(1).EqualTo("Two lines"));
  }

  it("handles empty line with newline");
  {
    std::vector<std::string> res;
    StringLineParser::Parse("\n", res);
    AssertThat(res, Is().OfLength(1).And().Exactly(1).OfLength(0));
  }

  it("handles two empty lines");
  {
    std::vector<std::string> res;
    StringLineParser::Parse("\n\n", res);
    AssertThat(res, HasLength(2));
    AssertThat(res, Has().Exactly(2).OfLength(0));
  }

  it("handles two empty lines with Windows line endings");
  {
    std::vector<std::string> res;
    StringLineParser::Parse("\r\n\r\n", res);
    AssertThat(res, HasLength(2));
    AssertThat(res, Has().Exactly(2).OfLength(0));
  }

  it("handles carriage return only");
  {
    std::vector<std::string> res;
    StringLineParser::Parse("One line\rTwo lines", res);
    AssertThat(res, HasLength(2));
    AssertThat(res, Has().Exactly(1).EqualTo("One line"));
    AssertThat(res, Has().Exactly(1).EqualTo("Two lines"));
  }

  it("handles carriage return only at end of string");
  {
    std::vector<std::string> res;
    StringLineParser::Parse("One line\r\nTwo lines\r", res);
    AssertThat(res, HasLength(2));
    AssertThat(res, Has().Exactly(1).EqualTo("One line"));
    AssertThat(res, Has().Exactly(1).EqualTo("Two lines"));
  }
}
