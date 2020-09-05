/*
=============================================================================

QUAKE FILESYSTEM

OS-inspecific filesystem stuff

=============================================================================
*/

#include "engine.h"

// Extern C because the rest of the engine is in C
extern "C" {

#include "filesystem_private.h"


/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
void FS_CreatePath(const char* path)
{
	char npath[MAX_OSPATH];

	Com_sprintf(npath, "%s/%s", FS_Gamedir(), path);

	for (char *ofs = npath + 1; *ofs; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = '\0';
			FS_Mkdir_Private(npath);
			*ofs = '/';
		}
	}
}


/*
============
Developer_searchpath

RAFAEL
============
*/
int	Developer_searchpath (int who)
{	
	for (searchpath_t* search = fs_searchpaths; search; search = search->next)
	{
		if (strstr (search->filename, "xatrix"))
			return 1;

		if (strstr (search->filename, "rogue"))
			return 2;
	}
	return 0;
}


/*
============
FS_LoadFile

Filename are relative to the quake search path
a null buffer will just return the file length without loading
============
*/
int FS_LoadFile(const char* path, void** buffer)
{
	// look for it in the filesystem or pack files
	fshandle_t h;
	int len = FS_OpenFile(path, &h);
	if (!h)
	{
		if (buffer)
			*buffer = NULL;
		return -1;
	}

	if (!buffer)
	{
		FS_CloseFile(h);
		return len;
	}

	*buffer = Z_Malloc(len);

	FS_Read(*buffer, len, h);

	FS_CloseFile(h);

	return len;
}


/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile(void* buffer)
{
	Z_Free(buffer);
}

/*
================
FS_AddGameDirectory

Sets fs_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ...
================
*/
void FS_AddGameDirectory(const char* dir)
{
	int				i;
	searchpath_t*	search;
	pack_t*			pak;
	char			pakfile[MAX_OSPATH];

	strcpy(fs_gamedir, dir);

	//
	// add the directory to the search path
	//
	search = (searchpath_t*)Z_Malloc(sizeof(searchpath_t));
	strcpy(search->filename, dir);
	search->next = fs_searchpaths;
	fs_searchpaths = search;

	//
	// add any pak files in the format pak0.pak pak1.pak, ...
	//
	for (i = 0; i < 10; i++)
	{
		Com_sprintf(pakfile, sizeof(pakfile), "%s/pak%i.pak", dir, i);
		pak = FS_LoadPackFile(pakfile);
		if (!pak)
			continue;
		search = (searchpath_t*)Z_Malloc(sizeof(searchpath_t));
		search->pack = pak;
		search->next = fs_searchpaths;
		fs_searchpaths = search;
	}
}


/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.
================
*/
void FS_SetGamedir(const char* dir)
{
	searchpath_t* next;

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":"))
	{
		Com_Printf("Gamedir should be a single filename, not a path\n");
		return;
	}

	//
	// free up any current game dir info
	//
	while (fs_searchpaths != fs_base_searchpaths)
	{
		if (fs_searchpaths->pack)
		{
			FS_CloseFile(fs_searchpaths->pack->handle);
			Z_Free(fs_searchpaths->pack->files);
			Z_Free(fs_searchpaths->pack);
		}
		next = fs_searchpaths->next;
		Z_Free(fs_searchpaths);
		fs_searchpaths = next;
	}

	//
	// flush all data, so it will be forced to reload
	//
	// Slart: This causes the refresh to be loaded twice on game startup
	if (dedicated && !dedicated->value)
		Cbuf_AddText ("vid_restart\nsnd_restart\n");

	Com_sprintf(fs_gamedir, sizeof(fs_gamedir), "%s/%s", fs_basedir->string, dir);

	if (!strcmp(dir, BASEDIRNAME) || (*dir == 0))
	{
		Cvar_FullSet("gamedir", "", CVAR_SERVERINFO | CVAR_NOSET);
		Cvar_FullSet("game", "", CVAR_LATCH | CVAR_SERVERINFO);
	}
	else
	{
		Cvar_FullSet("gamedir", dir, CVAR_SERVERINFO | CVAR_NOSET);
		FS_AddGameDirectory(va("%s/%s", fs_basedir->string, dir));
	}
}

/*
============
FS_Gamedir

Called to find where to write a file (demos, savegames, etc)
============
*/
const char* FS_Gamedir(void)
{
	if (*fs_gamedir)
		return fs_gamedir;
	else
		return BASEDIRNAME;
}


/*
=============
FS_ExecAutoexec
=============
*/
void FS_ExecAutoexec(void)
{
	if (FS_FindFirst("autoexec.cfg", 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM))
		Cbuf_AddText("exec autoexec.cfg\n");
	FS_FindClose();
}


