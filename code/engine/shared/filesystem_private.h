/*
=============================================================================

QUAKE FILESYSTEM

PRIVATE - DO NOT TOUCH - OR I WILL CALL THE POLICE

=============================================================================
*/

#pragma once

/*

All of Quake's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

*/

//
// In memory
//
// On disk versions can be found in qfiles.h
//

struct packfile_t
{
	char		name[MAX_QPATH];
	int			filepos, filelen;
};

struct pack_t
{
	char			filename[MAX_OSPATH];
	fshandle_t		handle;
	int				numfiles;
	packfile_t*		files;
};

struct filelink_t
{
	filelink_t*		next;
	char*			from;
	int				fromlength;
	char*			to;
};

struct searchpath_t
{
	char			filename[MAX_OSPATH];
	pack_t*			pack;		// only one of filename / pack will be used
	searchpath_t*	next;
};

// Things used by filesystem_shared
// But defined in filesystem_OSspecific

// These globals must be implemented in each OS specific filesystem file
extern char				fs_gamedir[MAX_OSPATH];
extern cvar_t*			fs_basedir;
extern cvar_t*			fs_gamedirvar;

extern filelink_t*		fs_links;

extern searchpath_t*	fs_searchpaths;
extern searchpath_t*	fs_base_searchpaths;	// without gamedirs

qboolean FS_Mkdir_Private(const char* path);

qboolean FS_CopyFile_Private(const char* src, const char* dest);

qboolean FS_DeleteFile_Private(const char* filename);

pack_t* FS_LoadPackFile(const char* packfile);

// Things used by filesystem_OSspecific
// But defined in filesystem_shared
