//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined ( LIGHTSTYLE_H )
#define LIGHTSTYLE_H
#if defined( _WIN32 )
#pragma once
#endif

// Same as in util.cpp
#define MAX_PATTERN_LENGTH	255
#define MAX_LIGHTSTYLES		128

/*
====================
CLightStyle

====================
*/
class CLightStyle
{
public:
	struct lightstyle_t
	{
		char pattern[MAX_PATTERN_LENGTH];
		int length;
		int index;
	};

public:
	void Init( void );
	void VidInit( void );
	int MsgFunc_LightStyle( const char *pszName, int iSize, void *pBuf );

	void SetStyle( int index, const char* pstrStyle );
	void Animate( void );
	float GetLightStyleValue( int index );

private:
	lightstyle_t m_lightStyles[MAX_LIGHTSTYLES];
	int m_numLightStyles;

	float m_lightStyleValues[MAX_LIGHTSTYLES];
};
extern CLightStyle gLightStyles;
#endif