#ifndef STREAM_H
#define STREAM_H

extern std::unique_ptr<Fluxmap> readStream(
    std::string dir, unsigned track, unsigned side);
extern std::unique_ptr<Fluxmap> readStream(const std::string& path);
extern std::unique_ptr<Fluxmap> readStream(const Bytes& bytes);

#endif
