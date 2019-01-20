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
        std::string source;

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
    operator std::string () const;

    std::string filename;
    std::map<std::string, Modifier> modifiers;
    std::vector<Location> locations;
};

std::ostream& operator << (std::ostream& os, const DataSpec& dataSpec)
{ os << (std::string)dataSpec; return os; }

class DataSpecFlag : public Flag
{
public:
    DataSpecFlag(const std::vector<std::string>& names, const std::string helptext,
            const std::string& defaultValue):
        Flag(names, helptext),
        value(defaultValue)
    {}

    bool hasArgument() const { return true; }
    const std::string defaultValueAsString() const { return value; }
    void set(const std::string& value) { this->value.set(value); }

public:
    DataSpec value;
};

#endif
