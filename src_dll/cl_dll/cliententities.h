//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CLIENTENTITIES_H
#define CLIENTENTITIES_H

#define MAX_CL_ENTITIES 1024

/*
====================
CEntityManager

====================
*/
class CEntityManager
{
public:
	CEntityManager( void );
	~CEntityManager( void );

public:
	void Init( void );
	void VidInit( void );

private:
	// array of entities to be rendered
	cl_entity_t m_entities[MAX_CL_ENTITIES];
	int m_numEntities;

	// cvar for controlling the rendering of cl ents
	cvar_t* m_pCvarDrawClientEntities;
};
#endif //CLIENTENTITIES_H