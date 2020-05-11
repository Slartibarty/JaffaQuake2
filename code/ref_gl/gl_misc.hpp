/*
=============================================================

Misc functions that don't fit anywhere else

=============================================================
*/

#pragma once

// Creates the default texture
void R_InitParticleTexture(void);

// Called by the engine when the viewport changes size
void R_WindowSize(int width, int height);

// Captures a screenshot
void GL_ScreenShot_f(void);

// Prints some OpenGL strings
void GL_Strings_f(void);

// Sets some OpenGL state variables
void GL_SetDefaultState(void);
