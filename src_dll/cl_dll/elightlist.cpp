/***+
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// Ammo.cpp
//
// implementation of CHudAmmo class
//

#include <Windows.h>

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "pm_shared.h"

#include <string.h>
#include <stdio.h>
#include "elightlist.h"

#include "pmtrace.h"
#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"
#include "pm_defs.h"

#include "elightlist.h"
#include "com_model.h"
#include "r_studioint.h"
#include "svd_render.h"

// Class declaration
CELightList gELightList;

extern engine_studio_api_t IEngineStudio;
extern char* COM_ReadLine( char* pstr, char* pstrOut );
extern void COM_ToLowerCase( char* pstr );

extern model_t* g_pWorld;
extern int g_msurfaceStructSize;

// Direct light value, same as in HLRAD
static const float DIRECT_LIGHT = 0.1;

// Testing function
void __CmdFunc_MakeLight( void )
{
	vec3_t lightColor;
	int entIndex = atoi(gEngfuncs.Cmd_Argv(1));
	lightColor[0] = atof(gEngfuncs.Cmd_Argv(2))/255.0f;
	lightColor[1] = atof(gEngfuncs.Cmd_Argv(3))/255.0f;
	lightColor[2] = atof(gEngfuncs.Cmd_Argv(4))/255.0f;
	float radius = atof(gEngfuncs.Cmd_Argv(5));

	gELightList.AddEntityLight(entIndex, gHUD.m_vecOrigin, lightColor, radius, false, EL_TYPE_NORMAL);
}

int __MsgFunc_ELight( const char *pszName, int iSize, void *pBuf )
{
	return gELightList.MsgFunc_ELight( pszName, iSize, pBuf );
}

/*
====================
Init

====================
*/
void CELightList::Init( void )
{
	gEngfuncs.pfnAddCommand("make_light", __CmdFunc_MakeLight);

	HOOK_MESSAGE(ELight);

	glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");

	m_pCvarDebugELights		= CVAR_CREATE( "r_debug_elights", "0", FCVAR_CLIENTDLL );
	m_pCvarLightMultiplier	= CVAR_CREATE( "gl_surflight_multiplier", "1", FCVAR_ARCHIVE );
}

/*
====================
VidInit

====================
*/
void CELightList::VidInit( void )
{
	memset(m_pEntityLights, 0, sizeof(m_pEntityLights));
	m_iNumEntityLights = NULL;

	memset(m_pTempEntityLights, 0, sizeof(m_pTempEntityLights));
	m_iNumTempEntityLights = NULL;

	// Get pointer to first elight
	m_pGoldSrcELights = gEngfuncs.pEfxAPI->CL_AllocElight(0);
	m_pGoldSrcDLights = gEngfuncs.pEfxAPI->CL_AllocDlight(0);

	m_readLightsRad = false;
}

/*
====================
AddEntityLight

====================
*/
void CELightList::AddEntityLight( int entindex, const vec3_t& origin, const vec3_t& color, float radius, bool isTemporary, float die )
{
	AddEntityLight(entindex, origin, color, radius, isTemporary, EL_TYPE_NORMAL, 0, die);
}

