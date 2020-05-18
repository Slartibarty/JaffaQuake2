//
// WAL2IMG
//
// Okay so, I need this to write files into the folder they came from, the user has
//

#include <string>

#include <cstdlib>
#include <cstdio>
#include <cassert>

#include "wal2img.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../external/stb/stb_image_write.h"

#define COLOURMAP_NAME	"colormap.pcx"

static rgb_t* LoadColourMap()
{
	FILE* handle = fopen(COLOURMAP_NAME, "rb");
	if (!handle) {
		return nullptr;
	}

	rgb_t* colourMap = LoadPCXPalette(handle);
	if (!colourMap) {
		return nullptr;
	}

	return colourMap;
}

// Decodes an 8-bit paletted WAL to 24-bit raw image data
static rgb_t* DecodeWAL(FILE*& handle, rgb_t*& colourMap, int& width, int& height)
{
	miptex_t header;
	size_t amntRead;
	
	amntRead = fread(&header, sizeof(header), 1, handle);
	if (amntRead != 1)
	{
		printf("Error: Size read was less than that of a WAL file's header. Read %zu bytes.\n", amntRead);
		return nullptr;
	}

	uint size = header.width * header.height;
	
	// Allocate space for the 8-bit pic
	byte* pic8bit;
	pic8bit = (byte*)malloc(size);
	fseek(handle, header.offsets[0], SEEK_SET);
	fread(pic8bit, 1, size, handle);

	rgb_t* pic24bit;
	pic24bit = (rgb_t*)malloc(size * 3);

	uint c = header.width * header.height;

	// For each pixel, map to colourMap
	for (uint i = 0; i < c; ++i)
	{
		pic24bit[i] = colourMap[pic8bit[i]];
	}

	free(pic8bit);

	// Narrowing conversions
	width = header.width;
	height = header.height;
	
	return pic24bit;
}

int main(int argc, char** argv)
{	
	if (argc != 2)
	{
		printf("Error: Need to specify a .wal filename!\n");
		return 1;
	}
	
	// Verifications done, load the colourmap
	ezptr_t<rgb_t> colourMap(LoadColourMap());
	if (!colourMap.ptr)
	{
		printf(
		"Error: Couldn't open the colormap!\n"
		"Be sure to place Quake 2's colormap.pcx next to the executable, or in your working directory.\n"
		);
		return 1;
	}
		
	FILE* handle = fopen(argv[1], "rb");
	if (!handle)
	{
		printf("Error: Could not open %s.\n", argv[1]);
		return 1;
	}
	
	int width, height;
	ezptr_t<rgb_t> pic32bit(DecodeWAL(handle, colourMap.ptr, width, height));
	if (!pic32bit.ptr)
	{
		fclose(handle);
		return 1;
	}

	// Generate the output filename lazily
	char* outName;
	size_t inLength = strlen(argv[1]);
	outName = (char*)malloc(inLength + 1);
	memcpy(outName, argv[1], inLength + 1);
	
	outName[inLength - 1]	= 'a';
	outName[inLength - 2]	= 'g';
	outName[inLength - 3]	= 't';
	
	// Write a 24-bit TGA
	int result = stbi_write_tga(outName, width, height, 3, pic32bit.ptr);
	if (result == 0) {
		printf("Failed to write %s.", outName);
	}
	else {
		printf("Wrote %s.", outName);
	}

	free(outName);

	return 0;
}