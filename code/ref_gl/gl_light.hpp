/*
=============================================================

Lights?!

=============================================================
*/

#pragma once

// Variables

// Used by gl_mesh
extern vec3_t lightspot;

// Functions

/*
=============
DYNAMIC LIGHTS BLEND RENDERING
=============
*/

void R_RenderDlights(void);

/*
=============
DYNAMIC LIGHTS
=============
*/

void R_MarkLights(dlight_t* light, int bit, mnode_t* node);

void R_PushDlights(void);

/*
=============
LIGHT SAMPLING
=============
*/

void R_LightPoint(vec3_t p, vec3_t color);

// Used by gl_surf
void R_SetCacheState(msurface_t* surf);
// This too
void R_BuildLightMap(msurface_t* surf, byte* dest, int stride);


