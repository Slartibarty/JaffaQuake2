/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

#pragma once

#include "../../common/fstypes.h"

qboolean	FS_Mkdir(const char* path);

qboolean	FS_CopyFile(const char* src, const char* dest);

qboolean	FS_DeleteFile(const char* filename);

int			FS_FileLength(fshandle_t f);

void		FS_CreatePath(const char* path);

// Open a file for reading
int			FS_OpenFile(const char* filename, fshandle_t* file);
// Open a file for writing, discarding contents
void		FS_OpenFileWrite(const char* filename, fshandle_t* file, fswritemode_t mode);
// properly handles partial reads
int			FS_Read(void* buffer, int len, fshandle_t f);
int			FS_Write(const void* buffer, int len, fshandle_t f);
// note: this can't be called from another DLL, due to MS libc issues
void		FS_CloseFile(fshandle_t f);

// a null buffer will just return the file length without loading
// a -1 length is not present
int			FS_LoadFile(const char* path, void** buffer);
void		FS_FreeFile(void* buffer);

const char*	FS_Gamedir(void);

void		FS_ExecAutoexec(void);

void		FS_SetGamedir(const char* dir);

char*		FS_NextPath(const char* prevpath);

void		FS_InitFilesystem(void);

// directory searching
#define SFF_ARCH    0x01
#define SFF_HIDDEN  0x02
#define SFF_RDONLY  0x04
#define SFF_SUBDIR  0x08
#define SFF_SYSTEM  0x10

/*
** pass in an attribute mask of things you wish to REJECT
*/
char	*FS_FindFirst ( const char *path, uint musthave, uint canthave );
char	*FS_FindNext ( uint musthave, uint canthave );
void	FS_FindClose (void);

