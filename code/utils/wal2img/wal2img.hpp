
#pragma once

// Qfiles needs these, we use them too
using uint = unsigned int;
using byte = unsigned char;

#include "../../common/qfiles.h"

struct rgb_t
{
	byte r, g, b;
};

template <typename ptrType>
struct ezptr_t
{
	ptrType* ptr;

	// Methods

	void Free()
	{
		assert(ptr);
		free(ptr);
	}

	// Constructors and destructors

	// New pointer
	ezptr_t(size_t size)
	{
		ptr = malloc(size);
	}

	// Pointer to an existing pointer
	ezptr_t(ptrType* existing)
	{
		ptr = existing;
	}

	// I'm freeee!
	~ezptr_t()
	{
		if (ptr) {
			Free();
		}
	}

};

rgb_t* LoadPCXPalette(FILE*& handle);