/*
====================
AddEntityLight

====================
*/
void CELightList::AddEntityLight( int entindex, const vec3_t& origin, const vec3_t& color, float radius, bool isTemporary, elighttype_t type, float strength, float die )
{
	if(type == EL_TYPE_MAP_SURFLIGHT)
	{
		gEngfuncs.Con_Printf("%s - Type 'EL_TYPE_MAP_SURFLIGHT' specified.\n", __FUNCTION__);
		return;
	}

	elight_t* plight = NULL;
	if (!isTemporary)
	{
		for (int i = 0; i < m_iNumEntityLights; i++)
		{
			if (m_pEntityLights[i].entindex == entindex)
			{
				plight = &m_pEntityLights[i];
				break;
			}
		}
	}
	else if(entindex != 0)
	{
		for (int i = 0; i < m_iNumTempEntityLights; i++)
		{
			if (m_pTempEntityLights[i].entindex == entindex)
			{
				plight = &m_pTempEntityLights[i];
				break;
			}
		}
	}

	if(!plight)
	{
		if(!isTemporary)
		{
			if(m_iNumEntityLights == MAX_ENTITY_LIGHTS)
				return;

			plight = &m_pEntityLights[m_iNumEntityLights];
			m_iNumEntityLights++;
		}
		else
		{
			if(m_iNumTempEntityLights == MAX_GOLDSRC_ELIGHTS)
				return;

			plight = &m_pTempEntityLights[m_iNumTempEntityLights];
			m_iNumTempEntityLights++;
		}
	}

	plight->origin = origin;
	plight->color = color;
	
	plight->entindex = entindex;
	plight->temporary = isTemporary;
	plight->die = die;
	plight->type = type;

	switch(type)
	{
	case EL_TYPE_NORMAL:
		{
			// Only this is needed
			plight->radius = radius;
			plight->intensity = 0;
		}
		break;
	case EL_TYPE_MAP_POINTLIGHT:
		{
			float finalIntensity =  strength * m_pCvarLightMultiplier->value;
			float minimumValue = 1.0 / 255;
			plight->radius = sqrt(finalIntensity / minimumValue);
			plight->intensity = strength * m_pCvarLightMultiplier->value;
		}
		break;
	}

	for(int i = 0; i < 3; i++)
	{
		plight->mins[i] = plight->origin[i] - plight->radius;
		plight->maxs[i] = plight->origin[i] + plight->radius;
	}
}

/*
====================
AddEntityLight

====================
*/
void CELightList::RemoveEntityLight( int entindex )
{
	for(int i = 0; i < m_iNumEntityLights; i++)
	{
		if(m_pEntityLights[i].entindex == entindex)
		{
			for(int j = i; j < m_iNumEntityLights-1; j++)
				m_pEntityLights[j] = m_pEntityLights[j+1];

			m_iNumEntityLights--;
			return;
		}
	}
}

/*
====================
GetLightList

====================
*/
void CELightList::GetLightList( vec3_t& origin, const vec3_t& mins, const vec3_t& maxs, elight_t** lightArray, unsigned int* numLights )
{
	// Set this to zero
	*numLights = NULL;

	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	for(int i = 0; i < m_iNumEntityLights; i++)
	{
		if((*numLights) == MAX_MODEL_ENTITY_LIGHTS)
			return;

		elight_t* plight = &m_pEntityLights[i];

		if(CheckBBox(plight, mins, maxs))
			continue;

		static pmtrace_t traceResult;
		gEngfuncs.pEventAPI->EV_PlayerTrace( (float *)origin, (float *)plight->origin, PM_WORLD_ONLY, -1, &traceResult );
		if(traceResult.fraction != 1.0 || traceResult.allsolid || traceResult.startsolid)
			continue;

		lightArray[*numLights] = plight;
		(*numLights)++;
	}

	for(int i = 0; i < m_iNumTempEntityLights; i++)
	{
		if((*numLights) == MAX_MODEL_ENTITY_LIGHTS)
			return;

		elight_t* plight = &m_pTempEntityLights[i];

		if(CheckBBox(plight, mins, maxs))
			continue;

		static pmtrace_t traceResult;
		gEngfuncs.pEventAPI->EV_PlayerTrace( (float *)origin, (float *)plight->origin, PM_WORLD_ONLY, -1, &traceResult );
		if(traceResult.fraction != 1.0 || traceResult.allsolid || traceResult.startsolid)
			continue;

		lightArray[*numLights] = plight;
		(*numLights)++;
	}
}

/*
====================
CheckBBox

====================
*/
bool CELightList::CheckBBox( elight_t* plight, const vec3_t& vmins, const vec3_t& vmaxs )
{
	if (vmins[0] > plight->maxs[0]) 
		return true;

	if (vmins[1] > plight->maxs[1]) 
		return true;

	if (vmins[2] > plight->maxs[2]) 
		return true;

	if (vmaxs[0] < plight->mins[0]) 
		return true;

	if (vmaxs[1] < plight->mins[1]) 
		return true;

	if (vmaxs[2] < plight->mins[2]) 
		return true;

	return false;
}

