#ifndef FL2_H
#define FL2_H

class FluxFileProto;

extern FluxFileProto loadFl2File(const std::string filename);
extern void saveFl2File(const std::string filename, FluxFileProto& proto);

#endif
