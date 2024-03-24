//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <memory.h>
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio_util.h"
#include "r_studioint.h"
#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "pmtrace.h"
#include "pm_defs.h"

extern engine_studio_api_t IEngineStudio;

/*
====================
AngleMatrix

====================
*/
void AngleMatrix (const float *angles, float (*matrix)[4] )
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	
	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp*cy;
	matrix[1][0] = cp*sy;
	matrix[2][0] = -sp;
	matrix[0][1] = sr*sp*cy+cr*-sy;
	matrix[1][1] = sr*sp*sy+cr*cy;
	matrix[2][1] = sr*cp;
	matrix[0][2] = (cr*sp*cy+-sr*-sy);
	matrix[1][2] = (cr*sp*sy+-sr*cy);
	matrix[2][2] = cr*cp;
	matrix[0][3] = 0.0;
	matrix[1][3] = 0.0;
	matrix[2][3] = 0.0;
}

/*
====================
VectorCompare

====================
*/
int VectorCompare (const float *v1, const float *v2)
{
	int		i;
	
	for (i=0 ; i<3 ; i++)
		if (v1[i] != v2[i])
			return 0;
			
	return 1;
}

/*
====================
CrossProduct

====================
*/
void CrossProduct (const float *v1, const float *v2, float *cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

/*
====================
VectorTransform

====================
*/
void VectorTransform (const float *in1, float in2[3][4], float *out)
{
	out[0] = DotProduct(in1, in2[0]) + in2[0][3];
	out[1] = DotProduct(in1, in2[1]) + in2[1][3];
	out[2] = DotProduct(in1, in2[2]) + in2[2][3];
}

/*
================
ConcatTransforms

================
*/
void ConcatTransforms (float in1[3][4], float in2[3][4], float out[3][4])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
				in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
				in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
				in1[2][2] * in2[2][3] + in1[2][3];
}

// angles index are not the same as ROLL, PITCH, YAW

/*
====================
AngleQuaternion

====================
*/
void AngleQuaternion( float *angles, vec4_t quaternion )
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	// FIXME: rescale the inputs to 1/2 angle
	angle = angles[2] * 0.5;
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[1] * 0.5;
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[0] * 0.5;
	sr = sin(angle);
	cr = cos(angle);

	quaternion[0] = sr*cp*cy-cr*sp*sy; // X
	quaternion[1] = cr*sp*cy+sr*cp*sy; // Y
	quaternion[2] = cr*cp*sy-sr*sp*cy; // Z
	quaternion[3] = cr*cp*cy+sr*sp*sy; // W
}

