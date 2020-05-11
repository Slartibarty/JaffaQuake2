/*
=============================================================================

WINDOWS QUAKE FILESYSTEM

=============================================================================
*/

#include "engine.h"
#include "winquake.h"

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
=================
FS_ReadFile

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
	fshandle_t packHandle = CreateFileA(packfile, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
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

}
