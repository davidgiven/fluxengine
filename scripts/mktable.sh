#!/bin/sh
echo "#include <string>"
echo "#include <map>"
echo "class ConfigProto;"

word=$1
shift

for a in "$@"; do
	echo "extern const ConfigProto ${word}_${a}_pb;"
done

echo "extern const std::map<std::string, const ConfigProto*> ${word};"
echo "const std::map<std::string, const ConfigProto*> ${word} = {"
for a in "$@"; do
	echo "    { \"${a}\", &${word}_${a}_pb },"
done
echo "};"


