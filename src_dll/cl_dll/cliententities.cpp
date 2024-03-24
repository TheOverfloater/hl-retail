//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "cliententities.h"

/*
====================
Init

====================
*/
void CEntityManager::Init( void )
{
	// create the cvar
	m_pCvarDrawClientEntities = gEngfuncs.pfnRegisterVariable("r_drawcliententities", "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
}

/*
====================
VidInit

====================
*/
void CEntityManager::VidInit( void )
{
	memset(m_entities, 0, sizeof(m_entities));
	m_numEntities = 0;
}

