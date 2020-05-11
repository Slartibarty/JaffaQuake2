/*
=============================================================

Surface-related refresh code

=============================================================
*/

#pragma once

// Globals

extern int c_visible_lightmaps;
extern int c_visible_textures;

/*
=============================================================

	BRUSH MODELS!

=============================================================
*/

void R_RenderBrushPoly(msurface_t* fa);

void R_DrawAlphaSurfaces(void);

void R_DrawBrushModel(entity_t* e);

/*
=============================================================

	WORLD MODEL!

=============================================================
*/

void R_DrawWorld(void);

void R_MarkLeaves(void);

/*
=============================================================

	LIGHTMAP ALLOCATION!

=============================================================
*/

// These functions are only used by gl_model.cpp

void GL_BuildPolygonFromSurface(msurface_t* fa);
void GL_CreateSurfaceLightmap(msurface_t* surf);
void GL_BeginBuildingLightmaps(model_t* m);
void GL_EndBuildingLightmaps(void);
