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
//
// hud.cpp
//
// implementation of CHud class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "hud_servers.h"
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"

#include "demo.h"
#include "demo_api.h"
#include "vgui_scorepanel.h"
#include "fog.h"
#include "r_efx.h"
#include "event_api.h"
#include "pmtrace.h"
#include "pm_defs.h"
#include "elightlist.h"
#include "svd_render.h"
#include "svdformat.h"
#include "svd_render.h"

class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	virtual void GetPlayerTextColor(int entindex, int color[3])
	{
		color[0] = color[1] = color[2] = 255;

		if( entindex >= 0 && entindex < sizeof(g_PlayerExtraInfo)/sizeof(g_PlayerExtraInfo[0]) )
		{
			int iTeam = g_PlayerExtraInfo[entindex].teamnumber;

			if ( iTeam < 0 )
			{
				iTeam = 0;
			}

			iTeam = iTeam % iNumberOfTeamColors;

			color[0] = iTeamColors[iTeam][0];
			color[1] = iTeamColors[iTeam][1];
			color[2] = iTeamColors[iTeam][2];
		}
	}

	virtual void UpdateCursorState()
	{
		gViewPort->UpdateCursorState();
	}

	virtual int	GetAckIconHeight()
	{
		return ScreenHeight - gHUD.m_iFontHeight*3 - 6;
	}

	virtual bool			CanShowSpeakerLabels()
	{
		if( gViewPort && gViewPort->m_pScoreBoard )
			return !gViewPort->m_pScoreBoard->isVisible();
		else
			return false;
	}
};
static CHLVoiceStatusHelper g_VoiceStatusHelper;


extern client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);

extern cvar_t *sensitivity;
cvar_t *cl_lw = NULL;

// HLRETAIL
cvar_t *cl_rollangle;
cvar_t *cl_rollspeed;
// HLRETAIL

void ShutdownInput (void);

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_Logo(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_Logo(pszName, iSize, pbuf );
}

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_ResetHUD(pszName, iSize, pbuf );
}