/*
====================
QuaternionSlerp

====================
*/
void QuaternionSlerp( vec4_t p, vec4_t q, float t, vec4_t qt )
{
	int i;
	float	omega, cosom, sinom, sclp, sclq;

	// decide if one of the quaternions is backwards
	float a = 0;
	float b = 0;

	for (i = 0; i < 4; i++)
	{
		a += (p[i]-q[i])*(p[i]-q[i]);
		b += (p[i]+q[i])*(p[i]+q[i]);
	}
	if (a > b)
	{
		for (i = 0; i < 4; i++)
		{
			q[i] = -q[i];
		}
	}

	cosom = p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];

	if ((1.0 + cosom) > 0.000001)
	{
		if ((1.0 - cosom) > 0.000001)
		{
			omega = acos( cosom );
			sinom = sin( omega );
			sclp = sin( (1.0 - t)*omega) / sinom;
			sclq = sin( t*omega ) / sinom;
		}
		else
		{
			sclp = 1.0 - t;
			sclq = t;
		}
		for (i = 0; i < 4; i++) {
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	}
	else
	{
		qt[0] = -q[1];
		qt[1] = q[0];
		qt[2] = -q[3];
		qt[3] = q[2];
		sclp = sin( (1.0 - t) * (0.5 * M_PI));
		sclq = sin( t * (0.5 * M_PI));
		for (i = 0; i < 3; i++)
		{
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
}

/*
====================
QuaternionMatrix

====================
*/
void QuaternionMatrix( vec4_t quaternion, float (*matrix)[4] )
{
	matrix[0][0] = 1.0 - 2.0 * quaternion[1] * quaternion[1] - 2.0 * quaternion[2] * quaternion[2];
	matrix[1][0] = 2.0 * quaternion[0] * quaternion[1] + 2.0 * quaternion[3] * quaternion[2];
	matrix[2][0] = 2.0 * quaternion[0] * quaternion[2] - 2.0 * quaternion[3] * quaternion[1];

	matrix[0][1] = 2.0 * quaternion[0] * quaternion[1] - 2.0 * quaternion[3] * quaternion[2];
	matrix[1][1] = 1.0 - 2.0 * quaternion[0] * quaternion[0] - 2.0 * quaternion[2] * quaternion[2];
	matrix[2][1] = 2.0 * quaternion[1] * quaternion[2] + 2.0 * quaternion[3] * quaternion[0];

	matrix[0][2] = 2.0 * quaternion[0] * quaternion[2] + 2.0 * quaternion[3] * quaternion[1];
	matrix[1][2] = 2.0 * quaternion[1] * quaternion[2] - 2.0 * quaternion[3] * quaternion[0];
	matrix[2][2] = 1.0 - 2.0 * quaternion[0] * quaternion[0] - 2.0 * quaternion[1] * quaternion[1];
}

/*
====================
MatrixCopy

====================
*/
void MatrixCopy( float in[3][4], float out[3][4] )
{
	memcpy( out, in, sizeof( float ) * 3 * 4 );
}

void VectorRotate (const float *in1, float in2[3][4], float *out)
{
	out[0] = DotProduct(in1, in2[0]);
	out[1] = DotProduct(in1, in2[1]);
	out[2] = DotProduct(in1, in2[2]);
}

 void VectorIRotate (const float *in1, const float in2[3][4], float *out)
{
	out[0] = in1[0]*in2[0][0] + in1[1]*in2[1][0] + in1[2]*in2[2][0];
	out[1] = in1[0]*in2[0][1] + in1[1]*in2[1][1] + in1[2]*in2[2][1];
	out[2] = in1[0]*in2[0][2] + in1[1]*in2[1][2] + in1[2]*in2[2][2];
}

__forceinline float Q_rsqrt( float number )
{
        long i;
        float x2, y;
        const float threehalfs = 1.5F;
 
        x2 = number * 0.5F;
        y  = number;
        i  = * ( long * ) &y;                       // evil floating point bit level hacking
        i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
        y  = * ( float * ) &i;
        y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration

        return y;
}

void VectorNormalizeFast ( float *v )
{
	float ilength = DotProduct(v, v);
	float sqroot = Q_rsqrt(ilength);
	VectorScale(v, sqroot, v);
}

/*
==================
ParseColor

==================
*/
void ParseColor (vec3_t &out, color24 *lightmap)
{
	out[0] = (float)lightmap->r / 255.0f;
	out[1] = (float)lightmap->g / 255.0f;
	out[2] = (float)lightmap->b / 255.0f;
}

//===========================================
// RecursiveLightPoint
//
//===========================================
int RecursiveLightPoint( model_t* pworld, mnode_t *node, const vec3_t &start, const vec3_t &end, vec3_t &color )
{
	float		front, back, frac;
	int			side;
	mplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;
	color24		*lightmap;

	if (node->contents < 0)
		return FALSE;		// didn't hit anything
	
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;
	
	if ( (back < 0) == side )
		return RecursiveLightPoint (pworld, node->children[side], start, end, color);
	
	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;
	
// go down front side	
	int r = RecursiveLightPoint (pworld, node->children[side], start, mid, color);

	if (r) 
		return TRUE;
		
	if ((back < 0) == side)
		return FALSE;
	
	surf = pworld->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->flags & (SURF_DRAWTILED | SURF_DRAWSKY))
			continue;	// no lightmaps

		int index = node->firstsurface+i;
		tex = surf->texinfo;
		s = DotProduct(mid, tex->vecs[0])+tex->vecs[0][3];
		t = DotProduct(mid, tex->vecs[1])+tex->vecs[1][3];

		if (s < surf->texturemins[0] ||
		t < surf->texturemins[1])
			continue;
		
		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];
		
		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

		if (!surf->samples)
			continue;

		ds >>= 4;
		dt >>= 4;

		lightmap = surf->samples;

		if (lightmap)
		{
			int surfindex = node->firstsurface+i;
			int size = ((surf->extents[1]>>4)+1)*((surf->extents[0]>>4)+1);
			lightmap += dt * ((surf->extents[0]>>4)+1) + ds;

			float flIntensity = (lightmap->r + lightmap->g + lightmap->b)/3;
			float flScale = flIntensity/50;

			if(flScale > 1.0) 
				flScale = 1.0;

			ParseColor(color, lightmap);
			VectorScale(color, flScale, color);
		}	
		else
		{
			color[0] = color[1] = color[2] = 0.5;
		}
		return TRUE;
	}

// go down back side
	return RecursiveLightPoint (pworld, node->children[!side], mid, end, color);
}