/*
================
FS_Link_f

Creates a filelink_t
================
*/
void FS_Link_f(void)
{
	filelink_t* l, ** prev;

	if (Cmd_Argc() != 3)
	{
		Com_Printf("USAGE: link <from> <to>\n");
		return;
	}

	// see if the link already exists
	prev = &fs_links;
	for (l = fs_links; l; l = l->next)
	{
		if (!strcmp(l->from, Cmd_Argv(1)))
		{
			Z_Free(l->to);
			if (!strlen(Cmd_Argv(2)))
			{	// delete it
				*prev = l->next;
				Z_Free(l->from);
				Z_Free(l);
				return;
			}
			l->to = CopyString(Cmd_Argv(2));
			return;
		}
		prev = &l->next;
	}

	// create a new link
	l = (filelink_t*)Z_Malloc(sizeof(*l));
	l->next = fs_links;
	fs_links = l;
	l->from = CopyString(Cmd_Argv(1));
	l->fromlength = (int)strlen(l->from);
	l->to = CopyString(Cmd_Argv(2));
}


/*
** FS_ListFiles
*/
char** FS_ListFiles(const char* findname, int* numfiles, uint musthave, uint canthave)
{
	char* s;
	int nfiles = 0;
	char** list = 0;

	s = FS_FindFirst(findname, musthave, canthave);
	while (s)
	{
		if (s[strlen(s) - 1] != '.')
			nfiles++;
		s = FS_FindNext(musthave, canthave);
	}
	FS_FindClose();

	if (!nfiles)
		return NULL;

	nfiles++; // add space for a guard
	*numfiles = nfiles;

	list = (char**)Z_Malloc(sizeof(char*) * nfiles);
	memset(list, 0, sizeof(char*) * nfiles);

	s = FS_FindFirst(findname, musthave, canthave);
	nfiles = 0;
	while (s)
	{
		if (s[strlen(s) - 1] != '.')
		{
			// Slart: These are freed in menu.c
			list[nfiles] = Q_strdup(s);
#ifdef _WIN32
			Q_strlwr(list[nfiles]);
#endif
			nfiles++;
		}
		s = FS_FindNext(musthave, canthave);
	}
	FS_FindClose();

	return list;
}


/*
** FS_Dir_f
*/
void FS_Dir_f(void)
{
	char*	path = NULL;
	char	findname[1024];
	char	wildcard[1024] = "*.*";
	char**	dirnames;
	int		ndirs;

	if (Cmd_Argc() != 1)
	{
		strcpy(wildcard, Cmd_Argv(1));
	}

	while ((path = FS_NextPath(path)) != NULL)
	{
		char* tmp = findname;

		Com_sprintf(findname, sizeof(findname), "%s/%s", path, wildcard);

		while (*tmp != 0)
		{
			if (*tmp == '\\')
				*tmp = '/';
			tmp++;
		}
		Com_Printf("Directory of %s\n", findname);
		Com_Printf("----\n");

		if ((dirnames = FS_ListFiles(findname, &ndirs, 0, 0)) != 0)
		{
			int i;

			for (i = 0; i < ndirs - 1; i++)
			{
				if (strrchr(dirnames[i], '/'))
					Com_Printf("%s\n", strrchr(dirnames[i], '/') + 1);
				else
					Com_Printf("%s\n", dirnames[i]);

				free(dirnames[i]);
			}
			free(dirnames);
		}
		Com_Printf("\n");
	};
}


/*
============
FS_Path_f

============
*/
void FS_Path_f(void)
{
	searchpath_t*	s;
	filelink_t*		l;

	Com_Printf("Current search path:\n");
	for (s = fs_searchpaths; s; s = s->next)
	{
		if (s == fs_base_searchpaths)
			Com_Printf("----------\n");
		if (s->pack)
			Com_Printf("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
		else
			Com_Printf("%s\n", s->filename);
	}

	Com_Printf("\nLinks:\n");
	for (l = fs_links; l; l = l->next)
		Com_Printf("%s : %s\n", l->from, l->to);
}


/*
================
FS_NextPath

Allows enumerating all of the directories in the search path
================
*/
char* FS_NextPath(const char* prevpath)
{
	searchpath_t*	s;
	const char*		prev;

	if (!prevpath)
		return fs_gamedir;

	prev = fs_gamedir;
	for (s = fs_searchpaths; s; s = s->next)
	{
		if (s->pack)
			continue;
		if (prevpath == prev)
			return s->filename;
		prev = s->filename;
	}

	return NULL;
}


/*
================
FS_InitFilesystem
================
*/
void FS_InitFilesystem(void)
{
	Cmd_AddCommand("path", FS_Path_f);
	Cmd_AddCommand("link", FS_Link_f);
	Cmd_AddCommand("dir", FS_Dir_f);

	//
	// basedir <path>
	// allows the game to run from outside the data tree
	//
	fs_basedir = Cvar_Get("basedir", ".", CVAR_NOSET);

	//
	// start up with baseq2 by default
	//
	FS_AddGameDirectory(BASEDIRNAME);

	// any set gamedirs will be freed up to here
	fs_base_searchpaths = fs_searchpaths;

	// check for game override
	fs_gamedirvar = Cvar_Get("game", "", CVAR_LATCH | CVAR_SERVERINFO);
	if (fs_gamedirvar->string[0])
		FS_SetGamedir(fs_gamedirvar->string);
}

}
