/*
=============================================================================

Filesystem types that need to be used outside of the engine
The functions are still exported through the structs

=============================================================================
*/

#ifndef _WIN32
#define FILESYS_STDIO
#endif

// Interchangable between stdio and win
typedef void*	fshandle_t;

typedef enum
{
	fs_overwrite,
	fs_append
} fswritemode_t;
