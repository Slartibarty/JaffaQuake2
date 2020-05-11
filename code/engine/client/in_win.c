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
// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

#include "client.h"
#include "../shared/winquake.h"

#define MOUSE_BUTTONS	3

extern	uint	sys_msg_time;

cvar_t	*in_mouse;
cvar_t	*in_joystick;

qboolean	in_appactive;

/*
============================================================

  MOUSE CONTROL

============================================================
*/

// mouse variables
cvar_t*		m_filter;

int			mouse_oldbuttonstate;
int			mouse_x, mouse_y, old_mouse_x, old_mouse_y, mx_accum, my_accum;

int			old_x, old_y;

qboolean	mouseactive;	// false when not focus app

qboolean	restore_spi;
qboolean	mouseinitialized;
int			originalmouseparms[3], newmouseparms[3] = {0, 0, 0}; // Slart: Was 0, 0, 1 - 0 disables mouse acceleration
qboolean	mouseparmsvalid;

int			window_center_x, window_center_y;


/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void IN_ActivateMouse (void)
{
	if (!mouseinitialized)
		return;
	if (!in_mouse->value)
	{
		mouseactive = false;
		return;
	}
	if (mouseactive)
		return;

	mouseactive = true;

	if (mouseparmsvalid)
		restore_spi = SystemParametersInfoW (SPI_SETMOUSE, 0, newmouseparms, 0);

	int width = GetSystemMetrics (SM_CXSCREEN);
	int height = GetSystemMetrics (SM_CYSCREEN);

	RECT window_rect;
	GetWindowRect ( cl_hwnd, &window_rect);
	if (window_rect.left < 0)
		window_rect.left = 0;
	if (window_rect.top < 0)
		window_rect.top = 0;
	if (window_rect.right >= width)
		window_rect.right = width-1;
	if (window_rect.bottom >= height-1)
		window_rect.bottom = height-1;

	window_center_x = (window_rect.right + window_rect.left)/2;
	window_center_y = (window_rect.top + window_rect.bottom)/2;

	SetCursorPos (window_center_x, window_center_y);

	old_x = window_center_x;
	old_y = window_center_y;

	SetCapture ( cl_hwnd );
	ClipCursor (&window_rect);
	while (ShowCursor (FALSE) >= 0)
		;
}

/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
void IN_DeactivateMouse (void)
{
	if (!mouseinitialized)
		return;
	if (!mouseactive)
		return;

	if (restore_spi)
		SystemParametersInfoW (SPI_SETMOUSE, 0, originalmouseparms, 0);

	mouseactive = false;

	ClipCursor (NULL);
	ReleaseCapture ();
	while (ShowCursor (TRUE) < 0)
		;
}

/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse (void)
{
	cvar_t* cv;

	cv = Cvar_Get ("in_initmouse", "1", CVAR_NOSET);
	if ( !cv->value ) 
		return; 

	mouseinitialized = true;
	mouseparmsvalid = SystemParametersInfoW (SPI_GETMOUSE, 0, originalmouseparms, 0);
}

/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent (int mstate)
{
	if (!mouseinitialized)
		return;

// perform button actions
	for (int i=0 ; i<MOUSE_BUTTONS ; i++)
	{
		if ( (mstate & (1<<i)) &&
			!(mouse_oldbuttonstate & (1<<i)) )
		{
			Key_Event (K_MOUSE1 + i, true, sys_msg_time);
		}

		if ( !(mstate & (1<<i)) &&
			(mouse_oldbuttonstate & (1<<i)) )
		{
			Key_Event (K_MOUSE1 + i, false, sys_msg_time);
		}
	}	
		
	mouse_oldbuttonstate = mstate;
}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove (usercmd_t *cmd)
{
	if (!mouseactive)
		return;

	// find mouse movement
	POINT current_pos;
	if (!GetCursorPos (&current_pos))
		return;

	int mx = current_pos.x - window_center_x;
	int my = current_pos.y - window_center_y;

	if (m_filter->value)
	{
		mouse_x = (mx + old_mouse_x) * 0.5f;
		mouse_y = (my + old_mouse_y) * 0.5f;
	}
	else
	{
		mouse_x = mx;
		mouse_y = my;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	mouse_x *= sensitivity->value;
	mouse_y *= sensitivity->value;

	// add mouse X/Y movement to cmd
	cl.viewangles[YAW] -= m_yaw->value * mouse_x;
	cl.viewangles[PITCH] += m_pitch->value * mouse_y;

	// force the mouse to the center, so there's room to move
	if (mx || my)
		SetCursorPos (window_center_x, window_center_y);
}

/*
=========================================================================

VIEW CENTERING

=========================================================================
*/

cvar_t	*v_centermove;
cvar_t	*v_centerspeed;

/*
===========
IN_Init
===========
*/
void IN_Init (void)
{
	// mouse variables
	m_filter				= Cvar_Get ("m_filter",					"0",		0);
    in_mouse				= Cvar_Get ("in_mouse",					"1",		CVAR_ARCHIVE);

	// joystick variables
	in_joystick				= Cvar_Get ("in_joystick",				"0",		CVAR_ARCHIVE);

	// centering
	v_centermove			= Cvar_Get ("v_centermove",				"0.15",		0);
	v_centerspeed			= Cvar_Get ("v_centerspeed",			"500",		0);

	IN_StartupMouse ();
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown (void)
{
	IN_DeactivateMouse ();
}

/*
===========
IN_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void IN_Activate (qboolean active)
{
	in_appactive = active;
	mouseactive = !active;		// force a new window check or turn off
}

/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame (void)
{
	if (!mouseinitialized)
		return;

	if (!in_mouse || !in_appactive)
	{
		IN_DeactivateMouse ();
		return;
	}

	if ( !cl.refresh_prepped
		|| cls.key_dest == key_console
		|| cls.key_dest == key_menu)
	{
		// temporarily deactivate if in fullscreen
		// Slart: Disable all the time in menus for now
	//	if (Cvar_VariableValue ("vid_fullscreen") == 0.0f)
	//	{
			IN_DeactivateMouse ();
			return;
	//	}
	}

	IN_ActivateMouse ();
}

/*
===========
IN_Move
===========
*/
void IN_Move (usercmd_t *cmd)
{
	IN_MouseMove (cmd);
}

/*
=========================================================================

JOYSTICK

=========================================================================
*/

/*
===========
IN_Commands
===========
*/
void IN_Commands (void)
{
}
