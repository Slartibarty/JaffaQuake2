/*
=============================================================

OpenGL refresh precompiled header file

=============================================================
*/

#pragma once

// CSTD
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <cassert>

// GL
#include "GL/glew.h"

// Our stuff
#include "../common/refresh.h"
#include "../common/qfiles.h"

#define	REF_VERSION	"GL 0.05"

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2

//====================================================================

/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

*/

enum imagetype_t
{
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky
};

struct msurface_t;

struct image_s
{
	char				name[MAX_QPATH];				// Filename relative to game folder
	imagetype_t			type;
	int					width, height;
	GLuint				texnum;							// GL texture ID
	int					registration_sequence;			// 0 = free
	msurface_t			*texturechain;					// for sort-by-texture world drawing
	float				sl, tl, sh, th;					// 0,0 - 1,1 unless part of the scrap
};
// Slart: Glue
using image_t = image_s;

#define	TEXNUM_LIGHTMAPS	1024
#define	TEXNUM_IMAGES		1153

#define MAX_GLTEXTURES		2048

#define BACKFACE_EPSILON	0.01f

#include "gl_misc.hpp"

#include "gl_image.hpp"

#include "gl_draw.hpp"

#include "gl_model.hpp"

#include "gl_hunk.hpp"

#include "gl_light.hpp"

#include "gl_surf.hpp"

#include "gl_warp.hpp"

#include "gl_mesh.hpp"

#include "gl_main.hpp"

#include "gl_imp.hpp"

//====================================================================
