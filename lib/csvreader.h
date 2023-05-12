#ifndef CSVREADER_H
#define CSVREADER_H

#include <istream>

class CsvReader
{
public:
    CsvReader(std::istream& istream): _istream(istream) {}

    std::vector<std::string> readLine();

private:
    std::istream& _istream;
};

#endif
