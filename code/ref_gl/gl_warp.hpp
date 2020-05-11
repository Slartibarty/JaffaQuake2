/*
=============================================================

Code for drawing sky and water polygons

=============================================================
*/

#pragma once

// Breaks a polygon up along axial 64 unit
// boundaries so that turbulent and sky warps
// can be done reasonably.
void GL_SubdivideSurface(msurface_t* fa);

// Does a water warp on the pre-fragmented glpoly_t chain
void EmitWaterPolys(msurface_t* fa);

void R_AddSkySurface(msurface_t* fa);

void R_ClearSkyBox(void);

void R_DrawSkyBox(void);

// Set the current sky image
void R_SetSky(const char* name, float rotate, vec3_t axis);


