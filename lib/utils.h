#ifndef UTILS_H
#define UTILS_H

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

extern bool beginsWith(const std::string& value, const std::string& beginning);
extern bool endsWith(const std::string& value, const std::string& ending);
extern std::string leftTrimWhitespace(std::string value);
extern std::string rightTrimWhitespace(std::string value);
extern std::string trimWhitespace(const std::string& value);
extern std::string getLeafname(const std::string& value);

/* If set, any running job will terminate as soon as possible (with an error).
 */

extern bool emergencyStop;
class EmergencyStopException {};
extern void testForEmergencyStop();

#endif

