//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================


#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "lightstyle.h"

// Lightstyles object
CLightStyle gLightStyles;

int __MsgFunc_LightStyle( const char *pszName, int iSize, void *pBuf )
{
	return gLightStyles.MsgFunc_LightStyle( pszName, iSize, pBuf );
}

/*
====================
Init

====================
*/
void CLightStyle::Init( void )
{
	HOOK_MESSAGE( LightStyle );
}

/*
====================
VidInit

====================
*/
void CLightStyle::VidInit( void )
{
	m_numLightStyles = 0;
	memset(m_lightStyles, 0, sizeof(m_lightStyles));
	memset(m_lightStyleValues, 0, sizeof(m_lightStyleValues));
}

/*
====================
MsgFunc_Fog

====================
*/
int CLightStyle::MsgFunc_LightStyle( const char *pszName, int iSize, void *pBuf )
{
	BEGIN_READ( pBuf, iSize );
	int index = READ_BYTE();
	const char* pstrStyle = READ_STRING();

	SetStyle(index, pstrStyle);
	return 1;
}

/*
====================
SetStyle

====================
*/
void CLightStyle::SetStyle( int index, const char* pstrStyle )
{
	if(index >= MAX_LIGHTSTYLES)
	{
		gEngfuncs.Con_Printf("%s - Lightstyle index '%d' exceeds MAX_LIGHTSTYLES.\n", __FUNCTION__, index);
		return;
	}

	if(strlen(pstrStyle) >= MAX_PATTERN_LENGTH)
	{
		gEngfuncs.Con_Printf("%s - Lightstyle index '%d' pattern '%s' exceeds MAX_PATTERN_LENGTH.\n", __FUNCTION__, index, pstrStyle);
		return;
	}

	lightstyle_t* pstyle = NULL;
	for(int i = 0; i < m_numLightStyles; i++)
	{
		lightstyle_t& style = m_lightStyles[i];
		if(style.index == index)
		{
			pstyle = &style;
			break;
		}
	}

	if(!pstyle)
	{
		if(m_numLightStyles == MAX_LIGHTSTYLES)
		{
			gEngfuncs.Con_Printf("%s - Exceeded MAX_LIGHTSTYLES.\n", __FUNCTION__);
			return;
		}
		else
		{
			pstyle = &m_lightStyles[m_numLightStyles];
			m_numLightStyles++;
		}
	}

	strcpy(pstyle->pattern, pstrStyle);
	pstyle->length = strlen(pstrStyle);
	pstyle->index = index;
}

/*
====================
Animate

====================
*/
void CLightStyle::Animate( void )
{
	float time = gEngfuncs.GetClientTime();
	int i = (int)(time * 10);
	for (int j = 0; j < m_numLightStyles; j++)
	{
		lightstyle_t& style = m_lightStyles[j];
		if (!style.length)
		{
			m_lightStyleValues[j] = 1.0;
			continue;
		}

		int k = i % style.length;
		k = style.pattern[k] - 'a';
		k = k * 22;
		m_lightStyleValues[style.index] = (float)k / 255.0f;
	}
}

/*
====================
GetLightStyleValue

====================
*/
float CLightStyle::GetLightStyleValue( int index )
{
	if(index >= MAX_LIGHTSTYLES)
		return 1.0;
	else
		return m_lightStyleValues[index];
}