/*
====================
CalcRefDef

====================
*/
void CELightList::CalcRefDef( void )
{
	// Reset to zero
	m_iNumTempEntityLights = 0;

	float fltime = gEngfuncs.GetClientTime();

	dlight_t* pdlight = m_pGoldSrcELights;
	for(int i = 0; i < MAX_GOLDSRC_ELIGHTS; i++, pdlight++)
	{
		if(!pdlight->radius || pdlight->die < fltime)
			continue;

		vec3_t lightColor;
		lightColor.x = (float)pdlight->color.r/255.0f;
		lightColor.y = (float)pdlight->color.g/255.0f;
		lightColor.z = (float)pdlight->color.b/255.0f;

		AddEntityLight(pdlight->key, pdlight->origin, lightColor, pdlight->radius*8, true, pdlight->die);
	}

	pdlight = m_pGoldSrcDLights;
	for(int i = 0; i < MAX_GOLDSRC_DLIGHTS; i++, pdlight++)
	{
		if(!pdlight->radius || pdlight->die < fltime)
			continue;

		vec3_t lightColor;
		lightColor.x = (float)pdlight->color.r/255.0f;
		lightColor.y = (float)pdlight->color.g/255.0f;
		lightColor.z = (float)pdlight->color.b/255.0f;

		AddEntityLight(pdlight->key, pdlight->origin, lightColor, pdlight->radius, true, pdlight->die );
	}

	for (int i = 0; i < m_iNumTempEntityLights; i++)
	{
		elight_t& el = m_pTempEntityLights[i];
		if (el.die && el.die < fltime)
		{
			for (int j = i; j < m_iNumTempEntityLights - 1; j++)
				m_pTempEntityLights[j] = m_pTempEntityLights[j + 1];

			m_iNumEntityLights--;
			i--;
			continue;
		}
	}

	if(!m_readLightsRad)
	{
		g_pWorld = IEngineStudio.GetModelByIndex(1);
		ReadLightsRadFile();
		m_readLightsRad = true;
	}
}

/*
====================
DrawNormal

====================
*/
void CELightList::DrawNormal( void )
{
	if(m_pCvarDebugELights->value < 1)
		return;

	// Push texture state
	glPushAttrib(GL_TEXTURE_BIT);

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE3);
	glDisable(GL_TEXTURE_2D);

	// Set the active texture unit
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);

	// Draw elight positions
	glDisable(GL_DEPTH_TEST);
	
	glPointSize(4.0);
	glBegin(GL_POINTS);
	for(int i = 0; i < m_iNumEntityLights; i++)
	{
		elight_t& el = m_pEntityLights[i];

		glColor4f(el.color.x, el.color.y, el.color.z, 1.0);
		glVertex3fv(el.origin);
	}
	glEnd();

	glEnable(GL_DEPTH_TEST);
	glPopAttrib();
}

/*
====================
MsgFunc_ELight

====================
*/
int CELightList::MsgFunc_ELight( const char *pszName, int iSize, void *pBuf )
{
	BEGIN_READ(pBuf, iSize);

	int entindex = READ_SHORT();
	bool active = READ_BYTE() ? true : false;

	if(!active)
	{
		RemoveEntityLight(entindex);
		return 1;
	}

	vec3_t origin;
	for(int i = 0; i < 3; i++)
		origin[i] = READ_COORD();

	Vector color;
	for(int i = 0; i < 3; i++)
		color[i] = READ_BYTE();

	bool isMapLight = READ_BYTE() == 1 ? true : false;

	elighttype_t type;
	float strength;
	float radius;

	if(isMapLight)
	{
		strength = READ_COORD();
		type = EL_TYPE_MAP_POINTLIGHT;
		radius = 0;

		for(int i = 0; i < 3; i++)
			color[i] = pow( color[i] / 114.0, 0.6 ) * 264;
	}
	else
	{
		radius = READ_COORD()*9.5;
		type = EL_TYPE_NORMAL;
		strength = 0;
	}

	// Convert from 0-255 to 0-1
	for(int i = 0; i < 3; i++)
		color[i] = color[i] / 255.0f;

	AddEntityLight(entindex, origin, color, radius, false, type, strength);

	return 1;
}

