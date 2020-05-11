/*
=============================================================

OS specific OpenGL stuff

=============================================================
*/

#pragma once

void	GLimp_BeginFrame(void);
void	GLimp_EndFrame(void);

void	GLimp_SetGamma(byte* red, byte* green, byte* blue);
void	GLimp_RestoreGamma(void);

int 	GLimp_Init(void* hinstance, void* hWnd);
void	GLimp_Shutdown(void);

void	GLimp_AppActivate(qboolean active);
