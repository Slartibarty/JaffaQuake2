/*
=============================================================================

WINDOWS QUAKE FILESYSTEM

=============================================================================
*/

#include "engine.h"
#include "winquake.h"
#include <io.h>

// Extern C because the rest of the engine is in C
extern "C" {

#include "filesystem_private.h"

// Globals

char			fs_gamedir[MAX_OSPATH];
cvar_t*			fs_basedir;
cvar_t*			fs_gamedirvar;

filelink_t*		fs_links;

searchpath_t*	fs_searchpaths;
searchpath_t*	fs_base_searchpaths;	// without gamedirs


/*
================
FS_Mkdir_Private
================
*/
qboolean FS_Mkdir_Private(const char* path)
{
	return CreateDirectoryA(path, nullptr);
}


/*
================
FS_Mkdir
================
*/
qboolean FS_Mkdir(const char* path)
{
	char newpath[MAX_OSPATH];
	Com_sprintf(newpath, sizeof(newpath), "%s/%s", FS_Gamedir(), path);

	return FS_Mkdir_Private(newpath);
}


/*
================
FS_CopyFile_Private
================
*/
qboolean FS_CopyFile_Private(const char* src, const char* dest)
{
	return CopyFileA(src, dest, FALSE);
}


/*
================
FS_CopyFile
================
*/
qboolean FS_CopyFile(const char* src, const char* dest)
{
	char newsrc[MAX_OSPATH];
	char newdest[MAX_OSPATH];

	Com_sprintf(newsrc, "%s/%s", FS_Gamedir(), src);
	Com_sprintf(newdest, "%s/%s", FS_Gamedir(), dest);

	return FS_CopyFile_Private(newsrc, newdest);
}


/*
================
FS_DeleteFile_Private
================
*/
qboolean FS_DeleteFile_Private(const char* filename)
{
	return DeleteFileA(filename);
}


/*
================
FS_DeleteFile
================
*/
qboolean FS_DeleteFile(const char* filename)
{
	char newsrc[MAX_OSPATH];

	Com_sprintf(newsrc, "%s/%s", FS_Gamedir(), filename);

	return FS_DeleteFile_Private(filename);
}


/*
================
FS_FileLength
================
*/
int FS_FileLength (fshandle_t f)
{
	LARGE_INTEGER length;
	GetFileSizeEx(f, &length);

	// Slart: Narrowing conversion
	return (int)length.QuadPart;
}


/*
===========
FS_OpenFile

Finds the file in the search path.
returns filesize and an open FILE *
Used for streaming data out of either a pak file or
a seperate file.
===========
*/
int file_from_pak;
int FS_OpenFile(const char* filename, fshandle_t* file)
{
	searchpath_t	*search;
	char			netpath[MAX_OSPATH];
	pack_t			*pak;
	int				i;
	filelink_t		*link;

	file_from_pak = 0;

	// check for links first
	for (link = fs_links ; link ; link=link->next)
	{
		if (!strncmp (filename, link->from, link->fromlength))
		{
			Com_sprintf (netpath, sizeof(netpath), "%s%s",link->to, filename+link->fromlength);
			*file = CreateFileA(netpath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
			if (*file)
			{		
				Com_DPrintf ("link file: %s\n",netpath);
				return FS_FileLength (*file);
			}
			return -1;
		}
	}

//
// search through the path, one element at a time
//
	for (search = fs_searchpaths ; search ; search = search->next)
	{
	// is the element a pak file?
		if (search->pack)
		{
		// look through all the pak file elements
			pak = search->pack;
			for (i=0 ; i<pak->numfiles ; i++)
				if (!Q_strcasecmp (pak->files[i].name, filename))
				{
					// found it!
					file_from_pak = 1;
					Com_DPrintf ("PackFile: %s : %s\n",pak->filename, filename);
					// open a new file on the pakfile
					*file = CreateFileA(pak->filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
					if (*file == INVALID_HANDLE_VALUE) {
						Com_Errorf(ERR_FATAL, "Couldn't reopen %s", pak->filename);
					}

					// Slart: This sucks
					LARGE_INTEGER dist;
					dist.QuadPart = pak->files[i].filepos;
					SetFilePointerEx(*file, dist, nullptr, FILE_BEGIN);

					return pak->files[i].filelen;
				}
		}
		else
		{		
	// check a file in the directory tree
			
			Com_sprintf (netpath, sizeof(netpath), "%s/%s",search->filename, filename);
			
			*file = CreateFileA(netpath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
			if (*file == INVALID_HANDLE_VALUE)
				continue;
			
			Com_DPrintf ("FindFile: %s\n",netpath);

			return FS_FileLength (*file);
		}
		
	}
	
	Com_DPrintf ("FindFile: can't find %s\n", filename);
	
	*file = NULL;
	return -1;
}


/*
===========
FS_OpenFileWrite

Creates a file in the game tree for writing
Does not open existing files, if they do exist, they're overwritten
Does not write into pack files
This is usually used to write demos, configs and screenshots, so they
always write to the current gamedir
===========
*/
void FS_OpenFileWrite(const char* filename, fshandle_t* file, fswritemode_t mode)
{
	char path[MAX_OSPATH];
	DWORD dwDesiredAccess = GENERIC_WRITE;
	DWORD dwCreationDisposition = CREATE_ALWAYS;

	if (mode == fs_append) {
		dwDesiredAccess = FILE_APPEND_DATA;
		dwCreationDisposition = OPEN_EXISTING;
	}

	Com_sprintf(path, sizeof(path), "%s/%s", FS_Gamedir(), filename);

	*file = CreateFileA(path, dwDesiredAccess, FILE_SHARE_READ, nullptr, dwCreationDisposition, 0, nullptr);
	if (*file == INVALID_HANDLE_VALUE)
	{
		*file = NULL;
		Com_DPrintf("FileWrite: can't find %s\n", filename);
		return;
	}

	Com_DPrintf("FileWrite: %s\n", path);
}


/*
=================
FS_Read

Properly handles partial reads
=================
*/
int FS_Read (void *buffer, int len, fshandle_t f)
{
	DWORD bytesRead;
	BOOL result = ReadFile(f, buffer, len, &bytesRead, nullptr);
	assert(result);

	if (!result) {
		Com_Error(ERR_FATAL, "FS_Read: failed to read file!");
	}

	return (int)bytesRead;
}


/*
=================
FS_Write

Properly handles partial reads
=================
*/
int FS_Write (const void* buffer, int len, fshandle_t f)
{
	if (len == -1) {
		len = (int)strlen((const char*)buffer);
	}

	DWORD bytesWritten;
	BOOL result = WriteFile(f, buffer, len, &bytesWritten, nullptr);
	assert(result);

	if (!result) {
		Com_Error(ERR_FATAL, "FS_Write: failed to write to file!");
	}

	return (int)bytesWritten;
}


/*
==============
FS_CloseFile

For some reason, other dll's can't just call fclose()
on files returned by FS_OpenFile...
==============
*/
void FS_CloseFile(fshandle_t f)
{
	CloseHandle(f);
}


/*
=================
FS_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
pack_t *FS_LoadPackFile (const char *packfile)
{
	// Optimize for random access
	fshandle_t packHandle = CreateFileA(packfile, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);
	if (packHandle == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	dpackheader_t	header;
	DWORD			bytesRead;
	ReadFile(packHandle, &header, sizeof(header), &bytesRead, nullptr);
	if (bytesRead != sizeof(header) || header.ident != IDPAKHEADER) {
		// Obviously not a pakfile
		Com_Errorf(ERR_FATAL, "%s is not a packfile", packfile);
	}

	int numPackFiles = header.dirlen / sizeof(dpackfile_t);

	packfile_t*		newfiles = (packfile_t*)Z_Malloc(numPackFiles * sizeof(packfile_t));
	dpackfile_t*	info = (dpackfile_t*)Z_Malloc(numPackFiles * sizeof(dpackfile_t));

	LARGE_INTEGER tmpofs;
	tmpofs.QuadPart = header.dirofs;
	SetFilePointerEx(packHandle, tmpofs, nullptr, FILE_BEGIN);
	ReadFile(packHandle, info, header.dirlen, nullptr, nullptr);

	// parse the directory
	for (int i = 0; i < numPackFiles; ++i)
	{
		strcpy(newfiles[i].name, info[i].name);
		newfiles[i].filepos = LittleLong(info[i].filepos);
		newfiles[i].filelen = LittleLong(info[i].filelen);
	}

	Z_Free(info);

	pack_t* pack = (pack_t*)Z_Malloc(sizeof(pack_t));
	strcpy(pack->filename, packfile);
	pack->handle = packHandle;
	pack->numfiles = numPackFiles;
	pack->files = newfiles;

	Com_Printf("Added packfile %s (%i files)\n", packfile, numPackFiles);
	return pack;
}


// Find functions

//===============================================================================

static char		findbase[MAX_OSPATH];
static char		findpath[MAX_OSPATH];
static intptr_t	findhandle;

static bool CompareAttributes( uint found, uint musthave, uint canthave )
{
	if ( ( found & _A_RDONLY ) && ( canthave & SFF_RDONLY ) )
		return false;
	if ( ( found & _A_HIDDEN ) && ( canthave & SFF_HIDDEN ) )
		return false;
	if ( ( found & _A_SYSTEM ) && ( canthave & SFF_SYSTEM ) )
		return false;
	if ( ( found & _A_SUBDIR ) && ( canthave & SFF_SUBDIR ) )
		return false;
	if ( ( found & _A_ARCH ) && ( canthave & SFF_ARCH ) )
		return false;

	if ( ( musthave & SFF_RDONLY ) && !( found & _A_RDONLY ) )
		return false;
	if ( ( musthave & SFF_HIDDEN ) && !( found & _A_HIDDEN ) )
		return false;
	if ( ( musthave & SFF_SYSTEM ) && !( found & _A_SYSTEM ) )
		return false;
	if ( ( musthave & SFF_SUBDIR ) && !( found & _A_SUBDIR ) )
		return false;
	if ( ( musthave & SFF_ARCH ) && !( found & _A_ARCH ) )
		return false;

	return true;
}

char *FS_FindFirst ( const char *path, uint musthave, uint canthave )
{
	struct _finddata_t findinfo;

	assert(findhandle == 0);

	Com_sprintf (findbase, "%s/%s", FS_Gamedir(), path);
	COM_FilePath (findbase, findbase);
	findhandle = _findfirst (path, &findinfo);
	if (findhandle == -1)
		return NULL;
	if ( !CompareAttributes( findinfo.attrib, musthave, canthave ) )
		return NULL;
	Com_sprintf (findpath, sizeof(findpath), "%s/%s", findbase, findinfo.name);
	return findpath;
}

char *FS_FindNext ( uint musthave, uint canthave )
{
	struct _finddata_t findinfo;

	if (findhandle == -1)
		return NULL;
	if (_findnext (findhandle, &findinfo) == -1)
		return NULL;
	if ( !CompareAttributes( findinfo.attrib, musthave, canthave ) )
		return NULL;

	Com_sprintf (findpath, sizeof(findpath), "%s/%s", findbase, findinfo.name);
	return findpath;
}

void FS_FindClose (void)
{
	if (findhandle != -1)
		_findclose (findhandle);
	findhandle = 0;
}

}
