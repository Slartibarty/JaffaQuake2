/*
=============================================================

Windows specific OpenGL stuff

=============================================================
*/

#include "gl_local.hpp"

#include "../common/buildinfo.h"

#include <Windows.h>
#include "../engine/winres/resource.h"

#include "GL/wglew.h"

// Global
static struct GLWState
{
	HINSTANCE	hInstance;

	// Window vars
	HWND		hWnd;
	HDC			hDC;
	HGLRC		hGLRC;
	WNDPROC		wndproc;

	qboolean	fullscreen = true;

} s_glwstate;

/*
=============================================================================

DUMMY WINDOW

=============================================================================
*/

struct DummyVars
{
	HWND	hWnd;
	HGLRC	hGLRC;
};

#define DUMMY_CLASSNAME L"GRUG"

// Create a dummy window
//
static void GLimp_CreateDummyWindow (DummyVars &dvars)
{
	BOOL result;

	// Create our dummy window class
	const WNDCLASSEXW wcex =
	{
		sizeof (wcex),				// Size
		CS_OWNDC,					// Style
		DefWindowProcW,				// Wndproc
		0,							// ClsExtra
		0,							// WndExtra
		s_glwstate.hInstance,		// hInstance
		NULL,						// Icon
		NULL,						// Cursor
		(HBRUSH)(COLOR_WINDOW + 1),	// Background colour
		NULL,						// Menu
		DUMMY_CLASSNAME,			// Class name
		NULL						// Small icon
	};
	RegisterClassExW (&wcex);

	dvars.hWnd = CreateWindowExW (
		0,
		DUMMY_CLASSNAME,
		L"",
		0,
		0, 0, 800, 600,
		NULL,
		NULL,
		s_glwstate.hInstance,
		NULL
	);
	assert (dvars.hWnd);

	// Create our dummy PFD
	const PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof (pfd),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
		PFD_TYPE_RGBA,		// The kind of framebuffer. RGBA or palette
		32,					// Colordepth of the framebuffer
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,					// Number of bits for the depthbuffer
		8,					// Number of bits for the stencilbuffer
		0,					// Number of Aux buffers in the framebuffer
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	// Get this window's DC, we don't need to release it afterwards due to
	// CS_OWNDC
	HDC hDC = GetDC (dvars.hWnd);
	assert (hDC);

	int pixelformat = ChoosePixelFormat (hDC, &pfd);
	assert (pixelformat != 0);

	result = SetPixelFormat (hDC, pixelformat, &pfd);
	assert (result);

	dvars.hGLRC = wglCreateContext (hDC);
	assert (dvars.hGLRC);

	result = wglMakeCurrent (hDC, dvars.hGLRC);
	assert (result);
}

// Destroy a dummy window
//
static void GLimp_DestroyDummyWindow (DummyVars &dvars)
{
	BOOL result;

	result = wglDeleteContext (dvars.hGLRC);
	assert (result);
	result = DestroyWindow (dvars.hWnd);
	assert (result);
	result = UnregisterClassW (DUMMY_CLASSNAME, s_glwstate.hInstance);
	assert (result);
}

/*
=============================================================================

MAIN WINDOW

=============================================================================
*/

#define WINDOW_TITLE		(L"JaffaQuake 2 - v" VERSIONSTRING " - OpenGL - " BUILDSTRING)
#define	WINDOW_CLASS_NAME	L"JQ2"
#define	WINDOW_STYLE		(WS_OVERLAPPEDWINDOW)

