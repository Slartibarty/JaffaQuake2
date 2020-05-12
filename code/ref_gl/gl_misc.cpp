/*
=============================================================

Misc functions that don't fit anywhere else

=============================================================
*/

#include "gl_local.hpp"

/*
==================
R_InitParticleTexture

Create the default texture
also used when pointparameters aren't supported

This should be moved to gl_image.cpp
==================
*/
static constexpr byte dottexture[8][8]{
	{0,0,0,0,0,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};

void R_InitParticleTexture (void)
{
	int		x,y;
	byte	data[8][8][4];

	//
	// particle texture
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]*255;
		}
	}
	r_particletexture = R_CreateImage ("***particle***", (byte *)data, 8, 8, it_sprite);

	//
	// also use this for bad textures, but without alpha
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = dottexture[x&3][y&3]*255;
			data[y][x][1] = 0; // dottexture[x&3][y&3]*255;
			data[y][x][2] = 0; //dottexture[x&3][y&3]*255;
			data[y][x][3] = 255;
		}
	}
	r_notexture = R_CreateImage ("***r_notexture***", (byte *)data, 8, 8, it_wall);
}

/*
==================
R_WindowSize

Called by the engine when the viewport changes size
==================
*/
void R_WindowSize (int width, int height)
{
	glViewport (0, 0, width, height);
	r_newrefdef.width = width;
	r_newrefdef.height = height;
}

/*
================== 
GL_ScreenShot_f

Captures a screenshot
================== 
*/
void GL_ScreenShot_f (void) 
{
	byte		*buffer;
	char		picname[80]{"quake00.tga"};
	char		checkname[MAX_OSPATH]{"scrnshot"};
	fshandle_t	f;

	// create the scrnshots directory if it doesn't exist
	ri.FS_Mkdir (checkname);

	//
	// find a file name to save it to 
	//
	int i;
	for (i=0 ; i<=99 ; i++) 
	{ 
		picname[5] = i/10 + '0'; 
		picname[6] = i%10 + '0'; 
		Com_sprintf (checkname, sizeof(checkname), "scrnshot/%s", picname);
		ri.FS_OpenFile(checkname, &f);
		if (!f)
			break;	// file doesn't exist
		ri.FS_CloseFile (f);
	} 
	if (i==100) 
	{
		ri.Con_Print (PRINT_ALL, "GL_ScreenShot_f: Couldn't create a file\n"); 
		return;
 	}


	buffer = (byte*)malloc(r_newrefdef.width* r_newrefdef.height*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = r_newrefdef.width&255;
	buffer[13] = r_newrefdef.width>>8;
	buffer[14] = r_newrefdef.height&255;
	buffer[15] = r_newrefdef.height>>8;
	buffer[16] = 24;	// pixel size

	glReadPixels (0, 0, r_newrefdef.width, r_newrefdef.height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 );

	// swap rgb to bgr
	int c = 18+ r_newrefdef.width*r_newrefdef.height*3;
	int temp;
	for (i=18 ; i<c ; i+=3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	ri.FS_OpenFileWrite(checkname, &f, fs_overwrite);
	ri.FS_Write(buffer, c, f);
	ri.FS_CloseFile(f);

	free (buffer);
	ri.Con_Printf (PRINT_ALL, "Wrote %s\n", picname);
} 

/*
==================
GL_Strings_f

Prints some OpenGL strings
==================
*/
void GL_Strings_f( void )
{
	ri.Con_Printf (PRINT_ALL, "GL_VENDOR: %s\n", glGetString (GL_VENDOR) );
	ri.Con_Printf (PRINT_ALL, "GL_RENDERER: %s\n", glGetString (GL_RENDERER));
	ri.Con_Printf (PRINT_ALL, "GL_VERSION: %s\n", glGetString (GL_VERSION));
	ri.Con_Printf (PRINT_ALL, "GL_GLSL_VERSION: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));
//	ri.Con_Printf (PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string );
}

/*
==================
GL_SetDefaultState

Sets some OpenGL state variables
==================
*/
void GL_SetDefaultState( void )
{
	glClearColor (1.0f, 0.0f, 0.5f, 1.0f);
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666f);

	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glDisable (GL_BLEND);

	glColor4f (1.0f, 1.0f, 1.0f, 1.0f);

	glShadeModel (GL_FLAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv( GL_REPLACE );

	GL_TextureMode(gl_texturemode->string);
	GL_TextureAnisoMode(gl_textureanisomode->value);

	if ( GLEW_EXT_point_parameters && gl_ext_pointparameters->value )
	{
		float attenuations[3];

		attenuations[0] = gl_particle_att_a->value;
		attenuations[1] = gl_particle_att_b->value;
		attenuations[2] = gl_particle_att_c->value;

		glEnable( GL_POINT_SMOOTH );
		glPointParameterfEXT( GL_POINT_SIZE_MIN_EXT, gl_particle_min_size->value );
		glPointParameterfEXT( GL_POINT_SIZE_MAX_EXT, gl_particle_max_size->value );
		glPointParameterfvEXT( GL_DISTANCE_ATTENUATION_EXT, attenuations );
	}
}
