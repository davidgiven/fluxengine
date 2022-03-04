#include "globals.h"
#include "utils.h"

bool emergencyStop = false;

static const char* WHITESPACE = " \t\n\r\f\v";

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
    std::transform(value.rbegin(), value.rbegin() + ending.size(), lowercase.begin(), [](unsigned char c){ return std::tolower(c); });
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin()) ||
        std::equal(ending.rbegin(), ending.rend(), lowercase.begin());
}

void leftTrimWhitespace(std::string& value)
{
	value.erase(0, value.find_first_not_of(WHITESPACE));
}

void rightTrimWhitespace(std::string& value)
{
	value.erase(value.find_last_not_of(WHITESPACE) + 1);
}

void trimWhitespace(std::string& value)
{
	leftTrimWhitespace(value);
	rightTrimWhitespace(value);
}

void testForEmergencyStop()
{
	if (emergencyStop)
		throw EmergencyStopException();
}