static void GLimp_CreateWindow (qboolean fullscreen)
{
	BOOL result;

	const WNDCLASSEXW wcex =
	{
		sizeof (wcex),			// Structure size
		CS_OWNDC,				// Class style
		s_glwstate.wndproc,		// WndProc
		0,						// Extra storage
		0,						// Extra storage
		s_glwstate.hInstance,	// Module
		(HICON)LoadImageW (s_glwstate.hInstance, MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 0, 0, LR_SHARED),		// Icon
		(HCURSOR)LoadImageW (NULL, MAKEINTRESOURCEW (32512), IMAGE_CURSOR, 0, 0, LR_SHARED),	// Cursor to use
		(HBRUSH)(COLOR_WINDOW + 1),		// Background colour
		NULL,					// Menu
		WINDOW_CLASS_NAME,		// Classname
		NULL					// Small icon
	};
	RegisterClassExW (&wcex);

	RECT r{ 0,0,1024,768 };
	AdjustWindowRectEx (&r, WINDOW_STYLE, false, 0);

	// Create dummy window
	s_glwstate.hWnd = CreateWindowExW (
		0,						// Ex-Style
		WINDOW_CLASS_NAME,		// Window class
		WINDOW_TITLE,			// Window title
		WINDOW_STYLE,			// Window style
		CW_USEDEFAULT,			// X pos
		0,						// Y pos (Calls ShowWindow if WS_VISIBLE is set)
		r.right - r.left,		// Width
		r.bottom - r.top,		// Height
		NULL,					// Parent window
		NULL,					// Menu to use
		s_glwstate.hInstance,	// Module instance
		NULL					// Additional params
	);
	assert (s_glwstate.hWnd);

	// GL stuff

	int multiSamples = (int)gl_multisamples->value;

	if (multiSamples < 1) {
		multiSamples = 1;
		ri.Cvar_Set("gl_multisamples", "1");
		gl_multisamples->modified = false;
	}
	if (multiSamples > 16) {
		multiSamples = 16;
		ri.Cvar_Set("gl_multisamples", "16");
		gl_multisamples->modified = false;
	}

	const int attriblist[]{
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
	//	WGL_SAMPLES_ARB, multiSamples,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		0, // End
	};

	PIXELFORMATDESCRIPTOR pfd;
	int		pixelformat;
	UINT	numformats;

	s_glwstate.hDC = GetDC (s_glwstate.hWnd);
	assert (s_glwstate.hDC);

	result = wglChoosePixelFormatARB (s_glwstate.hDC, attriblist, NULL, 1, &pixelformat, &numformats);
	assert (result);

	result = DescribePixelFormat (s_glwstate.hDC, pixelformat, sizeof (pfd), &pfd);
	assert (result != 0);

	result = SetPixelFormat (s_glwstate.hDC, pixelformat, &pfd);
	assert (result);

#if 1
	const int contextAttribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		0, // End
	};

	s_glwstate.hGLRC = wglCreateContextAttribsARB (s_glwstate.hDC, NULL, contextAttribs);
	assert (s_glwstate.hGLRC);
#else
	s_glwstate.hGLRC = wglCreateContext (s_glwstate.hDC);
	assert (s_glwstate.hGLRC);
#endif

	result = wglMakeCurrent (s_glwstate.hDC, s_glwstate.hGLRC);
	assert (result);

	wglSwapIntervalEXT (gl_swapinterval->value);

	// Always show the window, we have no way of getting nCmdShow down here anyway
	ShowWindow (s_glwstate.hWnd, SW_SHOW);
}

// Toggle fullscreen mode for this window
//
static void GLimp_ToggleFullscreen (bool fullscreen)
{
	if (fullscreen == true)
	{
		// fullscreen is just a borderless window
		SetWindowLongPtrW (s_glwstate.hWnd, GWL_STYLE, WS_OVERLAPPED);

		SetWindowPos (s_glwstate.hWnd, HWND_TOP, 0, 0, 1920, 1080, SWP_FRAMECHANGED);
	}
	else
	{
		SetWindowLongPtrW (s_glwstate.hWnd, GWL_STYLE, WINDOW_STYLE);

		SetWindowPos (s_glwstate.hWnd, NULL, 0, 0, 800, 600, SWP_NOZORDER | SWP_FRAMECHANGED);
	}
}

// Destroy THE window
//
static void GLimp_DestroyWindow (void)
{
	if (s_glwstate.hGLRC)
	{
		wglMakeCurrent (NULL, NULL);
		wglDeleteContext (s_glwstate.hGLRC);
		s_glwstate.hGLRC = NULL;
	}
	if (s_glwstate.hWnd)
	{
		DestroyWindow (s_glwstate.hWnd);
		UnregisterClassW (WINDOW_CLASS_NAME, s_glwstate.hInstance);
		s_glwstate.hWnd = NULL;
	}
}

