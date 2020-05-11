/*
=============================================================================

STDIO QUAKE FILESYSTEM

=============================================================================
*/

#include "engine.h"

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
	int		pos;
	int		end;

	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, pos, SEEK_SET);

	return end;
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
			*file = fopen (netpath, "rb");
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
				{	// found it!
					file_from_pak = 1;
					Com_DPrintf ("PackFile: %s : %s\n",pak->filename, filename);
				// open a new file on the pakfile
					*file = fopen (pak->filename, "rb");
					if (!*file)
						Com_Errorf (ERR_FATAL, "Couldn't reopen %s", pak->filename);	
					fseek (*file, pak->files[i].filepos, SEEK_SET);
					return pak->files[i].filelen;
				}
		}
		else
		{		
	// check a file in the directory tree
			
			Com_sprintf (netpath, sizeof(netpath), "%s/%s",search->filename, filename);
			
			*file = fopen (netpath, "rb");
			if (!*file)
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
#define	MAX_READ	0x10000		// read in blocks of 64k
int FS_Read (void *buffer, int len, fshandle_t f)
{
	// Slart: A progress bar was never implemented, this block reading technique nets no benefit
	// If a progress bar is ever implemented, bring this path back
#ifdef PROGRESSBAR
	int		block, remaining;
	int		read;
	byte	*buf;

	buf = (byte *)buffer;

	// read in chunks for progress bar
	remaining = len;
	while (remaining)
	{
		block = remaining;
		if (block > MAX_READ)
			block = MAX_READ;
		read = (int)fread (buf, 1, block, f);
		if (read <= 0)
		{
			Com_Error (ERR_FATAL, "FS_Read: %d bytes read", read);
		}

		// do some progress bar thing here...

		remaining -= read;
		buf += read;
	}
#else
	int read = (int)fread(buffer, 1, len, f);

	if (read <= 0)
	{
		Com_Errorf(ERR_FATAL, "FS_Read: %d bytes read", read);
	}

	return read;
#endif
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
	fclose(f);
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
	dpackheader_t	header;
	int				i;
	packfile_t		*newfiles;
	int				numpackfiles;
	pack_t			*pack;
	FILE			*packhandle;
	dpackfile_t		*info;
	unsigned		checksum;

	packhandle = fopen(packfile, "rb");
	if (!packhandle)
		return NULL;

	fread (&header, 1, sizeof(header), packhandle);
	if (LittleLong(header.ident) != IDPAKHEADER)
		Com_Errorf (ERR_FATAL, "%s is not a packfile", packfile);
	header.dirofs = LittleLong (header.dirofs);
	header.dirlen = LittleLong (header.dirlen);

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	newfiles = (packfile_t*)Z_Malloc (numpackfiles * sizeof(packfile_t));
	info = (dpackfile_t*)Z_Malloc(numpackfiles * sizeof(dpackfile_t));

	fseek (packhandle, header.dirofs, SEEK_SET);
	fread (info, 1, header.dirlen, packhandle);

// parse the directory
	for (i=0 ; i<numpackfiles ; i++)
	{
		strcpy (newfiles[i].name, info[i].name);
		newfiles[i].filepos = LittleLong(info[i].filepos);
		newfiles[i].filelen = LittleLong(info[i].filelen);
	}

	free(info);

	pack = (pack_t*)Z_Malloc (sizeof (pack_t));
	strcpy (pack->filename, packfile);
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;
	
	Com_Printf ("Added packfile %s (%i files)\n", packfile, numpackfiles);
	return pack;
}

}
