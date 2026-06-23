//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ELIGHT_H
#define ELIGHT_H

#define MAX_MODEL_ENTITY_LIGHTS	32

enum elighttype_t
{
	EL_TYPE_NORMAL = 0,
	EL_TYPE_MAP_SURFLIGHT,
	EL_TYPE_MAP_POINTLIGHT
};

struct elight_t
{
	int entindex;

	vec3_t origin;
	vec3_t color;
	vec3_t normal;
	float radius;
	float intensity;

	vec3_t mins;
	vec3_t maxs;

	bool temporary;
	elighttype_t type;
	float die;
};
#endif