//===========================================
// GetModelLighting
//
//===========================================
void GetModelLighting( const Vector& lightposition, int effects, const Vector& skyVector, const Vector& skyColor, float directLight, alight_t& lighting )
{
	Vector lightdirection;
	Vector lightcolor;

	// First try getting it from the sky
	bool gotlighting = false;
	if(skyColor[0] != 0 || skyColor[1] != 0 || skyColor[2] != 0)
	{
		Vector vecSrc = lightposition + Vector(0, 0, 8);
		Vector vecEnd = vecSrc - skyVector * 8192;

		pmtrace_t tr;
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_WORLD_ONLY, -1, &tr );

		if(tr.fraction != 1.0 && gEngfuncs.PM_PointContents(tr.endpos, NULL) == CONTENTS_SKY)
		{
			lightcolor = skyColor;
			lightdirection = skyVector;
			gotlighting = true;
		}
	}

	// Then try lightmap
	if(!gotlighting)
	{
		if(effects & EF_INVLIGHT)
			lightdirection[2] = 1;
		else
			lightdirection[2] = -1;

		model_t* pworld = IEngineStudio.GetModelByIndex(1);
		if(!pworld || !pworld->lightdata)
		{
			lightcolor = Vector(1.0, 1.0, 1.0);
			lightdirection = Vector(0, 0, -1);
		}
		else
		{
			Vector vecSrc;
			Vector vecEnd;

			if(effects & EF_INVLIGHT)
			{
				vecSrc = lightposition - Vector(0, 0, 8);
				vecEnd = vecSrc + Vector(0, 0, 8192);
			}
			else
			{
				vecSrc = lightposition + Vector(0, 0, 8);
				vecEnd = vecSrc - Vector(0, 0, 8192);
			}

			// Get main sample
			if(RecursiveLightPoint(pworld, pworld->nodes, vecSrc, vecEnd, lightcolor))
			{
				Vector color;
				float values[4];

				// Get sample 1
				Vector shiftSrc = vecSrc - Vector(16, 16, 0);
				Vector shiftEnd = vecEnd - Vector(16, 16, 0);

				RecursiveLightPoint(pworld, pworld->nodes, shiftSrc, shiftEnd, color);
				values[0] = (color.x + color.y + color.z) / 3.0f;

				// Get sample 2
				shiftSrc = shiftSrc + Vector(32, 0, 0);
				shiftEnd = shiftEnd + Vector(32, 0, 0);

				RecursiveLightPoint(pworld, pworld->nodes, shiftSrc, shiftEnd, color);
				values[1] = (color.x + color.y + color.z) / 3.0f;

				// Get sample 3
				shiftSrc = shiftSrc + Vector(0, 32, 0);
				shiftEnd = shiftEnd + Vector(0, 32, 0);

				RecursiveLightPoint(pworld, pworld->nodes, shiftSrc, shiftEnd, color);
				values[2] = (color.x + color.y + color.z) / 3.0f;

				// Get sample 4
				shiftSrc = shiftSrc - Vector(32, 0, 0);
				shiftEnd = shiftEnd - Vector(32, 0, 0);

				RecursiveLightPoint(pworld, pworld->nodes, shiftSrc, shiftEnd, color);
				values[3] = (color.x + color.y + color.z) / 3.0f;

				lightdirection[0] = values[0] - values[1] - values[2] + values[3];
				lightdirection[1] = values[1] + values[0] - values[2] - values[3];

				VectorNormalizeFast(lightdirection);
			}
		}
	}

	// Calculate intensity
	float intensity;
	if(lightcolor.x > lightcolor.y && lightcolor.x > lightcolor.z)
		intensity = lightcolor.x;
	else if(lightcolor.y > lightcolor.z)
		intensity = lightcolor.y;
	else
		intensity = lightcolor.z;

	intensity *= 255;
	if(!intensity)
		intensity = 1.0;

	VectorScale(lightdirection, intensity * directLight, lightdirection);
	
	lighting.shadelight = lightdirection.Length();
	lighting.ambientlight = intensity - lighting.shadelight;
	
	if(intensity != 0)
		VectorScale(lightcolor, 255.0f/intensity, lighting.color);
	else
		lighting.color = lightcolor;

	VectorNormalizeFast(lighting.color);

	if(lighting.ambientlight > 128)
		lighting.ambientlight = 128;

	if(lighting.ambientlight + lighting.shadelight > 255)
		lighting.shadelight = 255 - lighting.ambientlight;

	VectorNormalize(lightdirection);
	if(lighting.plightvec)
		VectorCopy(lightdirection, lighting.plightvec);
}