#ifndef CATWEASEL_H
#define CATWEASEL_H

class Fluxmap;
class Bytes;

extern std::unique_ptr<Fluxmap> decodeCatweaselData(
    const Bytes& bytes, nanoseconds_t clock);

#endif
