/***
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

#include "hud.h"
#include "cl_util.h"
#include "mp3.h"

// MP3 support added by Killar
// Extended by Overfloater

// Default music channel
const int CMP3::MUSIC_CHANNEL_NUM = 0;

CMP3 gMP3;

/*
====================
MP3_OnFinishedCallback

====================
*/
signed char __stdcall MP3_OnFinishedCallback( FSOUND_STREAM *stream, void *buff, int len, int param )
{
	// Call MP3 player class to perform cleanup
	gMP3.OnPlaybackFinished(stream);
	return 0;
}

/*
====================
Init

====================
*/
bool CMP3::Init( void )
{
	// Build path to fmod binary
	char szPath[MAX_PATH];
	sprintf( szPath, "%s/fmod.dll", gEngfuncs.pfnGetGameDirectory() );

	// Remove any slashes
	for( size_t i = 0; i < strlen(szPath); i++ )
	{
		if( szPath[i] == '/' ) 
			szPath[i] = '\\';
	}

	m_hFMod = LoadLibrary( szPath );
	if(!m_hFMod)
	{
		gEngfuncs.Con_Printf("Fatal Error: FMOD library at path '%s' couldn't be loaded!\n");
		m_isAvailable = false;
		return false;
	}

	(FARPROC&)m_FSOUND_GetVersion = GetProcAddress(m_hFMod, "_FSOUND_GetVersion@0");
	(FARPROC&)m_FSOUND_Stream_Close = GetProcAddress(m_hFMod, "_FSOUND_Stream_Close@4");
	(FARPROC&)m_FSOUND_SetOutput = GetProcAddress(m_hFMod, "_FSOUND_SetOutput@4");
	(FARPROC&)m_FSOUND_SetBufferSize = GetProcAddress(m_hFMod, "_FSOUND_SetBufferSize@4");
	(FARPROC&)m_FSOUND_SetDriver = GetProcAddress(m_hFMod, "_FSOUND_SetDriver@4");
	(FARPROC&)m_FSOUND_Init = GetProcAddress(m_hFMod, "_FSOUND_Init@12");
	(FARPROC&)m_FSOUND_Stream_Open = GetProcAddress(m_hFMod, "_FSOUND_Stream_Open@16");
	(FARPROC&)m_FSOUND_Stream_Play = GetProcAddress(m_hFMod, "_FSOUND_Stream_Play@8");
	(FARPROC&)m_FSOUND_Stream_SetTime = GetProcAddress(m_hFMod, "_FSOUND_Stream_SetTime@8");
	(FARPROC&)m_FSOUND_Stream_GetLengthMs = GetProcAddress(m_hFMod, "_FSOUND_Stream_GetLengthMs@4");
	(FARPROC&)m_FSOUND_Stream_SetEndCallback = GetProcAddress(m_hFMod, "_FSOUND_Stream_SetEndCallback@12");
	(FARPROC&)m_FSOUND_SetPaused = GetProcAddress(m_hFMod, "_FSOUND_SetPaused@8");
	(FARPROC&)m_FSOUND_Close = GetProcAddress(m_hFMod, "_FSOUND_Close@0");
		
	if( !(m_FSOUND_Stream_Close && m_FSOUND_Stream_Open && m_FSOUND_SetBufferSize && m_FSOUND_SetDriver && m_FSOUND_Init 
		&& m_FSOUND_Stream_Open && m_FSOUND_Stream_Play && m_FSOUND_Stream_GetLengthMs && m_FSOUND_Close && m_FSOUND_Stream_SetEndCallback
		&& m_FSOUND_SetPaused) )
	{
		gEngfuncs.Con_Printf("Fatal Error: FMOD functions couldn't be loaded.\n");
		FreeLibrary( m_hFMod );
		m_isAvailable = false;
		return false;
	}
	else
	{
		gEngfuncs.Con_Printf("FMOD initialization succesfull.\n");
		m_isAvailable = true;
		return true;
	}
}

/*
====================
Init

====================
*/
void CMP3::VidInit( void )
{
	// Stop any playing sounds
	StopMP3(NULL);

	// Reset this
	m_isPaused = false;
	m_gotFrame = false;
	m_wasReset = true;
	m_gotEntities = false;
	m_gotRender = false;
}

/*
====================
Init

====================
*/
void CMP3::Shutdown( void )
{
	if( !m_hFMod )
		return;

	m_FSOUND_Close();
	FreeLibrary( m_hFMod );

	m_hFMod = NULL;
	m_isPlaying = false;
	m_playbackStarted = false;
	m_isAvailable = false;
	m_isPaused = false;
}

/*
====================
StopMP3

====================
*/
void CMP3::StopMP3( const char *pstrFilename )
{
	if(!m_isPlaying)
		return;

	if(pstrFilename && strlen(pstrFilename) > 0 
		&& strcmp(pstrFilename, m_szFilename))
		return;

	m_FSOUND_SetPaused(MUSIC_CHANNEL_NUM, TRUE);
	m_FSOUND_Stream_Close( m_pStream );

	m_isPlaying = false;
	m_playbackStarted = false;

	memset(m_szFilename, 0, sizeof(m_szFilename));
}

