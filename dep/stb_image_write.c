#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <zlib.h>

static uint8_t* stbiw_compress(uint8_t *data, int data_len, int *out_len, int quality);

#define STB_IMAGE_WRITE_IMPLEMENTATION

#if defined NDEBUG
	#define STBIW_ASSERT(x) /* */
#endif

#define STBIW_ZLIB_COMPRESS stbiw_compress

#include "stb_image_write.h"

static uint8_t* stbiw_compress(uint8_t *data, int data_len, int *out_len, int quality)
{
	unsigned long size = compressBound(data_len);
	uint8_t* buf = (uint8_t*)malloc(size);

	if (buf == NULL)
		return NULL;

	if (compress2(buf, &size, data, data_len, quality) != Z_OK)
	{
		free(buf);
		return NULL;
	}

	*out_len = size;
	return buf;
}
