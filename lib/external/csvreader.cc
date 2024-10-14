#include <string>
#include <vector>
#include "lib/external/csvreader.h"

std::vector<std::string> CsvReader::readLine()
{
    enum state_t
    {
        UNQUOTED,
        QUOTED,
        QUOTEDQUOTE
    };

    std::vector<std::string> results;
    std::string row;
    std::getline(_istream, row);
    if (_istream.bad() || _istream.fail())
        return results;

    results.push_back("");
    state_t state = UNQUOTED;
    size_t index = 0;
    for (char c : row)
    {
        switch (state)
        {
            case UNQUOTED:
                switch (c)
                {
                    case ',':
                        results.push_back("");
                        index++;
                        break;

                    case '"':
                        state = QUOTED;
                        break;

                    default:
                        results[index].push_back(c);
                        break;
                }
                break;

            case QUOTED:
                switch (c)
                {
                    case '"':
                        state = QUOTEDQUOTE;
                        break;

                    default:
                        results[index].push_back(c);
                        break;
                }
                break;

            case QUOTEDQUOTE:
                switch (c)
                {
                    case ',':
                        results.push_back("");
                        index++;
                        state = UNQUOTED;
                        break;

                    case '"':
                        results[index].push_back('"');
                        state = QUOTED;
                        break;

                    default:
                        state = UNQUOTED;
                        break;
                }
                break;
        }
    }
    return results;
}