/*
====================
PlayMP3

====================
*/
void CMP3::PlayMP3( const char *pstrFilename, int timeoffset, bool islooped )
{
	if(!pstrFilename || !strlen(pstrFilename))
	{
		if(m_isPlaying)
			m_FSOUND_Stream_Close( m_pStream );

		return;
	}

	if( m_isPlaying )
	{	
		// sound system is already initialized
		m_FSOUND_Stream_Close( m_pStream );
	} 
	else
	{
		// Initialize the output
		m_FSOUND_SetOutput( FSOUND_OUTPUT_DSOUND );
		m_FSOUND_SetBufferSize( 200 );
		m_FSOUND_SetDriver( 0 );
		m_FSOUND_Init( 44100, 1, 0 );	
	}

	char szFilePath[MAX_PATH];
	sprintf( szFilePath, "%s/sound/fmod/%s", gEngfuncs.pfnGetGameDirectory(), pstrFilename );

	// Set flags
	unsigned int flags = (FSOUND_NORMAL|FSOUND_MPEGACCURATE);
	if(islooped)
		flags |= FSOUND_LOOP_NORMAL;

	m_pStream = m_FSOUND_Stream_Open( szFilePath, flags, 0, 0 );
	if(!m_pStream)
	{
		gEngfuncs.Con_Printf("Error: Could not load mp3 file '%s'\n", szFilePath);
		m_isPlaying = false;
		m_playbackStarted = false;
		return;
	}

	// Check length
	int offsetms = 0;
	if(timeoffset > 0)
	{
		int lengthms = m_FSOUND_Stream_GetLengthMs(m_pStream);
		if(!islooped)
		{
			if(lengthms < timeoffset)
			{
				// Tell server to clear this track on player
				char szcmd[MAX_PATH];
				sprintf(szcmd, "clearmusic %s\n", pstrFilename);
				gEngfuncs.pfnServerCmd(szcmd);
				
				// Close the stream and exit
				m_FSOUND_Stream_Close( m_pStream );
				m_isPlaying = false;
				m_playbackStarted = false;
				return;
			}
			else
			{
				// just straight set it
				offsetms = timeoffset;
			}
		}
		else
		{
			// if looping, limit in bounds of length
			offsetms = timeoffset % lengthms;
		}

		m_FSOUND_Stream_SetTime(m_pStream, offsetms);
		gEngfuncs.Con_DPrintf("Playing '%s' with a time offset of %.2f seconds.\n", pstrFilename, (offsetms/1000.0f));
	}

	// Set callback fn
	m_FSOUND_Stream_SetEndCallback(m_pStream, MP3_OnFinishedCallback, 0);

	// Play this audio track
	if(!m_isPaused && !m_wasReset)
	{
		m_FSOUND_Stream_Play( MUSIC_CHANNEL_NUM, m_pStream );
		m_playbackStarted = true;
	}
	else
	{
		// Start after unpausing
		m_FSOUND_SetPaused(MUSIC_CHANNEL_NUM, TRUE);
		m_playbackStarted = false;
		m_isPaused = true;
	}

	// We are currently playing a track
	m_isPlaying = true;

	// Remember filename
	strcpy(m_szFilename, pstrFilename);
}

/*
====================
OnPlaybackFinished

====================
*/
void CMP3::OnPlaybackFinished( FSOUND_STREAM *stream )
{
	if(!m_pStream || !m_isPlaying)
		return;

	if(stream != m_pStream)
	{
		gEngfuncs.Con_Printf("%s - Called with wrong stream pointer.\n", __FUNCTION__);
		return;
	}

	// Clean up locally
	m_FSOUND_Stream_Close( m_pStream );
	m_isPlaying = false;

	// Tell server
	char szcmd[MAX_PATH];
	sprintf(szcmd, "clearmusic %s\n", m_szFilename);
	gEngfuncs.pfnServerCmd(szcmd);

	memset(m_szFilename, 0, sizeof(m_szFilename));
}

/*
====================
CalcRefDef

====================
*/
void CMP3::CalcRefDef( const ref_params_t* pparams )
{
	static float lasttime = 0;
	float curtime = gEngfuncs.GetClientTime();
	float frametime = (lasttime > 0) ? (curtime - lasttime) : 1.0;
	lasttime = curtime;

	if(frametime < 0)
		frametime = 0;

	// pparams->paused is NOT reliable for console pause on WON
	bool shouldPause = (frametime == 0) ? true : false;
	if(shouldPause && !m_isPaused)
	{
		if(m_pStream && m_isPlaying && m_playbackStarted)
			m_FSOUND_SetPaused(MUSIC_CHANNEL_NUM, TRUE);

		m_isPaused = true;
	}
	else if(!shouldPause && m_isPaused)
	{
		if(m_pStream && m_isPlaying)
		{
			if(m_playbackStarted)
			{
				m_FSOUND_SetPaused(MUSIC_CHANNEL_NUM, FALSE);
			}
			else
			{
				m_FSOUND_SetPaused(MUSIC_CHANNEL_NUM, FALSE);
				m_FSOUND_Stream_Play( MUSIC_CHANNEL_NUM, m_pStream );
				m_playbackStarted = true;
			}
		}

		m_isPaused = false;
	}

	if(m_wasReset && !shouldPause)
		m_wasReset = false;
}

/*
====================
W_HudFrame

====================
*/
void CMP3::W_HudFrame( void )
{
	if(!m_wasReset && m_gotFrame)
	{
		if(!m_gotRender)
		{
			gEngfuncs.Con_DPrintf("Resetting from W_HUDRame\n");
			StopMP3(NULL);

			// Mark that we got reset
			m_wasReset = true;
			return;
		}

		m_gotRender = false;
	}
}

/*
====================
W_CreateEntities

====================
*/
void CMP3::W_CreateEntities( void )
{
	m_gotEntities = true;
	m_gotRender = true;
}

/*
====================
W_DrawNormalTriangles

====================
*/
void CMP3::W_DrawNormalTriangles( void )
{
	m_gotFrame = true;
	m_gotRender = true;

	// This can only happen during a levelchange
	if(!m_wasReset && !m_gotEntities)
	{
		gEngfuncs.Con_DPrintf("Resetting from W_DNormal\n");
		StopMP3(NULL);

		// Mark that we got reset
		m_wasReset = true;
		return;
	}

	m_gotEntities = false;
}