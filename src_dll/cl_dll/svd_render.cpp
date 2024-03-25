//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "windows.h"
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

#include "pmtrace.h"
#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"
#include "pm_defs.h"
#include "elightlist.h"
#include "svdformat.h"
#include "svd_render.h"
#include "fog.h"

// Quake definitions
#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWSPRITE		8
#define SURF_DRAWTURB		0x10
#define SURF_DRAWTILED		0x20
#define SURF_DRAWBACKGROUND	0x40
#define SURF_UNDERWATER		0x80
#define SURF_DONTWARP		0x100
#define BACKFACE_EPSILON	0.01

// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// Globals used by shadow rendering
model_t*	g_pWorld;
int			g_visFrame = 0;
int			g_frameCount = 0;
vec3_t		g_viewOrigin;
bool		g_bFBOSupported = false;
bool		g_bAlertAboutMSAA = false;

// Latest free texture
GLuint		g_freeTextureIndex = 131072;

// renderbuffer objects for fbo
GLuint		g_colorRBO = 0;
GLuint		g_depthStencilRBO = 0;

// framebuffer object
GLuint		g_stencilFBO = 0;

// Steam HL's FBO
GLint		g_steamFBO = 0;

// The renderer object, created on the stack.
extern CGameStudioModelRenderer g_StudioRenderer;

// OpenGL functions needed
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = NULL;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = NULL;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = NULL;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = NULL;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = NULL;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = NULL;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv = NULL;
PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer = NULL;

/*
====================
Mod_PointInLeaf

====================
*/
mleaf_t *Mod_PointInLeaf (vec3_t p, model_t *model) // quake's func
{
	mnode_t *node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t *)node;
		mplane_t *plane = node->plane;
		float d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never reached
}

/*
==================
R_IsExtensionSupported

==================
*/
bool R_IsExtensionSupported( const char *ext )
{
	const char * extensions = (const char *)glGetString ( GL_EXTENSIONS );
	const char * start = extensions;
	const char * ptr;

	while ( ( ptr = strstr ( start, ext ) ) != NULL )
	{
		// we've found, ensure name is exactly ext
		const char * end = ptr + strlen ( ext );
		if ( isspace ( *end ) || *end == '\0' )
			return true;

		start = end;
	}
	return false;
}

//====================================
//
//====================================
bool R_CheckMSAA( void )
{
	GLuint iTestGen;
	glGenFramebuffers(1, &iTestGen);

	bool result;
	if(iTestGen != 1)
		result = false;
	else
		result = true;

	glDeleteFramebuffers(1, &iTestGen);
	return result;
}


/*
====================
SVD_CreateStencilFBO

====================
*/
void SVD_CreateStencilFBO( void )
{
	if(!g_bFBOSupported)
		return;

	SCREENINFO	scrinfo;
	scrinfo.iSize = sizeof(scrinfo);
	GetScreenInfo(&scrinfo);

	// Set up main rendering target
	glGenRenderbuffers(1, &g_colorRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, g_colorRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, scrinfo.iWidth, scrinfo.iHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glGenRenderbuffers(1, &g_depthStencilRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, g_depthStencilRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scrinfo.iWidth, scrinfo.iHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glGenFramebuffers(1, &g_stencilFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, g_stencilFBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_colorRBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_depthStencilRBO);

	GLenum eStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(eStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		gEngfuncs.Con_Printf("%s - FBO creation failed. Code returned: %d.\n", __FUNCTION__, (int)glGetError());
		g_bFBOSupported = false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*
====================
SVD_VidInit

====================
*/
void SVD_VidInit( void )
{
	SVD_Clear();
}

/*
====================
SVD_Frame

====================
*/
void SVD_Frame( void )
{
	if(g_bAlertAboutMSAA)
	{
		gEngfuncs.Con_Printf("MSAA is enabled. Stencil shadows will be disabled. Add '-nofbo' to the launch parameters to disable MSAA.\n");
		g_bAlertAboutMSAA = false;
	}
}

/*
====================
SVD_Init

====================
*/
void SVD_Init( void )
{
	if(!R_IsExtensionSupported("EXT_framebuffer_object") && !R_IsExtensionSupported("ARB_framebuffer_object"))
	{
		gEngfuncs.Con_Printf("Your hardware does not support framebuffer objects. Stencil shadows will remain disabled.\n");
		g_bFBOSupported = false;
		return;
	}

	glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffers");
	glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
	glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress("glRenderbufferStorage");
	glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)wglGetProcAddress("glRenderbufferStorageMultisample");
	glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress("glFramebufferRenderbuffer");
	glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
	glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
	glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
	glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
	glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)wglGetProcAddress("glDeleteRenderbuffers");
	glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
	glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)wglGetProcAddress("glGetFramebufferAttachmentParameteriv");
	glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)wglGetProcAddress("glGetRenderbufferParameteriv");
	glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)wglGetProcAddress("glBlitFramebuffer");

	if(glGenRenderbuffers && glBindRenderbuffer && glRenderbufferStorage && glFramebufferRenderbuffer
		&& glFramebufferTexture2D && glCheckFramebufferStatus && glBindFramebuffer && glGenFramebuffers
		&& glDeleteRenderbuffers && glDeleteFramebuffers && glGetFramebufferAttachmentParameteriv
		&& glGetRenderbufferParameteriv && glBlitFramebuffer)
	{
		// Functions loaded fine
		g_bFBOSupported = true;
	}

	if(!R_CheckMSAA())
	{
		g_bAlertAboutMSAA = true;
		g_bFBOSupported = false;
	}
	else
	{
		// Create the stencil FBO
		SVD_CreateStencilFBO();
	}
}

