/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

#pragma once

#include "../../common/fstypes.h"

void		FS_Mkdir(const char* path);

int			FS_FileLength(fshandle_t f);

void		FS_CreatePath(char* path);

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
