#ifndef FILEUTILS_H
#define FILEUTILS_H

extern FlagGroup fileFlags;

extern std::unique_ptr<Filesystem> createFilesystemFromConfig();

#endif

