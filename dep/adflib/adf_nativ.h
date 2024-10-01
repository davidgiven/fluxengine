#ifndef ADF_NATIV_H
#define ADF_NATIV_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "adf_str.h"

#define NATIVE_FILE 8001

    struct nativeDevice
    {
        FILE* fd;
    };

    struct nativeFunctions
    {
        /* called by adfMount() */
        RETCODE (*adfInitDevice)(struct Device*, char*, BOOL);
        /* called by adfReadBlock() */
        RETCODE (*adfNativeReadSector)(struct Device*, int32_t, int, uint8_t*);
        /* called by adfWriteBlock() */
        RETCODE (*adfNativeWriteSector)(struct Device*, int32_t, int, uint8_t*);
        /* called by adfMount() */
        BOOL (*adfIsDevNative)(char*);
        /* called by adfUnMount() */
        RETCODE (*adfReleaseDevice)(struct Device*);
    };

    extern void adfInitNativeFct();

#ifdef __cplusplus
}
#endif

#endif