/*
====================
ReadLightsRadFile

====================
*/
void CELightList::ReadLightsRadFile( void )
{
	m_numTexLights = 0;

	int length = 0;
	char* pFile = (char*)gEngfuncs.COM_LoadFile("lights.rad", 5, &length);
	if(!pFile)
		return;

	char szLine[1024];
	char szToken[128];

	char* pstr = pFile;
	while(pstr)
	{
		pstr = COM_ReadLine(pstr, szLine);
		if(!pstr)
			break;

		if(strlen(szLine) <= 0)
			continue;

		if(!strncmp(szLine, "//", 2))
			continue;

		char textureName[64];
		int colorR = 0;
		int colorG = 0;
		int colorB = 0;

		// Get light texture name
		char* plstr = gEngfuncs.COM_ParseFile(szLine, textureName);
		if(!plstr)
		{
			gEngfuncs.Con_Printf("Incomplete entry in lights.rad for '%s'.\n", textureName);
			continue;
		}

		// Get color elements
		plstr = gEngfuncs.COM_ParseFile(plstr, szToken);
		if(!plstr)
		{
			gEngfuncs.Con_Printf("Incomplete entry in lights.rad for '%s'.\n", textureName);
			continue;
		}

		colorR = atoi(szToken);
		if(colorR < 0)
			colorR = 0;
		else if(colorR > 255)
			colorR = 255;

		plstr = gEngfuncs.COM_ParseFile(plstr, szToken);
		if(!plstr)
		{
			gEngfuncs.Con_Printf("Incomplete entry in lights.rad for '%s'.\n", textureName);
			continue;
		}

		colorG = atoi(szToken);
		if(colorG < 0)
			colorG = 0;
		else if(colorG > 255)
			colorG = 255;

		plstr = gEngfuncs.COM_ParseFile(plstr, szToken);
		if(!plstr)
		{
			gEngfuncs.Con_Printf("Incomplete entry in lights.rad for '%s'.\n", textureName);
			continue;
		}

		colorB = atoi(szToken);
		if(colorB < 0)
			colorB = 0;
		else if(colorB > 255)
			colorB = 255;

		colorR = pow( colorR / 114.0, 0.6 ) * 264;
		colorG = pow( colorG / 114.0, 0.6 ) * 264;
		colorB = pow( colorB / 114.0, 0.6 ) * 264;

		// Get strength
		plstr = gEngfuncs.COM_ParseFile(plstr, szToken);

		float strength = atoi(szToken);
		if(strength < 0)
			strength = 0;

		if(m_numTexLights >= MAX_TEXLIGHTS)
		{
			gEngfuncs.Con_Printf("Exceeded MAX_TEXLIGHTS\n");
			break;
		}

		texlight_t* pnew = &m_texLights[m_numTexLights];
		m_numTexLights++;

		strcpy(pnew->texname, textureName);
		COM_ToLowerCase(pnew->texname);
		pnew->colorr = colorR;
		pnew->colorg = colorG;
		pnew->colorb = colorB;
		pnew->strength = strength;
	}

	gEngfuncs.COM_FreeFile(pFile);

	// Go through the world and find surfaces tied to this
	const model_t* pWorld = IEngineStudio.GetModelByIndex(1);
	if(!pWorld)
		return;

	char texName[64];
	int numStuckInSolid = 0;

	if (!g_msurfaceStructSize)
		g_msurfaceStructSize = R_DetermineSurfaceStructSize();

	// Parse through all surfaces and build lit surface array
	byte* pfirstsurfbyteptr = reinterpret_cast<byte*>(g_pWorld->surfaces);
	for(int i = 0; i < pWorld->numsurfaces; i++)
	{
		msurface_t* psurface = reinterpret_cast<msurface_t*>(pfirstsurfbyteptr + g_msurfaceStructSize * (g_pWorld->firstmodelsurface + i));

		// See if surface binds to a texlight
		mtexinfo_t* ptexinfo = psurface->texinfo;
		if(!ptexinfo->texture)
			continue;

		strcpy(texName, ptexinfo->texture->name);
		COM_ToLowerCase(texName);

		texlight_t* ptexlight = NULL;
		for(int j = 0; j < m_numTexLights; j++)
		{
			if(!strcmpi(m_texLights[j].texname, texName))
			{
				ptexlight = &m_texLights[j];
				break;
			}
		}

		if(!ptexlight)
			continue;

		// Compile vertices into an array
		std::vector<Vector> pointsVector(psurface->numedges);
		for(int j = 0; j < psurface->numedges; j++)
		{
			Vector vertexOrigin;
			int edgeIndex = pWorld->surfedges[psurface->firstedge+j];
			if(edgeIndex > 0)
				vertexOrigin = pWorld->vertexes[pWorld->edges[edgeIndex].v[0]].position;
			else
				vertexOrigin = pWorld->vertexes[pWorld->edges[-edgeIndex].v[1]].position;

			pointsVector[j] = vertexOrigin;
		}

		// Calculate surface area
		float surfArea = 0;
		for (int j = 2 ; j < pointsVector.size(); j++)
		{
			Vector d1, d2, cross;
			VectorSubtract (pointsVector[j-1], pointsVector[0], d1);
			VectorSubtract (pointsVector[j], pointsVector[0], d2);
			cross = CrossProduct (d1, d2);
			surfArea += 0.5 * cross.Length();
		}

		if(surfArea < 1.0)
			continue;

		// Calculate winding center
		Vector center = Vector(0, 0, 0);
		for (int j = 0; j < pointsVector.size(); j++)
			VectorAdd(pointsVector[j], center, center);

		float scale = 1.0/pointsVector.size();
		VectorScale (center, scale, center);
		
		float sign = (psurface->flags & SURF_PLANEBACK) ? -1 : 1;
		VectorMA(center, 1.0f * sign, psurface->plane->normal, center);
		int contents = gEngfuncs.PM_PointContents(center, NULL);
		if(contents == CONTENTS_SOLID)
		{
			numStuckInSolid++;
			continue;
		}

		if(m_iNumEntityLights == MAX_ENTITY_LIGHTS)
		{
			gEngfuncs.Con_Printf("Exceeded MAX_ENTITY_LIGHTS.\n");
			break;
		}

		elight_t* pel = &m_pEntityLights[m_iNumEntityLights];
		m_iNumEntityLights++;

		pel->origin = center;

		float scaler = ((float)ptexlight->strength / 255.0f);
		pel->color.x = ((float)ptexlight->colorr / 255.0f);
		pel->color.y = ((float)ptexlight->colorg / 255.0f);
		pel->color.z = ((float)ptexlight->colorb / 255.0f);
		
		pel->temporary = false;
		pel->entindex = -1;
		pel->type = EL_TYPE_MAP_SURFLIGHT;

		VectorCopy(psurface->plane->normal, pel->normal);
		if(psurface->flags & SURF_PLANEBACK)
			VectorInverse(pel->normal);

		VectorNormalize(pel->normal);

		// Same calculations as qrad
		float intensityNoColor = DIRECT_LIGHT * scaler * surfArea * m_pCvarLightMultiplier->value;
		float minimumValue = 1.0 / 255;
		pel->radius = sqrt(intensityNoColor / minimumValue);
		pel->intensity = intensityNoColor;

		for(int j = 0; j < 3; j++)
		{
			pel->mins[j] = pel->origin[j] - pel->radius;
			pel->maxs[j] = pel->origin[j] + pel->radius;
		}
	}

	gEngfuncs.Con_Printf("Removed %d per-vertex lights stuck in solids.\n", numStuckInSolid);
}