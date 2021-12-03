#ifndef UTILS_H
#define UTILS_H

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

extern bool beginsWith(const std::string& value, const std::string& beginning);
extern bool endsWith(const std::string& value, const std::string& ending);

#endif

