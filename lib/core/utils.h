#ifndef UTILS_H
#define UTILS_H

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

extern std::string join(
    const std::vector<std::string>& values, const std::string& separator);
extern std::vector<std::string> split(
    const std::string& string, char separator);
extern bool beginsWith(const std::string& value, const std::string& beginning);
extern bool endsWith(const std::string& value, const std::string& ending);
extern std::string toUpper(const std::string& value);
extern std::string leftTrimWhitespace(std::string value);
extern std::string rightTrimWhitespace(std::string value);
extern std::string trimWhitespace(const std::string& value);
extern std::string getLeafname(const std::string& value);
extern std::string toIso8601(time_t t);
extern std::string quote(const std::string& s);
extern std::string unhex(const std::string& s);
extern std::string tohex(const std::string& s);
extern bool doesFileExist(const std::string& filename);
extern int countSetBits(uint32_t word);
extern uint32_t unbcd(uint32_t bcd);
extern int findLowestSetBit(uint64_t value);

extern void fillBitmapTo(std::vector<bool>& bitmap,
    unsigned& cursor,
    unsigned terminateAt,
    const std::vector<bool>& pattern);

template <class K, class V>
std::map<V, K> reverseMap(const std::map<K, V>& map)
{
    std::map<V, K> reverse;
    for (const auto& [k, v] : map)
        reverse[v] = k;
    return reverse;
}

/* If set, any running job will terminate as soon as possible (with an error).
 */

extern bool emergencyStop;
class EmergencyStopException
{
};
extern void testForEmergencyStop();

#endif
