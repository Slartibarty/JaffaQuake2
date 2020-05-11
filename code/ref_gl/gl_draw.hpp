/*
=============================================================

Functions related to drawing 2D imagery

=============================================================
*/

#pragma once

// Init
void Draw_InitLocal(void);

// Draws a character at the specified coordinates
void Draw_Char(int x, int y, int c);

// Used only by the engine
image_t* Draw_FindPic(const char* name);

// Gets the size of an image (pic)
void Draw_GetPicSize(int* w, int* h, const char* name);

// Used to draw the console background???
void Draw_StretchPic(int x, int y, int w, int h, const char* name);

// Draw a named pic at specified coordinates
void Draw_Pic(int x, int y, const char* name);

// Draws that funny border around the game when zoomed out
void Draw_TileClear(int x, int y, int w, int h, const char* name);

// Draws a fill with specified color
void Draw_Fill(int x, int y, int w, int h, int c);

// Fades the screen
void Draw_FadeScreen(void);

// Used to draw cinematics
void Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte* data);
