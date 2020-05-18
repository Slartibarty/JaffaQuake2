
#include <cstdlib>
#include <cstdio>

#include "wal2img.hpp"

rgb_t* LoadPCXPalette(FILE*& handle)
{
	pcx_t pcx;

	size_t amntRead = fread(&pcx, sizeof(pcx), 1, handle);
	if (amntRead != 1) {
		return nullptr;
	}

	if (pcx.manufacturer != 0x0a
		|| pcx.version != 5
		|| pcx.encoding != 1
		|| pcx.bits_per_pixel != 8)
	{
		return nullptr;
	}

	int len;
	fseek(handle, 0, SEEK_END);
	len = ftell(handle);
	fseek(handle, len - 768, SEEK_SET);

	rgb_t* out = (rgb_t*)malloc(768);
	amntRead = fread(out, 1, 768, handle);
	if (amntRead < 768) {
		free(out);
		return nullptr;
	}

//	width = pcx.xmax + 1;
//	height = pcx.ymax + 1;

	return out;
}
