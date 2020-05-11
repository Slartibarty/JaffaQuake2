/*
=============================================================

Most of the stuff in here should be in gl_misc

=============================================================
*/

#include "gl_local.hpp"

// Globals

refimport_t	ri;

model_t		*r_worldmodel;

float		gldepthmin, gldepthmax;

glconfig_t	glConfig;
glstate_t	glState;

image_t		*r_notexture;		// use for bad textures
image_t		*r_particletexture;	// little dot for particles

entity_t	*currententity;
model_t		*currentmodel;

cplane_t	frustum[4];

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

int			c_brush_polys, c_alias_polys;

static float v_blend[4];		// final blending color

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
//static float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_lefthand;

cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

cvar_t	*gl_nosubimage;
cvar_t	*gl_allow_software;

cvar_t	*gl_vertex_arrays;

cvar_t	*gl_particle_min_size;
cvar_t	*gl_particle_max_size;
cvar_t	*gl_particle_size;
cvar_t	*gl_particle_att_a;
cvar_t	*gl_particle_att_b;
cvar_t	*gl_particle_att_c;

cvar_t	*gl_ext_swapinterval;
cvar_t	*gl_ext_multitexture;
cvar_t	*gl_ext_pointparameters;
cvar_t	*gl_ext_compiled_vertex_array;

cvar_t	*gl_lightmap;
cvar_t	*gl_shadows;
cvar_t	*gl_dynamic;
cvar_t	*gl_modulate;
cvar_t	*gl_showtris;
cvar_t	*gl_finish;
cvar_t	*gl_clear;
cvar_t	*gl_cull;
cvar_t	*gl_polyblend;
cvar_t	*gl_flashblend;
cvar_t	*gl_playermip;
cvar_t  *gl_saturatelighting;
cvar_t	*gl_swapinterval;
cvar_t	*gl_texturemode;
cvar_t	*gl_textureanisomode;
cvar_t	*gl_multisamples;
cvar_t	*gl_allowreplace;
cvar_t	*gl_lockpvs;

cvar_t	*vid_fullscreen;
cvar_t	*vid_gamma;
cvar_t	*vid_ref;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	if (r_nocull->value)
		return false;

	for (i=0 ; i<4 ; i++)
		if ( BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}


void R_RotateForEntity (entity_t *e)
{
    glTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    glRotatef (e->angles[1],  0, 0, 1);
    glRotatef (-e->angles[0],  0, 1, 0);
    glRotatef (-e->angles[2],  1, 0, 0);
}

/*
=============================================================

	SPRITE MODELS!

=============================================================
*/


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	float alpha = 1.0f;
	vec3_t	point;
	dsprframe_t	*frame;
	float		*up, *right;
	dsprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	psprite = (dsprite_t *)currentmodel->extradata;

#if 0
	if (e->frame < 0 || e->frame >= psprite->numframes)
	{
		ri.Con_Printf (PRINT_ALL, "no such sprite frame %i\n", e->frame);
		e->frame = 0;
	}
#endif
	e->frame %= psprite->numframes;

	frame = &psprite->frames[e->frame];

#if 0
	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
	vec3_t		v_forward, v_right, v_up;

	AngleVectors (currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
#endif
	{	// normal sprite
		up = vup;
		right = vright;
	}

	if ( e->flags & RF_TRANSLUCENT )
		alpha = e->alpha;

	if ( alpha != 1.0F )
		glEnable( GL_BLEND );

	glColor4f( 1, 1, 1, alpha );

    GL_Bind(currentmodel->skins[e->frame]->texnum);

	GL_TexEnv( GL_MODULATE );

	if ( alpha == 1.0 )
		glEnable (GL_ALPHA_TEST);
	else
		glDisable( GL_ALPHA_TEST );

	glBegin (GL_QUADS);

	glTexCoord2f (0, 1);
	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, point);
	glVertex3fv (point);

	glTexCoord2f (0, 0);
	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 0);
	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 1);
	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, point);
	glVertex3fv (point);
	
	glEnd ();

	glDisable (GL_ALPHA_TEST);
	GL_TexEnv( GL_REPLACE );

	if ( alpha != 1.0F )
		glDisable( GL_BLEND );

	glColor4f( 1, 1, 1, 1 );
}