int __MsgFunc_InitHUD(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_InitHUD( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_ViewMode(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_ViewMode( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_SetFOV(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_SetFOV( pszName, iSize, pbuf );
}

int __MsgFunc_Concuss(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_Concuss( pszName, iSize, pbuf );
}

int __MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_GameMode( pszName, iSize, pbuf );
}

// TFFree Command Menu
void __CmdFunc_OpenCommandMenu(void)
{
	if ( gViewPort )
	{
		gViewPort->ShowCommandMenu( gViewPort->m_StandardMenu );
	}
}

// TFC "special" command
void __CmdFunc_InputPlayerSpecial(void)
{
	if ( gViewPort )
	{
		gViewPort->InputPlayerSpecial();
	}
}

void __CmdFunc_CloseCommandMenu(void)
{
	if ( gViewPort )
	{
		gViewPort->InputSignalHideCommandMenu();
	}
}

void __CmdFunc_ForceCloseCommandMenu( void )
{
	if ( gViewPort )
	{
		gViewPort->HideCommandMenu();
	}
}

void __CmdFunc_ToggleServerBrowser( void )
{
	if ( gViewPort )
	{
		gViewPort->ToggleServerBrowser();
	}
}

// TFFree Command Menu Message Handlers
int __MsgFunc_ValClass(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ValClass( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_TeamNames(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamNames( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_Feign(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_Feign( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_Detpack(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_Detpack( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_VGUIMenu(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_VGUIMenu( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_MOTD(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_MOTD( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_BuildSt(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_BuildSt( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_RandomPC(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_RandomPC( pszName, iSize, pbuf );
	return 0;
}
 
int __MsgFunc_ServerName(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ServerName( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_ScoreInfo(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ScoreInfo( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_TeamScore(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamScore( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_TeamInfo(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamInfo( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_Spectator(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_Spectator( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_AllowSpec(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_AllowSpec( pszName, iSize, pbuf );
	return 0;
}
 
Vector TraceForward( void )
{
	Vector v_angles = gHUD.m_vecAngles;
	Vector v_origin = gHUD.m_vecOrigin;

	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);

	Vector forward;
	gEngfuncs.pfnAngleVectors(v_angles, forward, NULL, NULL);
	Vector endpos = v_origin + forward*8192;

	gEngfuncs.pEventAPI->EV_PlayerTrace(v_origin, endpos, PM_WORLD_ONLY, -1, &tr);
	if(tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
		return endpos;

	Vector offset = tr.endpos + tr.plane.normal * 16;
	return offset;
}

cl_entity_t* TraceForwardEntity( void )
{
	Vector v_angles = gHUD.m_vecAngles;
	Vector v_origin = gHUD.m_vecOrigin;

	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);

	Vector forward;
	gEngfuncs.pfnAngleVectors(v_angles, forward, NULL, NULL);
	Vector endpos = v_origin + forward*8192;

	gEngfuncs.pEventAPI->EV_PlayerTrace(v_origin, endpos, PM_NORMAL, -1, &tr);
	if(tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
		return NULL;

	int entindex = gEngfuncs.pEventAPI->EV_IndexFromTrace(&tr);
	return gEngfuncs.GetEntityByIndex(entindex);
}

void __CmdFunc_BlobExplosion( void )
{
	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_BlobExplosion(origin);
}

void __CmdFunc_BreakModel( void )
{
	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("models/shell.mdl", &modelindex))
		return;

	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_BreakModel(origin, Vector(64, 64, 64), Vector(0, 0, 1), 1, 32, 16, modelindex, NULL);
}

void __CmdFunc_Bubbles( void )
{
	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("sprites/laserdot.spr", &modelindex))
		return;

	Vector origin = TraceForward();
	Vector mins = origin - Vector(64, 64, 64);
	Vector maxs = origin + Vector(64, 64, 64);

	gEngfuncs.pEfxAPI->R_Bubbles(mins, maxs, 128, modelindex, 16, 8);
}

void __CmdFunc_BubbleTrail( void )
{
	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("sprites/laserdot.spr", &modelindex))
		return;

	Vector origin = TraceForward();
	Vector end = origin + Vector(64, 64, 64);

	gEngfuncs.pEfxAPI->R_BubbleTrail(origin, end, 128, modelindex, 16, 8);
}

void __CmdFunc_BulletImpact( void )
{
	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_BulletImpactParticles(origin);
}

void __CmdFunc_EntityParticles( void )
{
	cl_entity_t* pentity = TraceForwardEntity();
	if(!pentity)
		return;

	gEngfuncs.pEfxAPI->R_EntityParticles(pentity);
}

void __CmdFunc_Explosion( void )
{
	Vector origin = TraceForward();

	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("sprites/laserdot.spr", &modelindex))
		return;

	gEngfuncs.pEfxAPI->R_Explosion(origin, modelindex, 2, 1.0, 0);
}

void __CmdFunc_FizzEffect( void )
{
	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("sprites/laserdot.spr", &modelindex))
		return;

	cl_entity_t* pentity = TraceForwardEntity();
	if(!pentity)
		return;

	gEngfuncs.pEfxAPI->R_FizzEffect(pentity, modelindex, 15);
}

void __CmdFunc_FireField( void )
{
	Vector origin = TraceForward();

	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("sprites/laserdot.spr", &modelindex))
		return;

	gEngfuncs.pEfxAPI->R_FireField(origin, 64, modelindex, 16, FTENT_SPIRAL|TEFIRE_FLAG_ALLFLOAT|TEFIRE_FLAG_ALPHA, 16);
}

void __CmdFunc_FlickerParticles( void )
{
	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_FlickerParticles(origin);
}

void __CmdFunc_FunnelSprite( void )
{
	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("sprites/laserdot.spr", &modelindex))
		return;

	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_FunnelSprite(origin, modelindex, gEngfuncs.pfnRandomLong(0, 1));
}

void __CmdFunc_Implosion( void )
{
	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_Implosion(origin, 1024, 768, 16);
}

void __CmdFunc_LargeFunnel( void )
{
	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_LargeFunnel(origin, gEngfuncs.pfnRandomLong(0, 1));
}

void __CmdFunc_LavaSplash( void )
{
	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_LavaSplash(origin);
}

void __CmdFunc_ParticleBox( void )
{
	Vector origin = TraceForward();
	Vector mins = origin - Vector(64, 64, 64);
	Vector maxs = origin + Vector(64, 64, 64);

	gEngfuncs.pEfxAPI->R_ParticleBox(mins, maxs, gEngfuncs.pfnRandomLong(0, 255),
		gEngfuncs.pfnRandomLong(0, 255),
		gEngfuncs.pfnRandomLong(0, 255), 60);
}

void __CmdFunc_ParticleBurst( void )
{
	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_ParticleBurst(origin, 64, gEngfuncs.pfnRandomLong(0, 128), 60);
}

void __CmdFunc_ParticleExplosion( void )
{
	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_ParticleExplosion(origin);
}

void __CmdFunc_ParticleExplosion2( void )
{
	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_ParticleExplosion2(origin, 0, 255);
}

void __CmdFunc_ParticleLine( void )
{
	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_ParticleLine(origin, origin+Vector(0, 0, 128),
		gEngfuncs.pfnRandomLong(0, 255),
		gEngfuncs.pfnRandomLong(0, 255),
		gEngfuncs.pfnRandomLong(0, 255), 30);
}

void __CmdFunc_PlayerSprites( void )
{
	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("sprites/laserdot.spr", &modelindex))
		return;

	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_PlayerSprites(1, modelindex, 16, 1);
}

void __CmdFunc_Spray( void )
{
	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("models/shell.mdl", &modelindex))
		return;

	Vector origin = TraceForward();
	Vector dir = Vector(gEngfuncs.pfnRandomFloat(-1, 1), gEngfuncs.pfnRandomFloat(-1, 1), gEngfuncs.pfnRandomFloat(-1, 1));
	gEngfuncs.pEfxAPI->R_Spray(origin, dir, modelindex, gEngfuncs.pfnRandomLong(16, 64), gEngfuncs.pfnRandomLong(16, 64), 30, kRenderNormal);
}

void __CmdFunc_Sprite_Spray( void )
{
	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("sprites/laserdot.spr", &modelindex))
		return;

	Vector origin = TraceForward();
	Vector dir = Vector(gEngfuncs.pfnRandomFloat(-1, 1), gEngfuncs.pfnRandomFloat(-1, 1), gEngfuncs.pfnRandomFloat(-1, 1));

	gEngfuncs.pEfxAPI->R_Sprite_Spray(origin, dir, modelindex, 16, 11, 1);
}

void __CmdFunc_Sprite_Trail( void )
{
	Vector origin = TraceForward();
	Vector end = origin + Vector(64, 64, 64);

	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("sprites/laserdot.spr", &modelindex))
		return;

	gEngfuncs.pEfxAPI->R_Sprite_Trail(TE_SPRITETRAIL, origin, end, modelindex, 16, 30, 4, 0.1, 255, 30);
}

void __CmdFunc_StreakSplash( void )
{
	Vector origin = TraceForward();
	Vector dir = Vector(gEngfuncs.pfnRandomFloat(-1, 1), gEngfuncs.pfnRandomFloat(-1, 1), gEngfuncs.pfnRandomFloat(-1, 1));

	gEngfuncs.pEfxAPI->R_StreakSplash(origin, dir, 64, 60, 10, 10, 40);
}

void __CmdFunc_TracerEffect( void )
{
	Vector origin = TraceForward();
	Vector end = origin + Vector(64, 64, 64);
	gEngfuncs.pEfxAPI->R_TracerEffect(origin, end);
}

void __CmdFunc_TracerParticles( void )
{
	Vector origin = TraceForward();
	Vector vel = Vector(gEngfuncs.pfnRandomFloat(-1, 1), gEngfuncs.pfnRandomFloat(-1, 1), gEngfuncs.pfnRandomFloat(-1, 1));

	gEngfuncs.pEfxAPI->R_TracerParticles(origin, vel, 15);
}

void __CmdFunc_TeleportSplash( void )
{
	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_TeleportSplash(origin);
}

void __CmdFunc_TempSphereModel( void )
{
	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("models/shell.mdl", &modelindex))
		return;

	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_TempSphereModel(origin, 30, 30, 16, modelindex);
}

void __CmdFunc_TempModel( void )
{
	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("models/shell.mdl", &modelindex))
		return;

	Vector dir = Vector(gEngfuncs.pfnRandomFloat(-1, 1), gEngfuncs.pfnRandomFloat(-1, 1), gEngfuncs.pfnRandomFloat(-1, 1));

	Vector origin = TraceForward();
	gEngfuncs.pEfxAPI->R_TempModel(origin, dir, Vector(0, 0, 180), 15, modelindex, 0);
}

void __CmdFunc_TempSprite( void )
{
	Vector origin = TraceForward();
	Vector dir = Vector(gEngfuncs.pfnRandomFloat(-1, 1), gEngfuncs.pfnRandomFloat(-1, 1), gEngfuncs.pfnRandomFloat(-1, 1));

	int modelindex = 0;
	if(!gEngfuncs.CL_LoadModel("sprites/laserdot.spr", &modelindex))
		return;

	gEngfuncs.pEfxAPI->R_TempSprite(origin, dir, 1.0, modelindex, kRenderNormal, kRenderFxPulseSlowWide, 255, 60, FTENT_SINEWAVE);
}

// This is called every time the DLL is loaded
void CHud :: Init( void )
{


	HOOK_MESSAGE( Logo );
	HOOK_MESSAGE( ResetHUD );
	HOOK_MESSAGE( GameMode );
	HOOK_MESSAGE( InitHUD );
	HOOK_MESSAGE( ViewMode );
	HOOK_MESSAGE( SetFOV );
	HOOK_MESSAGE( Concuss );

	// TFFree CommandMenu
	HOOK_COMMAND( "+commandmenu", OpenCommandMenu );
	HOOK_COMMAND( "-commandmenu", CloseCommandMenu );
	HOOK_COMMAND( "ForceCloseCommandMenu", ForceCloseCommandMenu );
	HOOK_COMMAND( "special", InputPlayerSpecial );
	HOOK_COMMAND( "togglebrowser", ToggleServerBrowser );

	HOOK_COMMAND( "e_blobexplosion", BlobExplosion );
	HOOK_COMMAND( "e_breakmodel", BreakModel );
	HOOK_COMMAND( "e_bubbles", Bubbles );
	HOOK_COMMAND( "e_bubbletrail", BubbleTrail );
	HOOK_COMMAND( "e_bulletimpact", BulletImpact );
	HOOK_COMMAND( "e_entityparticles", EntityParticles );
	HOOK_COMMAND( "e_explosion", Explosion );
	HOOK_COMMAND( "e_fizzeffect", FizzEffect );
	HOOK_COMMAND( "e_firefield", FireField );
	HOOK_COMMAND( "e_flickerparticles", FlickerParticles );
	HOOK_COMMAND( "e_funnelsprite", FunnelSprite );
	HOOK_COMMAND( "e_implosion", Implosion );
	HOOK_COMMAND( "e_largefunnel", LargeFunnel );
	HOOK_COMMAND( "e_lavasplash", LavaSplash );
	HOOK_COMMAND( "e_particlebox", ParticleBox );
	HOOK_COMMAND( "e_particleburst", ParticleBurst );
	HOOK_COMMAND( "e_particleexplosion", ParticleExplosion );
	HOOK_COMMAND( "e_particleexplosion2", ParticleExplosion2 );
	HOOK_COMMAND( "e_particleline", ParticleLine );
	HOOK_COMMAND( "e_playersprites", PlayerSprites );
	HOOK_COMMAND( "e_spray", Spray );
	HOOK_COMMAND( "e_spritespray", Sprite_Spray );
	HOOK_COMMAND( "e_spritetrail", Sprite_Trail );
	HOOK_COMMAND( "e_streaksplash", StreakSplash );
	HOOK_COMMAND( "e_tracereffect", TracerEffect );
	HOOK_COMMAND( "e_tracerparticles", TracerParticles );
	HOOK_COMMAND( "e_teleportsplash", TeleportSplash );
	HOOK_COMMAND( "e_tempspheremodel", TempSphereModel );
	HOOK_COMMAND( "e_tempmodel", TempModel );
	HOOK_COMMAND( "e_tempsprite", TempSprite );

	HOOK_MESSAGE( ValClass );
	HOOK_MESSAGE( TeamNames );
	HOOK_MESSAGE( Feign );
	HOOK_MESSAGE( Detpack );
	HOOK_MESSAGE( MOTD );
	HOOK_MESSAGE( BuildSt );
	HOOK_MESSAGE( RandomPC );
	HOOK_MESSAGE( ServerName );
	HOOK_MESSAGE( ScoreInfo );
	HOOK_MESSAGE( TeamScore );
	HOOK_MESSAGE( TeamInfo );

	HOOK_MESSAGE( Spectator );
	HOOK_MESSAGE( AllowSpec );

	// VGUI Menus
	HOOK_MESSAGE( VGUIMenu );

	CVAR_CREATE( "hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );		// controls whether or not to suicide immediately on TF class switch
	CVAR_CREATE( "hud_takesshots", "0", FCVAR_ARCHIVE );		// controls whether or not to automatically take screenshots at the end of a round


	m_iLogo = 0;
	m_iFOV = 0;

	CVAR_CREATE( "zoom_sensitivity_ratio", "1.2", 0 );
	default_fov = CVAR_CREATE( "default_fov", "90", 0 );
	m_pCvarStealMouse = CVAR_CREATE( "hud_capturemouse", "1", FCVAR_ARCHIVE );
	m_pCvarDraw = CVAR_CREATE( "hud_draw", "1", FCVAR_ARCHIVE );
	cl_lw = gEngfuncs.pfnGetCvarPointer( "cl_lw" );

	m_pSpriteList = NULL;

	// Clear any old HUD list
	if ( m_pHudList )
	{
		HUDLIST *pList;
		while ( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}

	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;

	m_Ammo.Init();
	m_Health.Init();
	m_SayText.Init();
	m_Spectator.Init();
	m_Geiger.Init();
	m_Train.Init();
	m_Battery.Init();
	m_Flash.Init();
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_AmmoSecondary.Init();
	m_TextMessage.Init();
	m_StatusIcons.Init();
	GetClientVoiceMgr()->Init(&g_VoiceStatusHelper, (vgui::Panel**)&gViewPort);
	gELightList.Init();

	m_Menu.Init();
	
	ServersInit();

	MsgFunc_ResetHUD(0, 0, NULL );

	// HLRETAIL - view roll code - based on Mazor's tutorial
	cl_rollangle = gEngfuncs.pfnRegisterVariable("cl_rollangle", "0.65", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
	cl_rollspeed = gEngfuncs.pfnRegisterVariable("cl_rollspeed", "300", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
	// HLRETAIL

	gFog.Init();

	SVD_Init();
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud :: ~CHud()
{
	delete [] m_rgSpriteHandle_ts;
	delete [] m_rgrcRects;
	delete [] m_rgszSpriteNames;

	if ( m_pHudList )
	{
		HUDLIST *pList;
		while ( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}

	ServersShutdown();
	SVD_Clear();
	SVD_Shutdown();
}

// GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// returns an index into the gHUD.m_rgSpriteHandle_ts[] array
// returns 0 if sprite not found
int CHud :: GetSpriteIndex( const char *SpriteName )
{
	// look through the loaded sprite name list for SpriteName
	for ( int i = 0; i < m_iSpriteCount; i++ )
	{
		if ( strncmp( SpriteName, m_rgszSpriteNames + (i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH ) == 0 )
			return i;
	}

	return -1; // invalid sprite
}

void CHud :: VidInit( void )
{
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	// ----------
	// Load Sprites
	// ---------
//	m_hsprFont = LoadSprite("sprites/%d_font.spr");
	
	m_hsprLogo = 0;	
	m_hsprCursor = 0;

	if (ScreenWidth < 640)
		m_iRes = 320;
	else
		m_iRes = 640;

	// Only load this once
	if ( !m_pSpriteList )
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList("sprites/hud.txt", &m_iSpriteCountAllRes);

		if (m_pSpriteList)
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t *p = m_pSpriteList;
			int j;
			for ( j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if ( p->iRes == m_iRes )
					m_iSpriteCount++;
				p++;
			}

			// allocated memory for sprite handle arrays
 			m_rgSpriteHandle_ts = new SpriteHandle_t[m_iSpriteCount];
			m_rgrcRects = new wrect_t[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

			p = m_pSpriteList;
			int index = 0;
			for ( j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if ( p->iRes == m_iRes )
				{
					char sz[256];
					sprintf(sz, "sprites/%s.spr", p->szSprite);
					m_rgSpriteHandle_ts[index] = SPR_Load(sz);
					m_rgrcRects[index] = p->rc;
					strncpy( &m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH );

					index++;
				}

				p++;
			}
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t *p = m_pSpriteList;
		int index = 0;
		for ( int j = 0; j < m_iSpriteCountAllRes; j++ )
		{
			if ( p->iRes == m_iRes )
			{
				char sz[256];
				sprintf( sz, "sprites/%s.spr", p->szSprite );
				m_rgSpriteHandle_ts[index] = SPR_Load(sz);
				index++;
			}

			p++;
		}
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex( "number_0" );

	m_iFontHeight = m_rgrcRects[m_HUD_number_0].bottom - m_rgrcRects[m_HUD_number_0].top;

	m_Ammo.VidInit();
	m_Health.VidInit();
	m_Spectator.VidInit();
	m_Geiger.VidInit();
	m_Train.VidInit();
	m_Battery.VidInit();
	m_Flash.VidInit();
	m_Message.VidInit();
	m_StatusBar.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_Menu.VidInit();
	m_AmmoSecondary.VidInit();
	m_TextMessage.VidInit();
	m_StatusIcons.VidInit();
	GetClientVoiceMgr()->VidInit();
	SVD_VidInit();
	gELightList.VidInit();

	gFog.VidInit();

	SVD_CreateStencilFBO();
}

int CHud::MsgFunc_Logo(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	// update Train data
	m_iLogo = READ_BYTE();

	return 1;
}

float g_lastFOV = 0.0;

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase ( const char *in, char *out)
{
	int len, start, end;

	len = strlen( in );
	
	// scan backward for '.'
	end = len - 1;
	while ( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;
	
	if ( in[end] != '.' )		// no '.', copy to end
		end = len-1;
	else 
		end--;					// Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len-1;
	while ( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if ( in[start] != '/' && in[start] != '\\' )
		start = 0;
	else 
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy( out, &in[start], len );
	// Terminate it
	out[len] = 0;
}

/*
=================
HUD_IsGame

=================
*/
int HUD_IsGame( const char *game )
{
	const char *gamedir;
	char gd[ 1024 ];

	gamedir = gEngfuncs.pfnGetGameDirectory();
	if ( gamedir && gamedir[0] )
	{
		COM_FileBase( gamedir, gd );
		if ( !stricmp( gd, game ) )
			return 1;
	}
	return 0;
}

/*
=====================
HUD_GetFOV

Returns last FOV
=====================
*/
float HUD_GetFOV( void )
{
	if ( gEngfuncs.pDemoAPI->IsRecording() )
	{
		// Write it
		int i = 0;
		unsigned char buf[ 100 ];

		// Active
		*( float * )&buf[ i ] = g_lastFOV;
		i += sizeof( float );

		Demo_WriteBuffer( TYPE_ZOOM, i, buf );
	}

	if ( gEngfuncs.pDemoAPI->IsPlayingback() )
	{
		g_lastFOV = g_demozoom;
	}
	return g_lastFOV;
}

int CHud::MsgFunc_SetFOV(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int newfov = READ_BYTE();
	int def_fov = CVAR_GET_FLOAT( "default_fov" );

	//Weapon prediction already takes care of changing the fog. ( g_lastFOV ).
	if ( cl_lw && cl_lw->value )
		return 1;

	g_lastFOV = newfov;

	if ( newfov == 0 )
	{
		m_iFOV = def_fov;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if ( m_iFOV == def_fov )
	{  
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{  
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)def_fov) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}

	return 1;
}


void CHud::AddHudElem(CHudBase *phudelem)
{
	HUDLIST *pdl, *ptemp;

//phudelem->Think();

	if (!phudelem)
		return;

	pdl = (HUDLIST *)malloc(sizeof(HUDLIST));
	if (!pdl)
		return;

	memset(pdl, 0, sizeof(HUDLIST));
	pdl->p = phudelem;

	if (!m_pHudList)
	{
		m_pHudList = pdl;
		return;
	}

	ptemp = m_pHudList;

	while (ptemp->pNext)
		ptemp = ptemp->pNext;

	ptemp->pNext = pdl;
}

float CHud::GetSensitivity( void )
{
	return m_flMouseSensitivity;
}


