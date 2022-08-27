#include "globals.h"
#include "utils.h"

bool emergencyStop = false;

static const char* WHITESPACE = " \t\n\r\f\v";
static const char* SEPARATORS = "/\\";

void ErrorException::print() const
{
    std::cerr << message << '\n';
}

std::string join(
    const std::vector<std::string>& values, const std::string& separator)
{
    switch (values.size())
    {
        case 0:
            return "";

        case 1:
            return values[0];

        default:
            std::stringstream ss;
            ss << values[0];
            for (int i = 1; i < values.size(); i++)
                ss << separator << values[i];
            return ss.str();
    }
}

bool beginsWith(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.begin(), ending.end(), value.begin());
}

// Case-insensitive for endings within ASCII.
bool endsWith(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size())
        return false;

    std::string lowercase(ending.size(), 0);
    std::transform(value.rbegin(),
        value.rbegin() + ending.size(),
        lowercase.begin(),
        [](unsigned char c)
        {
            return std::tolower(c);
        });

    return std::equal(ending.rbegin(), ending.rend(), value.rbegin()) ||
           std::equal(ending.rbegin(), ending.rend(), lowercase.begin());
}

std::string leftTrimWhitespace(std::string value)
{
    value.erase(0, value.find_first_not_of(WHITESPACE));
    return value;
}

std::string rightTrimWhitespace(std::string value)
{
    value.erase(value.find_last_not_of(WHITESPACE) + 1);
    return value;
}

std::string trimWhitespace(const std::string& value)
{
    return leftTrimWhitespace(rightTrimWhitespace(value));
}

std::string getLeafname(const std::string& value)
{
    constexpr char sep = '/';

    size_t i = value.find_last_of(SEPARATORS);
    if (i != std::string::npos)
    {
        return value.substr(i + 1, value.length() - i);
    }

    return value;
}

void testForEmergencyStop()
{
    if (emergencyStop)
        throw EmergencyStopException();
}
