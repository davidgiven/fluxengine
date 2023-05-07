#ifndef FLUXENGINE_H
#define FLUXENGINE_H

extern void showProfiles(const std::string& command,
    const std::map<std::string, const ConfigProto*>& profiles);

extern const std::map<std::string, const ConfigProto*> formats;

#endif
