/*
=============================================================

Image loading and processing

SlartTodo: Move SwapImage to a per-image basis, swap 8to24table beforehand
to prevent having to SwapImage all WAL files, this might save performance somewhere
Non power of two textures aren't mipmapped correctly

=============================================================
*/

#include "gl_local.hpp"

// Use STBI for most formats
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_LINEAR
#define STBI_NO_STDIO
#define STBI_ONLY_TGA
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb/stb_image.h"

//
// Globals
//

image_t			gltextures[MAX_GLTEXTURES];
int				numgltextures;

static byte		s_intensitytable[256];
static byte		s_gammatable[256];

cvar_t*	r_intensity;
cvar_t* r_overbrightbits;

uint	d_8to24table[256];

int		gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
int		gl_filter_max = GL_LINEAR;


/*
==============
R_LoadPCX

Loads a PCX file
==============
*/
static void R_LoadPCX(byte* raw, int rawlen, byte** pic, byte** palette, int& width, int& height)
{
	pcx_t*	pcx;
	int		x, y;
	int		dataByte, runLength;
	byte*	out, * pix;
	int		xmax, ymax;

	*pic = NULL;
	*palette = NULL;
	width = 0;
	height = 0;

	//
	// parse the PCX file
	//
	pcx = (pcx_t*)raw;
	raw = &pcx->data;

	xmax = LittleShort(pcx->xmax);
	ymax = LittleShort(pcx->ymax);

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| xmax >= 1024
		|| ymax >= 1024)
	{
		ri.Con_Printf(PRINT_ALL, "Bad pcx file (%i x %i) (%i x %i)\n", xmax + 1, ymax + 1, pcx->xmax, pcx->ymax);
		return;
	}

	out = (byte*)malloc((ymax + 1) * (xmax + 1));

	*pic = out;

	pix = out;

	if (palette)
	{
		*palette = (byte*)malloc(768);
		memcpy(*palette, (byte*)pcx + rawlen - 768, 768);
	}

	width = xmax + 1;
	height = ymax + 1;
	// FIXME: use bytes_per_line here?

	for (y = 0; y <= ymax; y++, pix += xmax + 1)
	{
		for (x = 0; x <= xmax; )
		{
			dataByte = *raw++;

			if ((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while (runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if (raw - (byte*)pcx > rawlen)
	{
		ri.Con_Print(PRINT_DEVELOPER, "PCX file was malformed");
		free(*pic);
		*pic = NULL;
	}
}

/*
==============
LoadPCX32

Loads an 8-bit PCX and converts it to 32-bit
==============
*/
static void R_LoadPCX32(byte* raw, int rawlen, byte** pic, int& width, int& height)
{
	byte*	palette;
	byte*	pic8;
	byte*	pic32;

	R_LoadPCX(raw, rawlen, &pic8, &palette, width, height);
	if (!pic8) {
		*pic = NULL;
		return;
	}

	int c = width * height;
	pic32 = *pic = (byte*)malloc(4 * c);
	for (int i = 0; i < c; i++)
	{
		int p = pic8[i];
		pic32[0] = palette[p * 3];
		pic32[1] = palette[p * 3 + 1];
		pic32[2] = palette[p * 3 + 2];
		if (p == 255) {
			pic32[3] = 0;
		}
		else {
			pic32[3] = 255;
		}

		pic32 += 4;
	}

	free(pic8);
	free(palette);
}

/*
================
R_LoadWAL

Loads a WAL, passes it through the global palette and makes it 32-bit
================
*/
static void R_LoadWAL(byte* raw, byte** pic, int& width, int& height)
{
	miptex_t* mt = (miptex_t*)raw;

	// Send out the width and height
	width = mt->width;
	height = mt->height;

	// Number of pixels, each pixel is the index of an entry in the palette
	int c = width * height;

	// Pointer to the 8-bit data
	byte* pic8 = (byte*)mt + mt->offsets[0];
	uint* pic32 = (uint*)malloc(c * 4);

	// uint	d_8to24table[256];

	// For each pixel
	for (int i = 0; i < c; ++i)
	{
		pic32[i] = d_8to24table[pic8[i]];
	}

	// Don't lose it!
	*pic = (byte*)pic32;
}

/*
================
R_LightScaleTexture

Scales up the pixel values in a texture to increase the
lighting range
================
*/
#if 0
static void R_LightScaleTexture(byte* in, int width, int height)
{
	int c = width * height;

	for (int i = 0; i < c; i++, in += 4)
	{
		in[0] = s_intensitytable[in[0]];
		in[1] = s_intensitytable[in[1]];
		in[2] = s_intensitytable[in[2]];
	}
}
#endif

#if QMIP
/*
================
R_MipMap

Operates in place, quartering the size of the texture
Proper linear filter
================
*/
static void R_MipMap(uint* in, int inWidth, int inHeight)
{
	int			i, j, k;
	byte*		outpix;
	int			inWidthMask, inHeightMask;
	int			total;
	int			outWidth, outHeight;
	uint*		temp;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	// Slart: Badmalloc
	temp = (uint*)malloc(outWidth * outHeight * 4);
	assert(temp);

	inWidthMask = inWidth - 1;
	inHeightMask = inHeight - 1;

	for (i = 0; i < outHeight; i++) {
		for (j = 0; j < outWidth; j++) {
			outpix = (byte*)(temp + i * outWidth + j);
			for (k = 0; k < 4; k++) {
				total =
					1 * ((byte*)&in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
					2 * ((byte*)&in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
					2 * ((byte*)&in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
					1 * ((byte*)&in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +

					2 * ((byte*)&in[((i * 2) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
					4 * ((byte*)&in[((i * 2) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
					4 * ((byte*)&in[((i * 2) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
					2 * ((byte*)&in[((i * 2) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +

					2 * ((byte*)&in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
					4 * ((byte*)&in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
					4 * ((byte*)&in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
					2 * ((byte*)&in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +

					1 * ((byte*)&in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
					2 * ((byte*)&in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
					2 * ((byte*)&in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
					1 * ((byte*)&in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k];
				outpix[k] = total / 36;
			}
		}
	}

	memcpy(in, temp, outWidth * outHeight * 4);
	free(temp);
}
#endif

/*
===============
R_SwapImage

Swap an image's bytes from RGBA to BGRA or vice versa
===============
*/
void R_SwapImage(int width, int height, byte* data)
{
#if 1
	// width * height * depth (depth is always 4, 32-bit)
	const int numpixels = width * height * 4;

	byte old;

	for (int i = 0; i < numpixels; i += 4)
	{
		// Copy red to old
		old = data[i];
		// Assign blue to red
		data[i] = data[i + 2];
		// Assign red to blue
		data[i + 2] = old;
	}
#endif
}

/*
===============
R_Upload32

This must NEVER fail
===============
*/
static GLuint R_Upload32(uint* data, int width, int height, imagetype_t type)
{
	// Only mipmap select types
	// We want to mipmap walls, skins and sprites, although never
	// 2D pics or skies

	constexpr GLenum format =
#ifdef _WIN32
	// Store as GL_BGRA on Windows
	// We have to manually swap bytes
	GL_BGRA;
	R_SwapImage(width, height, (byte*)data);
#else
	// GL_RGBA everywhere else
	GL_RGBA;
#endif

	// Lightscale everything!
//	R_LightScaleTexture((byte*)data, width, height);

	GLuint id;

	// Generate and bind the texture
	glGenTextures(1, &id);
	GL_Bind(id);

	// Upload it
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	// Pics and skies should never be mipmapped
	if (type != it_pic && type != it_sky)
	{
		// Auto-generate mipmaps!
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		// Only mipmappable textures should be anisotropic
		// It's safe to do this because we clamp it in GL_TextureAnisoMode in GL_SetDefaultState
		// Sprites should never be anisotropically filtered because they always face the player
		if (type != it_sprite) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, gl_textureanisomode->value);
		}
	}
	else if (type == it_pic)
	{
		// NEVER filter pics, EVER, it looks like ass
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}

	// Clamp everything that isn't lit
	if (type != it_wall && type != it_skin)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	return id;
}

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
void R_LoadImage(const char* name, byte** pic, int& width, int& height)
{
	byte*	buf;
	int		buflen;

	if (gl_allowreplace->value)
	{
		size_t namelen = strlen(name);

		char newname[MAX_QPATH];
		COM_StripExtension(name, newname);

		constexpr int numExts = 5;
		constexpr const char* ext[numExts]{ "png", "tga", "jpg", "wal", "pcx" };
		newname[namelen - 4] = '.';
		newname[namelen] = '\0';

		// Test all FIVE supported extensions
		for (int i = 0; i < numExts; ++i)
		{
			newname[namelen - 3] = ext[i][0];
			newname[namelen - 2] = ext[i][1];
			newname[namelen - 1] = ext[i][2];

			buflen = ri.FS_LoadFile(newname, (void**)&buf);
			if (buf)
			{
				goto good;
			}
		}
		// Nothing? Crap
		*pic = NULL;
		return;
	}
	else
	{
		// Standard procedure
		buflen = ri.FS_LoadFile(name, (void**)&buf);
		if (!buf)
		{
			*pic = NULL;
			return;
		}
	}
good:

	// We load using STBI first, if it fails, fall back to the WAL and PCX loaders

	// We always want 32-bit, but comp can tell us whether that 32-bit alpha is used...
	*pic = stbi_load_from_memory(buf, buflen, &width, &height, nullptr, 4);

	if (!*pic)
	{
		// STBI failed, legacy Q2 content

		const char* ext = strrchr(name, '.') + 1;

		if (strcmp(ext, "wal") == 0)
		{
			R_LoadWAL(buf, pic, width, height);
		}
		else if (strcmp(ext, "pcx") == 0)
		{
			R_LoadPCX32(buf, buflen, pic, width, height);
		}

		// Pic is NULL by now
	}

	ri.FS_FreeFile(buf);
}

/*
================
R_CreateImage

This is the only way any image_t are created
================
*/
image_t* R_CreateImage(const char* name, const byte* pic, int width, int height, imagetype_t type)
{
	int			i;
	image_t*	image;

	// find a free image_t
	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (!image->texnum)
			break;
	}
	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			ri.Sys_Error(ERR_DROP, "MAX_GLTEXTURES");
		numgltextures++;
	}
	image = &gltextures[i];

	strcpy(image->name, name);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	// Upload it!
	// Pics and skies are never mipmapped (they don't need em)
	image->texnum = R_Upload32((uint*)pic, width, height, type);

	image->sl = 0;
	image->sh = 1;
	image->tl = 0;
	image->th = 1;

	return image;
}

/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t* GL_FindImage(const char* name, imagetype_t type)
{
	assert(name);

	// look for it
	int i;
	image_t* image;
	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (strcmp(name, image->name) == 0)
		{
			image->registration_sequence = registration_sequence;
			return image;
		}
	}

	//
	// load the pic from disk
	//
	byte* pic;
	int width, height;

	R_LoadImage(name, &pic, width, height);
	if (!pic) {
		return r_notexture;
	}

	image = R_CreateImage(name, pic, width, height, type);

	if (pic)
		free(pic);

	return image;
}

/*
===============
R_RegisterSkin
===============
*/
image_t* R_RegisterSkin(const char* name)
{
	return GL_FindImage(name, it_skin);
}

/*
================
GL_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void GL_FreeUnusedImages(void)
{
	int i;
	image_t* image;

	// never free r_notexture or particle texture
	r_notexture->registration_sequence = registration_sequence;
	r_particletexture->registration_sequence = registration_sequence;

	for (i = 0, image = gltextures; i < numgltextures; ++i, ++image)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type == it_pic)
			continue;		// don't free pics

		// free it
		glDeleteTextures(1, &image->texnum);
		memset(image, 0, sizeof(*image));
	}
}

/*
===============
R_GetPalette
===============
*/
int R_GetPalette(void)
{
	byte* buf;

	// Load the palette PCX

	int buflen = ri.FS_LoadFile("pics/colormap.pcx", (void**)&buf);
	if (!buf) {
		ri.Sys_Error(ERR_FATAL, "Couldn't load pics/colormap.pcx");
	}

	byte* pic;
	byte* pal;
	int width, height;

	R_LoadPCX(buf, buflen, &pic, &pal, width, height);
	if (!pal) {
		ri.Sys_Error(ERR_FATAL, "pics/colormap.pcx is not a valid PCX!");
	}

	free(pic);

	ri.FS_FreeFile(buf);

	int		r, g, b;
	uint	v;

	for (int i = 0; i < 256; ++i)
	{
		r = pal[i * 3 + 0];
		g = pal[i * 3 + 1];
		b = pal[i * 3 + 2];

		v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
		d_8to24table[i] = LittleLong(v);
	}

	free(pal);

	d_8to24table[255] &= LittleLong(0xffffff);	// 255 is transparent

	return 0;
}

/*
===============
R_SetColorMappings
===============
*/
static void R_SetColorMappings(void)
{
	r_overbrightbits = ri.Cvar_Get("r_overbrightbits", "2", 0);
	r_intensity = ri.Cvar_Get("r_intensity", "1", 0);

	// Setup the overbright lighting
	glConfig.overbrightBits = (int)r_overbrightbits->value;

	// Cap overbright bits
	if (glConfig.overbrightBits > 2) {
		glConfig.overbrightBits = 2;
	}
	if (glConfig.overbrightBits < 0) {
		glConfig.overbrightBits = 0;
	}

	if (r_intensity->value < 1.0f) {
		ri.Cvar_Set("r_intensity", "1");
	}

	// Set up gamma table
	for (int i = 0; i < 256; i++)
	{
		int inf;

		if (vid_gamma->value == 1.0f) {
			inf = i;
		}
		else {
			inf = (int)(255.0f * powf(i / 255.0f, 1.0f / vid_gamma->value) + 0.5f);
		}
		inf <<= glConfig.overbrightBits;
		if (inf < 0) {
			inf = 0;
		}
		if (inf > 255) {
			inf = 255;
		}
		s_gammatable[i] = inf;
	}

	// Set up intensity table
	for (int i = 0; i < 256; i++)
	{
		int j = (int)(i * r_intensity->value);
		if (j > 255) {
			j = 255;
		}
		s_intensitytable[i] = (byte)j;
	}

//	GLimp_SetGamma(s_gammatable, s_gammatable, s_gammatable);
}

/*
===============
GL_InitImages
===============
*/
void GL_InitImages(void)
{
	registration_sequence = 1;

	R_SetColorMappings();
}

/*
===============
GL_ShutdownImages
===============
*/
void GL_ShutdownImages(void)
{
	int i;
	image_t* image;

	for (i = 0, image = gltextures; i < numgltextures; ++i, ++image)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot

		// free it
		glDeleteTextures(1, &image->texnum);
		memset(image, 0, sizeof(*image));
	}

	// Restore gamma set in R_SetColorMappings
//	GLimp_RestoreGamma();
}


/*
=============================================================

Helper functions for other code

=============================================================
*/


void GL_EnableMultitexture(qboolean enable)
{
	if (!gl_ext_multitexture->value)
		return;

	if (enable)
	{
		GL_SelectTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		GL_TexEnv(GL_REPLACE);
	}
	else
	{
		GL_SelectTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		GL_TexEnv(GL_REPLACE);
	}
	GL_SelectTexture(GL_TEXTURE0);
	GL_TexEnv(GL_REPLACE);
}

void GL_SelectTexture(GLenum texture)
{
	if (!gl_ext_multitexture->value)
		return;

	GLuint tmu;

	if (texture == GL_TEXTURE0)
	{
		tmu = 0;
	}
	else
	{
		tmu = 1;
	}

	if (tmu == glState.currenttmu)
	{
		return;
	}

	glState.currenttmu = tmu;

	glActiveTexture(texture);
	glClientActiveTexture(texture);
}

void GL_TexEnv(GLint mode)
{
	static int lastmodes[2]{ -1, -1 };

	if (mode != lastmodes[glState.currenttmu])
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		lastmodes[glState.currenttmu] = mode;
	}
}

void GL_Bind(GLuint texnum)
{
	if (glState.currenttextures[glState.currenttmu] == texnum)
		return;
	glState.currenttextures[glState.currenttmu] = texnum;
	glBindTexture(GL_TEXTURE_2D, texnum);
}

void GL_MBind(GLenum target, GLuint texnum)
{
	GL_SelectTexture(target);
	if (target == GL_TEXTURE0)
	{
		if (glState.currenttextures[0] == texnum)
			return;
	}
	else
	{
		if (glState.currenttextures[1] == texnum)
			return;
	}
	GL_Bind(texnum);
}

struct glmode_t
{
	const char* name;
	int	minimize, maximize;
};

static constexpr glmode_t modes[]{
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

/*
===============
GL_TextureMode

SlartTodo: This sets the gl_filter settings based on the convar, THE FILTERMODE WILL ALWAYS BE SET TO LINEAR ON STARTUP!!!!
===============
*/
void GL_TextureMode(const char* string)
{
	int		i;
	image_t* glt;

	for (i = 0; i < NUM_GL_MODES; i++)
	{
		if (!Q_stricmp(modes[i].name, string))
			break;
	}

	if (i == NUM_GL_MODES)
	{
		ri.Con_Print(PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i = 0, glt = gltextures; i < numgltextures; i++, glt++)
	{
		if (glt->type != it_pic && glt->type != it_sky)
		{
			GL_Bind(glt->texnum);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
GL_TextureAnisoMode
===============
*/
void GL_TextureAnisoMode(float mode)
{
	if (mode < 1.0f || mode > 16.0f)
	{
		ri.Con_Print(PRINT_ALL, "invalid texture ansiotropy mode\n");
		ri.Cvar_Set("gl_textureanisomode", "1");
		gl_textureanisomode->modified = false;
		return;
	}

	// change all the existing mipmap texture objects
	int i;
	image_t* glt;
	for (i = 0, glt = gltextures; i < numgltextures; i++, glt++)
	{
		if (glt->type != it_pic && glt->type != it_sky)
		{
			GL_Bind(glt->texnum);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, mode);
		}
	}
}

/*
===============
GL_ImageList_f
===============
*/
void GL_ImageList_f(void)
{
	int		i;
	image_t* image;
	int		texels;

	ri.Con_Print(PRINT_ALL, "------------------\n");
	texels = 0;

	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (image->texnum <= 0)
			continue;
		texels += image->width * image->height;
		switch (image->type)
		{
		case it_skin:
			ri.Con_Print(PRINT_ALL, "M");
			break;
		case it_sprite:
			ri.Con_Print(PRINT_ALL, "S");
			break;
		case it_wall:
			ri.Con_Print(PRINT_ALL, "W");
			break;
		case it_pic:
			ri.Con_Print(PRINT_ALL, "P");
			break;
		default:
			ri.Con_Print(PRINT_ALL, " ");
			break;
		}

		ri.Con_Printf(PRINT_ALL, " %3i %3i: %s\n",
			image->width, image->height, image->name);
	}
	ri.Con_Printf(PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels);
}
