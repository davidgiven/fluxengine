#ifndef DATASPEC_H
#define DATASPEC_H

class MissingModifierException : public std::runtime_error
{
public:
    MissingModifierException(const std::string& mod);
    const std::string mod;
};

class DataSpec
{
public:
    struct Modifier
    {
        std::string name;
        std::set<unsigned> data;
        std::string source;

        bool operator == (const Modifier& other) const
        { return (name == other.name) && (data == other.data); }

        bool operator != (const Modifier& other) const
        { return (name != other.name) || (data != other.data); }

        unsigned only() const
        {
            if (data.size() != 1)
                Error() << "modifier " << name << " can only have one value";
            return *(data.begin());
        }
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

    const Modifier& at(const std::string& mod) const;
    bool has(const std::string& mod) const;

    std::string filename;
    std::map<std::string, Modifier> modifiers;
};

class FluxSpec
{
public:
    struct Location
    {
        unsigned drive;
        unsigned track;
        unsigned side;

        bool operator == (const Location& other) const
        { return (drive == other.drive) && (track == other.track) && (side == other.side); }

        bool operator != (const Location& other) const
        { return (drive != other.drive) || (track != other.track) || (side != other.side); }
    };

public:
    FluxSpec(const DataSpec& dataspec);

public:
    std::string filename;
    std::vector<Location> locations;
    unsigned drive;
};

class ImageSpec
{
public:
    ImageSpec(const DataSpec& dataspec);
    ImageSpec(const std::string filename,
        unsigned cylinders, unsigned heads, unsigned sectors, unsigned bytes);

public:
    std::string filename;
    unsigned cylinders;
    unsigned heads;
    unsigned sectors;
    unsigned bytes;
    bool initialised : 1;
};

static inline std::ostream& operator << (std::ostream& os, const DataSpec& dataSpec)
{ os << (std::string)dataSpec; return os; }

class DataSpecFlag : public Flag
{
public:
    DataSpecFlag(const std::vector<std::string>& names, const std::string helptext,
            const std::string& defaultValue):
        Flag(names, helptext),
        _value(defaultValue)
    {}

    const DataSpec& get() const
    { checkInitialised(); return _value; }

    operator const DataSpec& () const
    { return get(); }

    bool hasArgument() const { return true; }
    const std::string defaultValueAsString() const { return _value; }
    void set(const std::string& value) { _value.set(value); }

private:
    DataSpec _value;
};

#endif
