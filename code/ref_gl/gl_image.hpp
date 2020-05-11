/*
=============================================================

Image loading and processing

=============================================================
*/

#pragma once

// Globals

extern image_t		gltextures[MAX_GLTEXTURES];
extern int			numgltextures;

extern uint			d_8to24table[256];

extern int			gl_filter_min, gl_filter_max;

//
// Helpers
//

// Swap an image's bits from RGBA to BGRA, or vice versa
void R_SwapImage(int width, int height, byte* data);

// These are mostly defunct now that it isn't 1997

void GL_Bind(GLuint texnum);
void GL_MBind(GLenum target, GLuint texnum);
void GL_TexEnv(GLint value);
void GL_EnableMultitexture(qboolean enable);
void GL_SelectTexture(GLenum texture);

//
// Main
//

// Create an image from a name, raw 32-bit pixels, and misc
image_t* R_CreateImage(const char* name, const byte* pic, int width, int height, imagetype_t type);
// Find an image by name, otherwise, create
// type is used when creating the image
image_t* GL_FindImage(const char* name, imagetype_t type);

// Register a skin
image_t* R_RegisterSkin(const char* name);

// Free up images that aren't registered
void GL_FreeUnusedImages(void);

// Load the legacy shared palette
int R_GetPalette(void);

// List all loaded images
void GL_ImageList_f(void);

// Change image filter mode
void GL_TextureMode(const char* string);
// Change image anisotropy mode
void GL_TextureAnisoMode(float mode);

// Init image system
void GL_InitImages(void);
// Shutdown image system
void GL_ShutdownImages(void);



