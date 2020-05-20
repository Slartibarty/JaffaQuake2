/*
=============================================================

Most of the stuff in here should be in gl_misc

=============================================================
*/

#pragma once

// Globals

struct glconfig_t
{
	int		overbrightBits;
};
extern glconfig_t glConfig;

struct glstate_t
{
	int		lightmap_textures;

	GLuint	currenttextures[2];
	GLuint	currenttmu;
};
extern glstate_t	glState;

// Our connection to the engine
extern refimport_t	ri;

extern model_t*		r_worldmodel;

// Near and far
extern float		gldepthmin, gldepthmax;

// Default textures
extern image_t*		r_notexture;		// use for bad textures
extern image_t*		r_particletexture;	// little dot for particles

extern entity_t*	currententity;
extern model_t*		currentmodel;

extern cplane_t		frustum[4];

extern int			r_visframecount;	// bumped when going to a new PVS
extern int			r_framecount;		// used for dlight push checking

extern int			c_brush_polys, c_alias_polys;

// view origin
extern vec3_t	vup;
extern vec3_t	vpn;
extern vec3_t	vright;
extern vec3_t	r_origin;

extern float	r_world_matrix[16];

// screen size info
extern refdef_t	r_newrefdef;

extern int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

// The palette used by gl_draw's StretchRaw
extern uint		r_rawpalette[256];

// Functions

qboolean R_CullBox(vec3_t mins, vec3_t maxs);

void R_RotateForEntity(entity_t* e);

//===========================================================

void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

// Cvars

extern cvar_t* r_norefresh;
extern cvar_t* r_drawentities;
extern cvar_t* r_drawworld;
extern cvar_t* r_speeds;
extern cvar_t* r_fullbright;
extern cvar_t* r_novis;
extern cvar_t* r_nocull;
extern cvar_t* r_lerpmodels;
extern cvar_t* r_lefthand;

extern cvar_t* r_lightlevel;	// FIXME: This is a HACK to get the client's light level

extern cvar_t* gl_nosubimage;

extern cvar_t* gl_vertex_arrays;

extern cvar_t* gl_particle_min_size;
extern cvar_t* gl_particle_max_size;
extern cvar_t* gl_particle_size;
extern cvar_t* gl_particle_att_a;
extern cvar_t* gl_particle_att_b;
extern cvar_t* gl_particle_att_c;

extern cvar_t* gl_ext_swapinterval;
extern cvar_t* gl_ext_multitexture;
extern cvar_t* gl_ext_pointparameters;
extern cvar_t* gl_ext_compiled_vertex_array;

extern cvar_t* gl_lightmap;
extern cvar_t* gl_shadows;
extern cvar_t* gl_dynamic;
extern cvar_t* gl_modulate;
extern cvar_t* gl_showtris;
extern cvar_t* gl_finish;
extern cvar_t* gl_clear;
extern cvar_t* gl_cull;
extern cvar_t* gl_polyblend;
extern cvar_t* gl_flashblend;
extern cvar_t* gl_playermip;
extern cvar_t* gl_saturatelighting;
extern cvar_t* gl_swapinterval;
extern cvar_t* gl_texturemode;
extern cvar_t* gl_textureanisomode;
extern cvar_t* gl_multisamples;
extern cvar_t* gl_allowreplace;
extern cvar_t* gl_lockpvs;

extern cvar_t* vid_fullscreen;
extern cvar_t* vid_gamma;
extern cvar_t* vid_ref;
