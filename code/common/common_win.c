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

#include "common.h"
#include <Windows.h>
#include <io.h>

/*
================
Sys_Milliseconds
================
*/
int	curtime;
int Sys_Milliseconds (void)
{
	static LARGE_INTEGER	currentTime;
	static LARGE_INTEGER	startTime;

	static double			timeToMS;
	static qboolean			initialized = false;

	if (!initialized)
	{
		QueryPerformanceFrequency (&currentTime);
		timeToMS = 1e3 / currentTime.QuadPart;

		QueryPerformanceCounter (&startTime);

		initialized = true;
	}

	QueryPerformanceCounter (&currentTime);

	return (curtime = (currentTime.QuadPart - startTime.QuadPart) * timeToMS);
}
