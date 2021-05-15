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
	static std::set<unsigned> parseRange(const std::string& spec);

    static Modifier parseMod(const std::string& spec);

public:
    DataSpec(const std::string& spec)
    { set(spec); }

    void set(const std::string& spec);
    operator std::string () const;

    const Modifier& at(const std::string& mod) const;
    bool has(const std::string& mod) const;

	unsigned atOr(const std::string& mod, unsigned value) const
	{ return has(mod) ? at(mod).only() : value; }

    std::string filename;
    std::map<std::string, Modifier> modifiers;
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

class RangeFlag : public Flag
{
public:
    RangeFlag(const std::vector<std::string>& names, const std::string helptext,
            const std::string& defaultValue):
        Flag(names, helptext),
		_stringValue(defaultValue),
        _value(DataSpec::parseRange(defaultValue))
    {}

    const std::set<unsigned>& get() const
    { checkInitialised(); return _value; }

    operator const std::set<unsigned>& () const
    { return get(); }

    bool hasArgument() const { return true; }
    const std::string defaultValueAsString() const { return _stringValue; }

    void set(const std::string& value)
	{
		_stringValue = value;
		_value = DataSpec::parseRange(value);
	}

private:
	std::string _stringValue;
    std::set<unsigned> _value;
};

class Agg2D;

class BitmapSpec
{
public:
    BitmapSpec(const DataSpec& dataSpec);
    BitmapSpec(const std::string filename, unsigned width, unsigned height);

	Agg2D& painter();
	void save();

private:
	std::vector<uint8_t> _bitmap;
	std::unique_ptr<Agg2D> _painter;
public:
    std::string filename;
    unsigned width;
    unsigned height;
    bool initialised : 1;
};

#endif
