#pragma once

#ifdef __cplusplus

class ConfigProto;

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
};

extern Config& globalConfig();

#endif

