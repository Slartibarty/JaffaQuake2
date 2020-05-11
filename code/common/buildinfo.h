//
// Build info
//

#pragma once

#define	VERSION			3.22
#define	VERSIONSTRING	"3.22"

#ifdef _WIN64

#ifdef NDEBUG
#define BUILDSTRING "Win64 RELEASE"
#else
#define BUILDSTRING "Win64 DEBUG"
#endif

#define	CPUSTRING	"x64"

// Win32
#elif _WIN32

#ifdef NDEBUG
#define BUILDSTRING "Win32 RELEASE"
#else
#define BUILDSTRING "Win32 DEBUG"
#endif

#define	CPUSTRING	"x86"

#else

#error Invalid platform

#endif
