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

// MP3 support added by Killar
// Extended by Overfloater

#ifndef MP3_H
#define MP3_H

#include "fmod.h"
#include "fmod_errors.h"
#include "windows.h"
#include "ref_params.h"

class CMP3
{
public:
	// Default music channel
	static const int MUSIC_CHANNEL_NUM;

public:
	bool Init( void );
	void Shutdown( void );
	void VidInit( void );
	void PlayMP3( const char *pstrFilename, int timeoffset, bool islooped );
	void StopMP3( const char *pstrFilename );
	void OnPlaybackFinished( FSOUND_STREAM *stream );
	void CalcRefDef( const ref_params_t* pparams );

	void W_HudFrame( void );
	void W_CreateEntities( void );
	void W_DrawNormalTriangles( void );

private:
	float			(_stdcall * m_FSOUND_GetVersion)			( void );//AJH get fmod dll version
	signed char		(_stdcall * m_FSOUND_Stream_Close)			( FSOUND_STREAM *stream );
	signed char		(_stdcall * m_FSOUND_SetOutput)				( int outputtype );
	signed char		(_stdcall * m_FSOUND_SetBufferSize)			( int len_ms );
	signed char		(_stdcall * m_FSOUND_SetDriver)				( int driver );
	signed char		(_stdcall * m_FSOUND_Init)					( int mixrate, int maxsoftwarechannels, unsigned int flags );
	FSOUND_STREAM*	(_stdcall * m_FSOUND_Stream_Open)			( const char *filename, unsigned int mode, int offset, int memlength );	//AJH use new fmod
	int 			(_stdcall * m_FSOUND_Stream_Play)			( int channel, FSOUND_STREAM *stream );
	signed char     (_stdcall * m_FSOUND_Stream_SetTime)		( FSOUND_STREAM *stream, int ms );
	int             (_stdcall * m_FSOUND_Stream_GetLengthMs)	( FSOUND_STREAM *stream );
	signed char		(_stdcall * m_FSOUND_Stream_SetEndCallback)	( FSOUND_STREAM *stream, FSOUND_STREAMCALLBACK callback, int userdata );
	signed char     (_stdcall * m_FSOUND_SetPaused)				( int channel, signed char paused );
	void			(_stdcall * m_FSOUND_Close)					( void );

private:
	FSOUND_STREAM	*m_pStream;
	bool			m_isPlaying;
	bool			m_playbackStarted;
	HINSTANCE		m_hFMod;
	bool			m_isAvailable;
	bool			m_isPaused;
	char			m_szFilename[MAX_PATH];

	bool			m_wasReset;
	bool			m_gotFrame;
	bool			m_gotRender;
	bool			m_gotEntities;
};

extern CMP3 gMP3;
#endif