/*
=============================================================================

MISC

=============================================================================
*/

static unsigned short s_oldHardwareGamma[3][256];

// GLimp_SetGamma
//
void GLimp_SetGamma(byte* red, byte* green, byte* blue)
{
	if (!s_glwstate.hDC || !s_glwstate.fullscreen) {
		return;
	}

	unsigned short table[3][256];

	for (int i = 0; i < 256; i++)
	{
		table[0][i] = (((unsigned short)red[i]) << 8) | red[i];
		table[1][i] = (((unsigned short)green[i]) << 8) | green[i];
		table[2][i] = (((unsigned short)blue[i]) << 8) | blue[i];
	}

	// enforce constantly increasing
	for (int j = 0; j < 3; j++)
	{
		for (int i = 1; i < 256; i++)
		{
			if (table[j][i] < table[j][i - 1])
			{
				table[j][i] = table[j][i - 1];
			}
		}
	}

	HDC hDC = GetDC(NULL);

	GetDeviceGammaRamp(hDC, s_oldHardwareGamma);

	ReleaseDC(NULL, hDC);

	SetDeviceGammaRamp(s_glwstate.hDC, table);
}

// GLimp_RestoreGamma
//
void GLimp_RestoreGamma(void)
{
	if (!s_glwstate.fullscreen) {
		return;
	}

	HDC hDC = GetDC(NULL);

	SetDeviceGammaRamp(hDC, s_oldHardwareGamma);

	ReleaseDC(NULL, hDC);
}

// Init this subsystem
//
qboolean GLimp_Init (void *hinstance, void *wndproc)
{
	s_glwstate.hInstance = (HINSTANCE)hinstance;
	s_glwstate.wndproc = (WNDPROC)wndproc;

	DummyVars dvars;
	GLimp_CreateDummyWindow (dvars);

	// initialize our OpenGL dynamic bindings
	glewExperimental = GL_TRUE;
	if (glewInit () != GLEW_OK && wglewInit () != GLEW_OK)
	{
		ri.Con_Print (PRINT_ALL, "ref_gl::R_Init() - could not load OpenGL bindings\n");
		return false;
	}

	GLimp_CreateWindow (vid_fullscreen->value);

	GLimp_DestroyDummyWindow (dvars);

	return true;
}

// Shutdown this subsystem
//
void GLimp_Shutdown (void)
{
	GLimp_DestroyWindow ();
}

// Responsible for doing stuff at the start of a frame
//
void GLimp_BeginFrame (void)
{
//	glDrawBuffer (GL_BACK);

	// If fullscreen has been modified, do a toggle!
	if (vid_fullscreen->modified)
	{
		vid_fullscreen->modified = false;

		// SlartTodo: Add fullscreen support back
	//	GLimp_ToggleFullscreen (vid_fullscreen->value);
	}

	// If our swap interval has changed, then change it!
	if (gl_swapinterval->modified)
	{
		gl_swapinterval->modified = false;

		if (WGLEW_EXT_swap_control)
		{
			wglSwapIntervalEXT (gl_swapinterval->value);
		}
	}
}

// Responsible for doing a swapbuffers and that's about it
//
void GLimp_EndFrame (void)
{
	SwapBuffers (s_glwstate.hDC);
}

// Actions to take when the game becomes active or inactive
//
void GLimp_AppActivate (qboolean active)
{
	if (active)
	{
		// Restore. SlartTodo: Should we check to see if the window is minimised before doing this?
		// Because SW_RESTORE will restore maximised windows to normal
	//	SetForegroundWindow (glw_state.hWnd);
	//	ShowWindow (s_glwstate.hWnd, SW_RESTORE);
	}
	else
	{
		// If we're fullscreen, minimise, otherwise, do nothing
	//	if (vid_fullscreen->value) {
	//		ShowWindow (s_glwstate.hWnd, SW_MINIMIZE);
	//	}
	}
}
