#ifndef TESTS_H
#define TESTS_H

class AssertionError
{
};

template <class T>
class Subject
{
public:
    Subject(const std::string& filename, int lineno, T value):
        _filename(filename),
        _lineno(lineno),
        _value(value)
    {
    }

public:
    void isEqualTo(T wanted)
    {
        if (_value != wanted)
            fail(fmt::format("wanted {}, got {}", wanted, _value));
    }

private:
    void fail(const std::string& message)
    {
        error("assertion failed: {}: {}: {}", _filename, _lineno, message);
    }

private:
    const std::string _filename;
    int _lineno;
    T _value;
};

#define assertThat(value) Subject(__FILE__, __LINE__, value)

#endif