/*
====================
SVD_Shutdown

====================
*/
void SVD_Shutdown( void )
{
	if(!g_bFBOSupported)
		return;

	if(g_colorRBO)
	{
		glDeleteRenderbuffers(1, &g_colorRBO);
		g_colorRBO = 0;
	}

	if(g_depthStencilRBO)
	{
		glDeleteRenderbuffers(1, &g_depthStencilRBO);
		g_depthStencilRBO = 0;
	}
}

/*
====================
SVD_RecursiveDrawWorld

====================
*/
void SVD_RecursiveDrawWorld ( mnode_t *node )
{
	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->visframe != g_visFrame)
		return;
	
	if (node->contents < 0)
		return;		// faces already marked by engine

	// recurse down the children, Order doesn't matter
	SVD_RecursiveDrawWorld (node->children[0]);
	SVD_RecursiveDrawWorld (node->children[1]);

	// draw stuff
	int c = node->numsurfaces;
	if (c)
	{
		msurface_t	*surf = g_pWorld->surfaces + node->firstsurface;

		for ( ; c ; c--, surf++)
		{
			if (surf->visframe != g_frameCount)
				continue;

			if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB|SURF_UNDERWATER))
				continue;

			glpoly_t *p = surf->polys;
			float *v = p->verts[0];

			glBegin (GL_POLYGON);			
			for (int i = 0; i < p->numverts; i++, v+= VERTEXSIZE)
			{
				glTexCoord2f (v[3], v[4]);
				glVertex3fv (v);
			}
			glEnd ();
		}
	}
}

/*
====================
SVD_CalcRefDef

====================
*/
void SVD_CalcRefDef ( ref_params_t* pparams )
{
	if(IEngineStudio.IsHardware() != 1)
		return;

	SVD_CheckInit();

	if(g_StudioRenderer.m_pCvarDrawShadows->value < 2)
		return;

	if(g_bFBOSupported)
	{
		// Bind the FBO to use
		glBindFramebuffer(GL_FRAMEBUFFER, g_stencilFBO);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, ScreenWidth, ScreenHeight);
	}

	// Might be using hacky DLL
	glClear( GL_STENCIL_BUFFER_BIT );
}

/*
====================
SVD_DrawTransparentTriangles

====================
*/
void SVD_DrawTransparentTriangles ( void )
{
	if(g_StudioRenderer.m_pCvarDrawShadows->value < 2)
		return;

	if(IEngineStudio.IsHardware() != 1)
		return;

	glPushAttrib(GL_TEXTURE_BIT);

	// Draw black fog
	gFog.RenderFog();

	// buz: workaround half-life's bug, when multitexturing left enabled after
	// rendering brush entities
	g_StudioRenderer.glActiveTexture( GL_TEXTURE1_ARB );
	glDisable(GL_TEXTURE_2D);
	g_StudioRenderer.glActiveTexture( GL_TEXTURE0_ARB );

	glDepthMask(GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(GL_ZERO, GL_ZERO, GL_ZERO, 0.5);

	glStencilFunc(GL_NOTEQUAL, 0, ~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glEnable(GL_STENCIL_TEST);

	// Set view origin
	g_viewOrigin = g_StudioRenderer.m_vRenderOrigin;

	// get current visframe number
	g_pWorld = IEngineStudio.GetModelByIndex(1);
	mleaf_t *pleaf = Mod_PointInLeaf ( g_StudioRenderer.m_vRenderOrigin, g_pWorld );
	g_visFrame = pleaf->visframe;

	// get current frame number
	g_frameCount = g_StudioRenderer.m_nFrameCount;

	// draw world
	SVD_RecursiveDrawWorld( g_pWorld->nodes );

	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);

	glPopAttrib();

	SVD_PerformFBOBlit();
}

/*
====================
SVD_PerformFBOBlit

====================
*/
void SVD_PerformFBOBlit ( void )
{
	// If supported, use FBO
	if(!g_bFBOSupported)
		return;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, g_stencilFBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, ScreenWidth, ScreenHeight, 0, 0, ScreenWidth, ScreenHeight, GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}