/*
=============================================================

Hunk allocation

=============================================================
*/

#include "gl_local.hpp"
#include <Windows.h>

static int		g_hunkcount;

static byte*	g_membase;
static int		g_hunkmaxsize;
static int		g_cursize;

void *Hunk_Begin (int maxsize)
{
	// reserve a huge chunk of memory, but don't commit any yet
	g_cursize = 0;
	g_hunkmaxsize = maxsize;

	g_membase = (byte*)VirtualAlloc (NULL, maxsize, MEM_RESERVE, PAGE_NOACCESS);

	if (!g_membase)
		ri.Sys_Error (ERR_FATAL, "VirtualAlloc reserve failed");

	return (void *)g_membase;
}

void *Hunk_Alloc (int size)
{
	void	*buf;

	// round to cacheline
	size = (size+31)&~31;

	// commit pages as needed
	buf = VirtualAlloc (g_membase, g_cursize+size, MEM_COMMIT, PAGE_READWRITE);
	if (!buf) {
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &buf, 0, NULL);
		ri.Sys_Errorf (ERR_FATAL, "VirtualAlloc commit failed.\n%s", buf);
	}

	g_cursize += size;
	if (g_cursize > g_hunkmaxsize)
		ri.Sys_Error (ERR_FATAL, "Hunk_Alloc overflow");

	return (void *)(g_membase+g_cursize-size);
}

int Hunk_End (void)
{
	// free the remaining unused virtual memory

	g_hunkcount++;
	//Com_Printf ("g_hunkcount: %i\n", g_hunkcount);
	return g_cursize;
}

void Hunk_Free (void *base)
{
	if (base) {
		VirtualFree (base, 0, MEM_RELEASE);
	}

	g_hunkcount--;
}
