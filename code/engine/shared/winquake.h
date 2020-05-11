// winquake.h: Win32-specific Quake header file

#pragma once

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

extern HINSTANCE	g_hInstance;

extern HWND			cl_hwnd;
extern qboolean		ActiveApp, Minimized;

// For the wndproc
void IN_MouseEvent (int mstate);

#ifdef __cplusplus
}
#endif
