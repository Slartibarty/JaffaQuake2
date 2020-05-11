//
// OpenGL shader management
//

#pragma once

enum shadertype_t
{
	st_vertex,
	st_fragment
};

// A shader
//
struct shader_t
{
	char				name[MAX_QPATH];	// Game path, including extension
	shadertype_t		type;
};

// A shader program
//
template<int numShaders>
struct shaderprogram_t
{
	char				name[MAX_QPATH];

	shader_t			shaders[numShaders];
};

#define MAX_GLSHADERS	64
#define NUM_GLPROGRAMS	1

extern GLuint g_mainProgram;

bool GL_InitShaders ();

void GLTEMP_DrawTestModel ();
