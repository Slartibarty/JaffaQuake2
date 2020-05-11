//
// OpenGL shader management
//

#include "gl_local.h"

#include "smdloader.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define NUM_PROGRAMS 1

// All textures
//shaderprogram_t<2>	g_shaders[NUM_PROGRAMS];
//int					g_numshaders;

GLuint					g_mainProgram;

constexpr auto s_vertShader
{
R"(#version 330 compatibility

layout (location = 0) in vec3 inPos;

// Uniforms
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main ()
{
	gl_Position = uProjection * uView * uModel * vec4(inPos, 1.0);
}
)"
};

constexpr auto s_fragShader
{
R"(#version 330 compatibility

out vec4 FragColour;

void main ()
{
	FragColour = vec4 (0.25, 0.25, 0.25, 1.0);
}
)"
};

/*
===============
GL_InitShaders
===============
*/
bool GL_InitShaders ()
{
	GLuint shaderProgram = glCreateProgram ();

	GLuint vertShader = glCreateShader (GL_VERTEX_SHADER);
	GLuint fragShader = glCreateShader (GL_FRAGMENT_SHADER);

//	char *vertSource;
//	char *fragSource;

//	ri.FS_LoadFile ("shaders/basic.vert", (void**)&vertSource);
//	ri.FS_LoadFile ("shaders/basic.frag", (void**)&fragSource);

	glShaderSource (vertShader, 1, &s_vertShader, nullptr);
	glShaderSource (fragShader, 1, &s_fragShader, nullptr);

//	ri.FS_FreeFile (vertSource);
//	ri.FS_FreeFile (fragSource);

	GLint status;
	char cmpBuf[64];

	glCompileShader (vertShader);
	glGetShaderiv (vertShader, GL_COMPILE_STATUS, &status);
	assert (status == GL_TRUE);

	glGetShaderInfoLog (vertShader, sizeof(cmpBuf), nullptr, cmpBuf);

	glCompileShader (fragShader);
	glGetShaderiv (fragShader, GL_COMPILE_STATUS, &status);
	assert (status == GL_TRUE);

	glGetShaderInfoLog (fragShader, sizeof (cmpBuf), nullptr, cmpBuf);

	glAttachShader (shaderProgram, vertShader);
	glAttachShader (shaderProgram, fragShader);

	glLinkProgram (shaderProgram);

	auto temp = glGetError ();
	assert (temp == GL_NO_ERROR);

	g_mainProgram = shaderProgram;

	return true;
}

// temp code

void GLTEMP_DrawTestModel ()
{
	static bool doOnce = false;
	static int numTris;

	if (doOnce == false)
	{
		rendersystem::SMDLoader smdLoader (L"C:\\Users\\Josh\\Projects\\JaffaQuake2\\game\\baseq2\\tempmodels\\gman.smd");

		float8 *tris = smdLoader.GetTris (numTris);

		// Create a vertex array object
		// Create a vertex buffer handle
		GLuint vao, vbo;
		glGenVertexArrays (1, &vao);
		glGenBuffers (1, &vbo);

		glBindVertexArray (vao);

		glBindBuffer (GL_ARRAY_BUFFER, vbo);
		// Slap our vertices into the vertex buffer
		glBufferData (GL_ARRAY_BUFFER, numTris * sizeof (float8), tris, GL_STATIC_DRAW);

		// Set up our in and out variables in the shaders
		glVertexAttribPointer (
			0,						// 0 is the location we set in the .glsl file of our "aPos" variable
			3,						// Size of each vertex in our hardcoded array, which is 3 floats for each vertex (we pass no extra data)
			GL_FLOAT,				// Type used for our vertex points, which is a float
			GL_FALSE,				// Normalise data (not relevant to me right now)
			3 * sizeof (GLfloat),	// The next vertex in the array is 3 times the size of a float away from the start of the last variable
			0						// Where the start of the vertex data is
		);
		glEnableVertexAttribArray (0); // Location of "aPos" again

		// note that this is allowed, the call to glVertexAttribPointer registered VBO as
		// the vertex attribute's bound vertex buffer object, so afterwards we can safely unbind
		glBindBuffer (GL_ARRAY_BUFFER, 0);

		// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
		// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	//	glBindVertexArray (0);

		doOnce = true;
	}

//	glUseProgram (g_mainProgram);

	// Get the locations of various uniforms assigned later
	auto modloc = glGetUniformLocation (g_mainProgram, "uModel");
	auto viewloc = glGetUniformLocation (g_mainProgram, "uView");
	auto projloc = glGetUniformLocation (g_mainProgram, "uProjection");

	// "Camera" matrix
	glm::mat4 view (1.0f);
	glm::vec3 tempview { r_newrefdef.vieworg[0], r_newrefdef.vieworg[1], r_newrefdef.vieworg[2] };
	view = glm::translate (view, tempview);

	glm::mat4 model (1.0f);
//	model = glm::rotate(model, r_newrefdef.time, { 0.0f, 0.0f, 1.0f });

	// Makes everything ortho or projected
	glm::mat4 projection;
	projection = glm::perspective (glm::radians (45.0f), 800.0f / 600.0f, 0.1f, 4096.0f);

	glUniformMatrix4fv (viewloc, 1, GL_FALSE, glm::value_ptr (view));
	glUniformMatrix4fv (modloc, 1, GL_FALSE, glm::value_ptr (model));
	glUniformMatrix4fv (projloc, 1, GL_FALSE, glm::value_ptr (projection));

	glDrawArrays (GL_TRIANGLES, 0, numTris);

//	glUseProgram (0);
}
