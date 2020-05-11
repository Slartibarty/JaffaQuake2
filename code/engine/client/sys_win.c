/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// Windows stuff

#include "../shared/engine.h"
#include <float.h>
#include "../shared/winquake.h"
#include "../shared/conproc.h"

//#define DEMO

qboolean		ActiveApp, Minimized;

uint			sys_msg_time, sys_frame_time;

static HANDLE	hinput, houtput;

#define MAX_NUM_ARGVS		128
int			argc;
char		*argv[MAX_NUM_ARGVS];


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

// Pops up an OS-specific messagebox, and exits the process
// used for final messages to the user
//
NORETURN void Sys_Error (const char *error)
{
	// Pop up a message box
	// IF we have a window, become a child of that!
	MessageBoxA(cl_hwnd ? cl_hwnd : NULL, error, NULL, MB_OK | MB_ICONERROR );

	Sys_Quit(1);
}

#if 0
NORETURN void Sys_Errorf (const char *fmt, ...)
{
	va_list		argptr;
	char		text[4096];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	Sys_Error(text);
}
#endif

// Exit process and shutdown OS specific stuff
//
NORETURN void Sys_Quit (int code)
{
	CL_Shutdown();
	Engine_Shutdown();

	if (dedicated && dedicated->value)
	{
		FreeConsole();

		// Shut down QHOST hooks if necessary
		DeinitConProc();
	}

	ExitProcess(code);
}


//================================================================


/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
	if (dedicated->value)
	{
		if (!AllocConsole ())
			Com_Error (ERR_FATAL, "Couldn't create dedicated server console");
		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	
		// let QHOST hook in
		InitConProc (argc, argv);
	}
}


static char	console_text[256];
static int	console_textlen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
	if (!dedicated || !dedicated->value)
		return NULL;

	INPUT_RECORD	recs[1024];
	int		ch;
	DWORD	dummy, numread, numevents;

	while (1)
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Com_Error (ERR_FATAL, "Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Com_Error (ERR_FATAL, "Error reading console input");

		if (numread != 1)
			Com_Error (ERR_FATAL, "Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);	

						if (console_textlen)
						{
							console_text[console_textlen] = 0;
							console_textlen = 0;
							return console_text;
						}
						break;

					case '\b':
						if (console_textlen)
						{
							console_textlen--;
							WriteFile(houtput, "\b \b", 3, &dummy, NULL);	
						}
						break;

					default:
						if (ch >= ' ')
						{
							if (console_textlen < sizeof(console_text)-2)
							{
								WriteFile(houtput, &ch, 1, &dummy, NULL);	
								console_text[console_textlen] = ch;
								console_textlen++;
							}
						}

						break;

				}
			}
		}
	}

	return NULL;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (const char *string)
{
	DWORD	dummy;
	char	text[256];

	if (!dedicated || !dedicated->value)
		return;

	if (console_textlen)
	{
		text[0] = '\r';
		memset(&text[1], ' ', console_textlen);
		text[console_textlen+1] = '\r';
		text[console_textlen+2] = 0;
		WriteFile(houtput, text, console_textlen+2, &dummy, NULL);
	}

	WriteFile(houtput, string, (DWORD)strlen(string), &dummy, NULL);

	if (console_textlen)
		WriteFile(houtput, console_text, console_textlen, &dummy, NULL);
}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void)
{
	MSG msg;

	while (PeekMessageW (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessageW (&msg, NULL, 0, 0))
			Com_Quit ();
		sys_msg_time = msg.time;
		TranslateMessage (&msg);
		DispatchMessageW (&msg);
	}

	// grab frame time 
	// Slart: Should this be TimeGetTime?
	sys_frame_time = Sys_Milliseconds();	// FIXME: should this be at start?
}



/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void )
{
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 )
	{
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 )
		{
			if ( ( cliptext = GlobalLock( hClipboardData ) ) != 0 ) 
			{
				data = malloc( GlobalSize( hClipboardData ) + 1 );
				strcpy( data, cliptext );
				GlobalUnlock( hClipboardData );
			}
		}
		CloseClipboard();
	}
	return data;
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate (void)
{
//	ShowWindow ( cl_hwnd, SW_RESTORE);
//	SetForegroundWindow ( cl_hwnd );
}

/*
========================================================================

GAME DLL

========================================================================
*/

static HINSTANCE	game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	if (!FreeLibrary (game_library))
		Com_Error (ERR_FATAL, "FreeLibrary failed for game library");
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);
	char	name[MAX_OSPATH];
	char	*path;
	char	cwd[MAX_OSPATH];

#if 1

	const char *gamename = "game" CPUSTRING ".dll";

#ifdef NDEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif

#endif

	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	// check the current debug directory first for development purposes
	GetCurrentDirectoryA (sizeof (cwd), cwd);
	Com_sprintf (name, sizeof(name), "%s\\%s\\%s", cwd, debugdir, gamename);
	game_library = LoadLibraryA ( name );
	if (game_library)
	{
		Com_DPrintf ("LoadLibrary (%s)\n", name);
	}
	else
	{
#ifdef DEBUG
		// check the current directory for other development purposes
		Com_sprintf (name, sizeof(name), "%s\\%s", cwd, gamename);
		game_library = LoadLibrary ( name );
		if (game_library)
		{
			Com_DPrintf ("LoadLibrary (%s)\n", name);
		}
		else
#endif
		{
			// now run through the search paths
			path = NULL;
			while (1)
			{
				path = FS_NextPath (path);
				if (!path)
					return NULL;		// couldn't find one anywhere
				Com_sprintf (name, sizeof(name), "%s\\%s", path, gamename);
				game_library = LoadLibraryA (name);
				if (game_library)
				{
					Com_DPrintf ("LoadLibrary (%s)\n",name);
					break;
				}
			}
		}
	}

	GetGameAPI = (void *)GetProcAddress (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame ();		
		return NULL;
	}

	return GetGameAPI (parms);
}

//=======================================================================


/*
==================
ParseCommandLine

==================
*/
void ParseCommandLine (LPSTR lpCmdLine)
{
	argc = 1;
	argv[0] = "exe";

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
			
		}
	}

}

/*
==================
WinMain

==================
*/
HINSTANCE	g_hInstance;

int APIENTRY WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int time, oldtime, newtime;

	g_hInstance = hInstance;

	// no abort/retry/fail errors
	// Slart: Was SetErrorMode
	SetThreadErrorMode (SEM_FAILCRITICALERRORS, NULL);

	ParseCommandLine (lpCmdLine);

	Engine_Init (argc, argv);
	oldtime = Sys_Milliseconds ();

	/* main window message loop */
	while (1)
	{
		// if at a full screen console, don't update unless needed
		if (Minimized || (dedicated && dedicated->value) )
		{
			Sleep (1);
		}

		do
		{
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);
		//Com_Printf ("time:%d - %d = %d\n", newtime, oldtime, time);

		//_controlfp( ~( _EM_ZERODIVIDE /*| _EM_INVALID*/ ), _MCW_EM );
#if 0
		_controlfp( _PC_24, _MCW_PC );
#endif
		Engine_Frame (time);

		oldtime = newtime;
	}

	// never gets here
	return TRUE;
}
