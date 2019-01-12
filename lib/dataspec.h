#ifndef DATASPEC_H
#define DATASPEC_H

class DataSpec
{
public:
    struct Location
    {
        unsigned track;
        unsigned side;

        bool operator == (const Location& other) const
        { return (track == other.track) && (side == other.side); }

        bool operator != (const Location& other) const
        { return (track != other.track) || (side != other.side); }
    };

    struct Modifier
    {
        std::string name;
        std::set<unsigned> data;

        bool operator == (const Modifier& other) const
        { return (name == other.name) && (data == other.data); }

        bool operator != (const Modifier& other) const
        { return (name != other.name) || (data != other.data); }
    };

public:
    static std::vector<std::string> split(
        const std::string& s, const std::string& delimiter);
    static Modifier parseMod(const std::string& spec);

public:
    DataSpec(const std::string& spec)
    { set(spec); }

    void set(const std::string& spec);

    std::string filename;
    std::map<std::string, Modifier> modifiers;
    std::vector<Location> locations;
};

#endif
