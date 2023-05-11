#pragma once

#ifdef __cplusplus

class ConfigProto;
class OptionProto;

class Config
{
public:
	ConfigProto* operator -> () const;
	operator ConfigProto* () const;
	operator ConfigProto& () const;

	/* Set and get individual config keys. */

	void set(std::string key, std::string value);
	std::string get(std::string key);

	/* Reset the entire configuration. */

	void clear();

	/* Merge in one config file. */

	void readConfigFile(std::string filename);

    /* Modify the current config to engage the named option. */

    void applyOption(const OptionProto& option);
    bool applyOption(const std::string& option);
};

extern Config& globalConfig();

#endif

