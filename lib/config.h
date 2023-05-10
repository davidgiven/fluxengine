#pragma once

class ConfigProto;

class Config
{
public:
	ConfigProto* operator -> () const;
	operator ConfigProto* () const;
	operator ConfigProto& () const;
};

extern Config& globalConfig();

