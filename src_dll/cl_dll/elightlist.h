//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined ( ELIGHTLIST_H )
#define ELIGHTLIST_H
#if defined( _WIN32 )
#pragma once
#endif

#include <Windows.h>

#include "elight.h"
#include "dlight.h"
#include <vector>
#include "com_model.h"

#include "gl/gl.h"
#include "gl/glext.h"

#define MAX_ENTITY_LIGHTS		1024
#define MAX_GOLDSRC_ELIGHTS		256
#define MAX_GOLDSRC_DLIGHTS		32
#define MAX_TEXLIGHTS			1024

/*
====================
CELightList

====================
*/
class CELightList
{
public:
	struct texlight_t
	{
		char texname[64];
		int colorr;
		int colorg;
		int colorb;
		int strength;
	};

	struct lightsurface_t
	{
		texture_t* ptexture;
		texlight_t* ptexlight;
		Vector normal;

		Vector mins;
		Vector maxs;

		std::vector<Vector> verts;
	};

public:
	void Init( void );
	void VidInit( void );
	void CalcRefDef( void );
	void ReadLightsRadFile( void );
	void DrawNormal( void );

	int MsgFunc_ELight( const char *pszName, int iSize, void *pBuf );

	void AddEntityLight( int entindex, const vec3_t& origin, const vec3_t& color, float radius, bool isTemporary );
	void RemoveEntityLight( int entindex );

	void GetLightList( vec3_t& origin, const vec3_t& mins, const vec3_t& maxs, elight_t** lightArray, unsigned int* numLights );

	bool CheckBBox( elight_t* plight, const vec3_t& vmins, const vec3_t& vmaxs );

private:
	elight_t	m_pEntityLights[MAX_ENTITY_LIGHTS];
	int			m_iNumEntityLights;

	elight_t	m_pTempEntityLights[MAX_GOLDSRC_ELIGHTS];
	int			m_iNumTempEntityLights;

	dlight_t	*m_pGoldSrcELights;
	dlight_t	*m_pGoldSrcDLights;

	texlight_t	m_texLights[MAX_TEXLIGHTS];
	int			m_numTexLights;

	bool		m_readLightsRad;

	PFNGLACTIVETEXTUREPROC			glActiveTexture;

	cvar_t*		m_pCvarDebugELights;
};

extern CELightList gELightList;
#endif // ELIGHTLIST_H