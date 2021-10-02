#!/bin/sh
echo "#include <string>"
echo "#include <map>"

word=$1
shift

for a in "$@"; do
	echo "extern std::string ${word}_${a}_pb();"
done

echo "extern const std::map<std::string, std::string> ${word};"
echo "const std::map<std::string, std::string> ${word} = {"
for a in "$@"; do
	echo "    { \"${a}\", ${word}_${a}_pb() },"
done
echo "};"


