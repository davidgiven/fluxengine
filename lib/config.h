#pragma once

#ifdef __cplusplus

class ConfigProto;

class Config
{
public:
	ConfigProto* operator -> () const;
	operator ConfigProto* () const;
	operator ConfigProto& () const;

	void set(std::string key, std::string value);
	std::string get(std::string key);
};

extern Config& globalConfig();

#endif