//==================================================================================

/*
=============
R_DrawNullModel
=============
*/
static void R_DrawNullModel (void)
{
	vec3_t	shadelight;
	int		i;

	if ( currententity->flags & RF_FULLBRIGHT )
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	else
		R_LightPoint (currententity->origin, shadelight);

    glPushMatrix ();
	R_RotateForEntity (currententity);

	glDisable (GL_TEXTURE_2D);
	glColor3fv (shadelight);

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (0, 0, -16);
	for (i=0 ; i<=4 ; i++)
		glVertex3f (16*cosf(i*M_PI/2), 16*sinf(i*M_PI/2), 0);
	glEnd ();

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (0, 0, 16);
	for (i=4 ; i>=0 ; i--)
		glVertex3f (16*cosf(i*M_PI/2), 16*sinf(i*M_PI/2), 0);
	glEnd ();

	glColor3f (1,1,1);
	glPopMatrix ();
	glEnable (GL_TEXTURE_2D);
}

/*
=============
R_DrawBeam
=============
*/
static void R_DrawBeam( entity_t *e )
{
#define NUM_BEAM_SEGS 6

	int	i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->frame / 2, perpvec );

	for ( i = 0; i < 6; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0f/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	glDisable( GL_TEXTURE_2D );
	glEnable( GL_BLEND );
	glDepthMask( GL_FALSE );

	r = ( d_8to24table[e->skinnum & 0xFF] ) & 0xFF;
	g = ( d_8to24table[e->skinnum & 0xFF] >> 8 ) & 0xFF;
	b = ( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF;

	r *= 1/255.0F;
	g *= 1/255.0F;
	b *= 1/255.0F;

	glColor4f( r, g, b, e->alpha );

	glBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		glVertex3fv( start_points[i] );
		glVertex3fv( end_points[i] );
		glVertex3fv( start_points[(i+1)%NUM_BEAM_SEGS] );
		glVertex3fv( end_points[(i+1)%NUM_BEAM_SEGS] );
	}
	glEnd();

	glEnable( GL_TEXTURE_2D );
	glDisable( GL_BLEND );
	glDepthMask( GL_TRUE );
}

