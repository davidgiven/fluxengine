#ifndef STREAM_H
#define STREAM_H

extern std::unique_ptr<Fluxmap> readStream(const std::string& path, unsigned track, unsigned side);

#endif