/*
=============
R_DrawEntitiesOnList
=============
*/
static void R_DrawEntitiesOnList (void)
{
	int		i;

	if (!r_drawentities->value)
		return;

	// draw non-transparent first
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// solid

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;
			if (!currentmodel)
			{
				R_DrawNullModel ();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel (currententity);
				break;
			case mod_brush:
				R_DrawBrushModel (currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel (currententity);
				break;
			default:
				ri.Sys_Error (ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	glDepthMask (0);		// no z writes
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (!(currententity->flags & RF_TRANSLUCENT))
			continue;	// solid

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;

			if (!currentmodel)
			{
				R_DrawNullModel ();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel (currententity);
				break;
			case mod_brush:
				R_DrawBrushModel (currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel (currententity);
				break;
			default:
				ri.Sys_Error (ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}
	glDepthMask (1);		// back to writing

}

/*
** GL_DrawParticles
**
*/
static void GL_DrawParticles( int num_particles, const particle_t particles[], const unsigned colortable[768] )
{
	const particle_t *p;
	int				i;
	vec3_t			up, right;
	float			scale;
	byte			color[4];

    GL_Bind(r_particletexture->texnum);
	glDepthMask( GL_FALSE );		// no z buffering
	glEnable( GL_BLEND );
	GL_TexEnv( GL_MODULATE );
	glBegin( GL_TRIANGLES );

	VectorScale (vup, 1.5f, up);
	VectorScale (vright, 1.5f, right);

	for ( p = particles, i=0 ; i < num_particles ; i++,p++)
	{
		// hack a scale up to keep particles from disapearing
		scale = ( p->origin[0] - r_origin[0] ) * vpn[0] + 
			    ( p->origin[1] - r_origin[1] ) * vpn[1] +
			    ( p->origin[2] - r_origin[2] ) * vpn[2];

		if (scale < 20)
			scale = 1;
		else
			scale = 1 + scale * 0.004f;

		*(int *)color = colortable[p->color];
		color[3] = p->alpha*255;

		glColor4ubv( color );

		glTexCoord2f( 0.0625f, 0.0625f );
		glVertex3fv( p->origin );

		glTexCoord2f( 1.0625f, 0.0625f );
		glVertex3f( p->origin[0] + up[0]*scale, 
			         p->origin[1] + up[1]*scale, 
					 p->origin[2] + up[2]*scale);

		glTexCoord2f( 0.0625f, 1.0625f );
		glVertex3f( p->origin[0] + right[0]*scale, 
			         p->origin[1] + right[1]*scale, 
					 p->origin[2] + right[2]*scale);
	}

	glEnd ();
	glDisable( GL_BLEND );
	glColor4f( 1,1,1,1 );
	glDepthMask( 1 );		// back to normal Z buffering
	GL_TexEnv( GL_REPLACE );
}

/*
===============
R_DrawParticles
===============
*/
static void R_DrawParticles (void)
{
	if ( GLEW_EXT_point_parameters && gl_ext_pointparameters->value)
	{
		int i;
		unsigned char color[4];
		const particle_t *p;

		glDepthMask( GL_FALSE );
		glEnable( GL_BLEND );
		glDisable( GL_TEXTURE_2D );

		glPointSize( gl_particle_size->value );

		glBegin( GL_POINTS );
		for ( i = 0, p = r_newrefdef.particles; i < r_newrefdef.num_particles; i++, p++ )
		{
			*(int *)color = d_8to24table[p->color];
			color[3] = p->alpha*255;

			glColor4ubv( color );

			glVertex3fv( p->origin );
		}
		glEnd();

		glDisable( GL_BLEND );
		glColor4f( 1.0F, 1.0F, 1.0F, 1.0F );
		glDepthMask( GL_TRUE );
		glEnable( GL_TEXTURE_2D );

	}
	else
	{
		GL_DrawParticles( r_newrefdef.num_particles, r_newrefdef.particles, d_8to24table );
	}
}

/*
============
R_PolyBlend
============
*/
static void R_PolyBlend (void)
{
	if (!gl_polyblend->value)
		return;
	if (!v_blend[3])
		return;

	glDisable (GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_TEXTURE_2D);

    glLoadIdentity ();

	// FIXME: get rid of these
    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up

	glColor4fv (v_blend);

	glBegin (GL_QUADS);

	glVertex3f (10, 100, 100);
	glVertex3f (10, -100, 100);
	glVertex3f (10, -100, -100);
	glVertex3f (10, 100, -100);
	glEnd ();

	glDisable (GL_BLEND);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_ALPHA_TEST);

	glColor4f(1,1,1,1);
}

//=======================================================================

static int SignbitsForPlane (cplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


static void R_SetFrustum (void)
{
#if 0
	/*
	** this code is wrong, since it presume a 90 degree FOV both in the
	** horizontal and vertical plane
	*/
	// front side is visible
	VectorAdd (vpn, vright, frustum[0].normal);
	VectorSubtract (vpn, vright, frustum[1].normal);
	VectorAdd (vpn, vup, frustum[2].normal);
	VectorSubtract (vpn, vup, frustum[3].normal);

	// we theoretically don't need to normalize these vectors, but I do it
	// anyway so that debugging is a little easier
	VectorNormalize( frustum[0].normal );
	VectorNormalize( frustum[1].normal );
	VectorNormalize( frustum[2].normal );
	VectorNormalize( frustum[3].normal );
#else
	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_newrefdef.fov_x / 2 ) );
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_newrefdef.fov_x / 2 );
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_newrefdef.fov_y / 2 );
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_newrefdef.fov_y / 2 ) );
#endif

	for (int i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
static void R_SetupFrame (void)
{
	int i;
	mleaf_t	*leaf;

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_newrefdef.vieworg, r_origin);

	AngleVectors (r_newrefdef.viewangles, vpn, vright, vup);

// current viewcluster
	if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf (r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	for (i=0 ; i<4 ; i++)
		v_blend[i] = r_newrefdef.blend[i];

	c_brush_polys = 0;
	c_alias_polys = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
	{
		glEnable( GL_SCISSOR_TEST );
		glScissor( r_newrefdef.x, r_newrefdef.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glDisable( GL_SCISSOR_TEST );
	}
}


void MYgluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan( fovy * M_PID / 360.0 );
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   xmin += -(2.0 * 0.0) / zNear;
   xmax += -(2.0 * 0.0) / zNear;

   glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}


/*
=============
R_SetupGL
=============
*/
static void R_SetupGL (void)
{
	//
	// set up projection matrix
	//
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
    MYgluPerspective (r_newrefdef.fov_y, (GLdouble)r_newrefdef.width / (GLdouble)r_newrefdef.height, 4.0, 4096.0);

	glCullFace(GL_FRONT);

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();

    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up
    glRotatef (-r_newrefdef.viewangles[2],  1, 0, 0);
    glRotatef (-r_newrefdef.viewangles[0],  0, 1, 0);
    glRotatef (-r_newrefdef.viewangles[1],  0, 0, 1);
    glTranslatef (-r_newrefdef.vieworg[0],  -r_newrefdef.vieworg[1],  -r_newrefdef.vieworg[2]);

	glGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	if (gl_cull->value)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
}

/*
=============
R_Clear
=============
*/
static void R_Clear (void)
{
	if (gl_clear->value)
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else
		glClear (GL_DEPTH_BUFFER_BIT);
	gldepthmin = 0.0f;
	gldepthmax = 1.0f;
	glDepthFunc (GL_LEQUAL);

	glDepthRange (gldepthmin, gldepthmax);
}

static void R_Flash( void )
{
	R_PolyBlend ();
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
static void R_RenderView (refdef_t *fd)
{
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		ri.Sys_Error (ERR_DROP, "R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	R_PushDlights ();

	if (gl_finish->value)
		glFinish ();

	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();

//	GLTEMP_DrawTestModel ();

	R_DrawEntitiesOnList ();

	R_RenderDlights ();

	R_DrawParticles ();

	R_DrawAlphaSurfaces ();

	R_Flash();

	if (r_speeds->value)
	{
		ri.Con_Printf (PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
			c_brush_polys, 
			c_alias_polys, 
			c_visible_textures, 
			c_visible_lightmaps); 
	}
}


static void R_SetGL2D (void)
{
	// set 2D virtual screen size
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	glOrtho  (0, r_newrefdef.width, r_newrefdef.height, 0, -99999, 99999);
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glDisable (GL_BLEND);
	glEnable (GL_ALPHA_TEST);
	glColor4f (1,1,1,1);
}


/*
====================
R_SetLightLevel

====================
*/
static void R_SetLightLevel (void)
{
	vec3_t		shadelight;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint (r_newrefdef.vieworg, shadelight);

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
			r_lightlevel->value = 150*shadelight[0];
		else
			r_lightlevel->value = 150*shadelight[2];
	}
	else
	{
		if (shadelight[1] > shadelight[2])
			r_lightlevel->value = 150*shadelight[1];
		else
			r_lightlevel->value = 150*shadelight[2];
	}

}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
static void R_RenderFrame (refdef_t *fd)
{
	R_RenderView( fd );
	R_SetLightLevel ();
	R_SetGL2D ();
}


static void R_Register( void )
{
	r_lefthand = ri.Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	r_norefresh = ri.Cvar_Get ("r_norefresh", "0", 0);
	r_fullbright = ri.Cvar_Get ("r_fullbright", "0", 0);
	r_drawentities = ri.Cvar_Get ("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get ("r_drawworld", "1", 0);
	r_novis = ri.Cvar_Get ("r_novis", "0", 0);
	r_nocull = ri.Cvar_Get ("r_nocull", "0", 0);
	r_lerpmodels = ri.Cvar_Get ("r_lerpmodels", "1", 0);
	r_speeds = ri.Cvar_Get ("r_speeds", "0", 0);

	r_lightlevel = ri.Cvar_Get ("r_lightlevel", "0", 0);

	gl_nosubimage = ri.Cvar_Get( "gl_nosubimage", "0", 0 );
	gl_allow_software = ri.Cvar_Get( "gl_allow_software", "0", 0 );

	gl_particle_min_size = ri.Cvar_Get( "gl_particle_min_size", "2", CVAR_ARCHIVE );
	gl_particle_max_size = ri.Cvar_Get( "gl_particle_max_size", "40", CVAR_ARCHIVE );
	gl_particle_size = ri.Cvar_Get( "gl_particle_size", "40", CVAR_ARCHIVE );
	gl_particle_att_a = ri.Cvar_Get( "gl_particle_att_a", "0.01", CVAR_ARCHIVE );
	gl_particle_att_b = ri.Cvar_Get( "gl_particle_att_b", "0.0", CVAR_ARCHIVE );
	gl_particle_att_c = ri.Cvar_Get( "gl_particle_att_c", "0.01", CVAR_ARCHIVE );

	gl_modulate = ri.Cvar_Get ("gl_modulate", "1", CVAR_ARCHIVE );
	gl_lightmap = ri.Cvar_Get ("gl_lightmap", "0", 0);
	gl_shadows = ri.Cvar_Get ("gl_shadows", "0", CVAR_ARCHIVE );
	gl_dynamic = ri.Cvar_Get ("gl_dynamic", "1", 0);
	gl_showtris = ri.Cvar_Get ("gl_showtris", "0", 0);
	gl_finish = ri.Cvar_Get ("gl_finish", "0", CVAR_ARCHIVE);
	gl_clear = ri.Cvar_Get ("gl_clear", "1", 0);
	gl_cull = ri.Cvar_Get ("gl_cull", "1", 0);
	gl_polyblend = ri.Cvar_Get ("gl_polyblend", "1", 0);
	gl_flashblend = ri.Cvar_Get ("gl_flashblend", "0", 0);
	gl_playermip = ri.Cvar_Get ("gl_playermip", "0", 0);
	gl_texturemode = ri.Cvar_Get( "gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
	gl_textureanisomode = ri.Cvar_Get( "gl_textureanisomode", "1", CVAR_ARCHIVE );
	gl_multisamples = ri.Cvar_Get("gl_multisamples", "1", CVAR_ARCHIVE);
	gl_allowreplace = ri.Cvar_Get("gl_allowreplace", "1", CVAR_ARCHIVE);
	gl_lockpvs = ri.Cvar_Get( "gl_lockpvs", "0", 0 );

	gl_vertex_arrays = ri.Cvar_Get( "gl_vertex_arrays", "0", CVAR_ARCHIVE );

	gl_ext_swapinterval = ri.Cvar_Get( "gl_ext_swapinterval", "0", CVAR_ARCHIVE );
	gl_ext_multitexture = ri.Cvar_Get( "gl_ext_multitexture", "0", CVAR_ARCHIVE );
	gl_ext_pointparameters = ri.Cvar_Get( "gl_ext_pointparameters", "1", CVAR_ARCHIVE );
	gl_ext_compiled_vertex_array = ri.Cvar_Get( "gl_ext_compiled_vertex_array", "1", CVAR_ARCHIVE );

	gl_swapinterval = ri.Cvar_Get( "gl_swapinterval", "0", CVAR_ARCHIVE );

	gl_saturatelighting = ri.Cvar_Get( "gl_saturatelighting", "1", 0 );

	vid_fullscreen = ri.Cvar_Get( "vid_fullscreen", "0", CVAR_ARCHIVE );
	vid_gamma = ri.Cvar_Get( "vid_gamma", "1.0", CVAR_ARCHIVE );
	vid_ref = ri.Cvar_Get( "vid_ref", "gl", CVAR_ARCHIVE );

	ri.Cmd_AddCommand( "imagelist", GL_ImageList_f );
	ri.Cmd_AddCommand( "screenshot", GL_ScreenShot_f );
	ri.Cmd_AddCommand( "modellist", Mod_Modellist_f );
	ri.Cmd_AddCommand( "gl_strings", GL_Strings_f );
}

/*
===============
R_Init
===============
*/
static int R_Init( void *hInstance, void *wndproc )
{
	// Init turbosin
	extern float r_turbsin[256];
	for ( int j = 0; j < 256; ++j )
	{
		r_turbsin[j] *= 0.5f;
	}

	ri.Con_Print (PRINT_ALL, "ref_gl version: " REF_VERSION "\n");

	R_GetPalette ();

	R_Register();

	// initialize OS-specific parts of OpenGL
	if ( !GLimp_Init( hInstance, wndproc ) )
	{
		return -1;
	}

//	if (!GL_InitShaders ())
//	{
//		return -1;
//	}

	ri.Vid_MenuInit();

	ri.Cvar_Set( "scr_drawall", "0" );

#ifdef __linux__
	ri.Cvar_Set( "gl_finish", "1" );
#endif

	GL_SetDefaultState();

	GL_InitImages ();
	R_InitParticleTexture();
	Mod_Init ();
	Draw_InitLocal ();

	auto err = glGetError();
	assert (err == GL_NO_ERROR);
	if ( err != GL_NO_ERROR )
		ri.Con_Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);

	return 1;
}

/*
===============
R_Shutdown
===============
*/
static void R_Shutdown (void)
{	
	ri.Cmd_RemoveCommand ("modellist");
	ri.Cmd_RemoveCommand ("screenshot");
	ri.Cmd_RemoveCommand ("imagelist");
	ri.Cmd_RemoveCommand ("gl_strings");

	Mod_FreeAll ();

	GL_ShutdownImages ();

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GLimp_Shutdown();
}



/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
static void R_BeginFrame( void )
{
	/*
	** update 3Dfx gamma -- it is expected that a user will do a vid_restart
	** after tweaking this value
	*/
	if ( vid_gamma->modified )
	{
		// slarttodo
		vid_gamma->modified = false;

	}

	GLimp_BeginFrame();

	/*
	** go into 2D mode
	*/
	R_SetGL2D ();

	/*
	** texturemode stuff
	*/
	if (gl_multisamples->modified)
	{
		gl_multisamples->modified = false;
		vid_ref->modified = true;
	}
	if ( gl_texturemode->modified )
	{
		gl_texturemode->modified = false;
		GL_TextureMode( gl_texturemode->string );
	}
	if (gl_textureanisomode->modified)
	{
		gl_textureanisomode->modified = false;
		GL_TextureAnisoMode(gl_textureanisomode->value);
	}

	//
	// clear screen if desired
	//
	R_Clear ();
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_EndFrame
@@@@@@@@@@@@@@@@@@@@@
*/
static void R_EndFrame(void)
{
	GLenum err = glGetError();
	assert(err == GL_NO_ERROR);

	GLimp_EndFrame();
}

/*
=============
R_SetPalette
=============
*/
uint r_rawpalette[256];
static void R_SetPalette (const unsigned char *palette)
{
	int i;

	byte *rp = ( byte * ) r_rawpalette;

	if ( palette )
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = palette[i*3+0];
			rp[i*4+1] = palette[i*3+1];
			rp[i*4+2] = palette[i*3+2];
			rp[i*4+3] = 0xff;
		}
	}
	else
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = d_8to24table[i] & 0xff;
			rp[i*4+1] = ( d_8to24table[i] >> 8 ) & 0xff;
			rp[i*4+2] = ( d_8to24table[i] >> 16 ) & 0xff;
			rp[i*4+3] = 0xff;
		}
	}
}

//===================================================================

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
extern "C" EXPORT refexport_t GetRefAPI (refimport_t rimp)
{
	refexport_t	re;

	ri = rimp;

	re.api_version = API_VERSION;

	re.BeginRegistration = R_BeginRegistration;
	re.RegisterModel = R_RegisterModel;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = Draw_FindPic;
	re.SetSky = R_SetSky;
	re.EndRegistration = R_EndRegistration;

	re.RenderFrame = R_RenderFrame;

	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawChar = Draw_Char;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFadeScreen= Draw_FadeScreen;

	re.DrawStretchRaw = Draw_StretchRaw;

	re.Init = R_Init;
	re.Shutdown = R_Shutdown;

	re.CinematicSetPalette = R_SetPalette;
	re.BeginFrame = R_BeginFrame;
	re.EndFrame = R_EndFrame;

	re.AppActivate = GLimp_AppActivate;
	re.WindowSize = R_WindowSize;

	Swap_Init ();

	return re;
}


#ifndef REF_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Com_Print (const char *msg)
{
	ri.Con_Print (PRINT_ALL, msg);
}
#endif
