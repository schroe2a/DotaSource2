//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hl2mp_gamerules.h"
#include "viewport_panel_names.h"
#include "gameeventdefs.h"
#include <KeyValues.h>
#include "ammodef.h"
#include "HeroDef.h"
#include "ItemDef.h"
#include "hl2_shareddefs.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
#else
	#include "skills.h"
	#include "eventqueue.h"
	#include "player.h"
	#include "gamerules.h"
	#include "game.h"
	#include "items.h"
	#include "entitylist.h"
	#include "mapentities.h"
	#include "in_buttons.h"
	#include <ctype.h>
	#include "voice_gamemgr.h"
	#include "iscorer.h"
	#include "hl2mp_player.h"
	#include "weapon_hl2mpbasehlmpcombatweapon.h"
	#include "team.h"
	#include "voice_gamemgr.h"
	#include "hl2mp_gameinterface.h"
	#include "hl2mp_cvars.h"
	#include "ItemMoney.h"

#ifdef DEBUG	
	#include "hl2mp_bot_temp.h"
#endif

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);

extern bool FindInList( const char **pStrings, const char *pToFind );

ConVar sv_hl2mp_weapon_respawn_time( "sv_hl2mp_weapon_respawn_time", "20", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_hl2mp_item_respawn_time( "sv_hl2mp_item_respawn_time", "30", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_report_client_settings("sv_report_client_settings", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY );

extern ConVar mp_chattime;

extern CBaseEntity	 *g_pLastCombineSpawn;
extern CBaseEntity	 *g_pLastRebelSpawn;

#define WEAPON_MAX_DISTANCE_FROM_SPAWN 64

#endif


REGISTER_GAMERULES_CLASS( CHL2MPRules );

BEGIN_NETWORK_TABLE_NOBASE( CHL2MPRules, DT_HL2MPRules )

	#ifdef CLIENT_DLL
		RecvPropBool( RECVINFO( m_bTeamPlayEnabled ) ),
	#else
		SendPropBool( SENDINFO( m_bTeamPlayEnabled ) ),
	#endif

END_NETWORK_TABLE()


LINK_ENTITY_TO_CLASS( hl2mp_gamerules, CHL2MPGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( HL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )

static HL2MPViewVectors g_HL2MPViewVectors(
	Vector( 0, 0, 64 ),       //VEC_VIEW (m_vView) 
							  
	Vector(-16, -16, 0 ),	  //VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  72 ),	  //VEC_HULL_MAX (m_vHullMax)
							  					
	Vector(-16, -16, 0 ),	  //VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  36 ),	  //VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 28 ),		  //VEC_DUCK_VIEW		(m_vDuckView)
							  					
	Vector(-10, -10, -10 ),	  //VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),	  //VEC_OBS_HULL_MAX	(m_vObsHullMax)
							  					
	Vector( 0, 0, 14 ),		  //VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

	Vector(-16, -16, 0 ),	  //VEC_CROUCH_TRACE_MIN (m_vCrouchTraceMin)
	Vector( 16,  16,  60 )	  //VEC_CROUCH_TRACE_MAX (m_vCrouchTraceMax)
);

static const char *s_PreserveEnts[] =
{
	"ai_network",
	"ai_hint",
	"hl2mp_gamerules",
	"team_manager",
	"player_manager",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_brush",
	"func_wall",
	"func_buyzone",
	"func_illusionary",
	"infodecal",
	"info_projecteddecal",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_deathmatch",
	"info_player_combine",
	"info_player_rebel",
	"info_map_parameters",
	"keyframe_rope",
	"move_rope",
	"info_ladder",
	"player",
	"point_viewcontrol",
	"scene_manager",
	"shadow_control",
	"sky_camera",
	"soundent",
	"trigger_soundscape",
	"viewmodel",
	"predicted_viewmodel",
	"worldspawn",
	"point_devshot_camera",
	"", // END Marker
};



#ifdef CLIENT_DLL
	void RecvProxy_HL2MPRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CHL2MPRules *pRules = HL2MPRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CHL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )
		RecvPropDataTable( "hl2mp_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_HL2MPRules ), RecvProxy_HL2MPRules )
	END_RECV_TABLE()
#else
	void* SendProxy_HL2MPRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CHL2MPRules *pRules = HL2MPRules();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE( CHL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )
		SendPropDataTable( "hl2mp_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_HL2MPRules ), SendProxy_HL2MPRules )
	END_SEND_TABLE()
#endif

#ifndef CLIENT_DLL

	class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
	{
	public:
		virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
		{
			return ( pListener->GetTeamNumber() == pTalker->GetTeamNumber() );
		}
	};
	CVoiceGameMgrHelper g_VoiceGameMgrHelper;
	IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

#endif

// NOTE: the indices here must match TEAM_TERRORIST, TEAM_CT, TEAM_SPECTATOR, etc.
char *sTeamNames[] =
{
	"Unassigned",
	"Spectator",
	"Combine",
	"Rebels",
};

CHL2MPRules::CHL2MPRules()
{
#ifndef CLIENT_DLL
	// Create the team managers
	for ( int i = 0; i < ARRAYSIZE( sTeamNames ); i++ )
	{
		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "team_manager" ));
		pTeam->Init( sTeamNames[i], i );

		g_Teams.AddToTail( pTeam );
	}
	InitDefaultAIRelationships();

	m_bTeamPlayEnabled = true; //teamplay.GetBool();
	m_flIntermissionEndTime = 0.0f;
	m_flGameStartTime = 0;

	m_hRespawnableItemsAndWeapons.RemoveAll();
	m_tmNextPeriodicThink = 0;
	m_flRestartGameTime = 0;
	m_bCompleteReset = false;
	m_bHeardAllPlayersReady = false;
	m_bAwaitingReadyRestart = false;
	m_bChangelevelDone = false;

	mp_readyrestart.SetValue( 1 );

#endif
}

const CViewVectors* CHL2MPRules::GetViewVectors()const
{
	return &g_HL2MPViewVectors;
}

const HL2MPViewVectors* CHL2MPRules::GetHL2MPViewVectors()const
{
	return &g_HL2MPViewVectors;
}
	
CHL2MPRules::~CHL2MPRules( void )
{
#ifndef CLIENT_DLL
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	g_Teams.Purge();
#endif
}

void CHL2MPRules::CreateStandardEntities( void )
{

#ifndef CLIENT_DLL
	// Create the entity that will send our data to the client.

	BaseClass::CreateStandardEntities();

	g_pLastCombineSpawn = NULL;
	g_pLastRebelSpawn = NULL;

#ifdef DBGFLAG_ASSERT
	CBaseEntity *pEnt = 
#endif
	CBaseEntity::Create( "hl2mp_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
#endif
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHL2MPRules::FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	if ( weaponstay.GetInt() > 0 )
	{
		// make sure it's only certain weapons
		if ( !(pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			return 0;		// weapon respawns almost instantly
		}
	}

	return sv_hl2mp_weapon_respawn_time.GetFloat();
#endif

	return 0;		// weapon respawns almost instantly
}

bool CHL2MPRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	
	CHL2MP_Player *player = dynamic_cast< CHL2MP_Player*>( pPlayer );
	if ( player && player->GetDeathTime() )
	{
		if ( player->GetTeamNumber() > 1 && player->GetHeroType() == -1 )
		{
			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();
			UTIL_ClientPrintFilter( user, HUD_PRINTCENTER, "You need to pick a hero to spawn." );
			return false;
		}

		int respawnDelay = player->GetLevel() * 4;

		if ( gpGlobals->curtime - player->GetDeathTime() < respawnDelay )
		{
			bool canSkip = false;
			for ( int i=1; i<=4; i++ )
			{
				if ( player->GetSkill(i)->CanSkipRespawnPenilty() )
					canSkip = true;
			}
	
			if ( canSkip )
			{
				for ( int i=1; i<=4; i++ )
				{
					player->GetSkill(i)->SkipRespawnPenilty();
				}
				return true;
			}
			else
			{
				CSingleUserRecipientFilter user( pPlayer );
				user.MakeReliable();
				char szText[200];
				Q_snprintf( szText, sizeof(szText), "You can respawn in %i seconds.", respawnDelay - (int)( gpGlobals->curtime - player->GetDeathTime() ) );
				UTIL_ClientPrintFilter( user, HUD_PRINTCENTER, szText );
				return false;
			}
		}
	}
#endif

	return true;
}

bool CHL2MPRules::IsIntermission( void )
{
#ifndef CLIENT_DLL
	return m_flIntermissionEndTime > gpGlobals->curtime;
#endif

	return false;
}

void CHL2MPRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	if ( IsIntermission() )
		return;
	BaseClass::PlayerKilled( pVictim, info );
	
	CHL2MP_Player *victim = dynamic_cast< CHL2MP_Player*>( pVictim );

	if ( victim ) 
	{
		bool canSkip = false;
		for ( int i=1; i<=4; i++ )
		{
			if ( victim->GetSkill(i) && victim->GetSkill(i)->CanSkipRespawnPenilty() )
			{
				canSkip = true;
				//don't actually use it here, it will get used when we actually respawn..
				//victim->GetSkill(i)->SkipRespawnPenilty();
			}
		}
		if ( !canSkip )
		{			
			int goldReward = victim->GetLevel() * 30;
			goldReward = min( goldReward, victim->GetMoney() );
			victim->AddMoney( -goldReward );

			//CBaseEntity *pInflictor = info.GetInflictor();
			CBaseEntity *pKiller = info.GetAttacker();
			
			int xpReward;
			if ( victim->GetLevel() < 5 )
				xpReward = ((victim->GetLevel() - 1) * 40) + 100;
			else
				xpReward = (victim->GetLevel() - 2) * 100;

			this->RewardXPinArea( xpReward * 2, victim->GetAbsOrigin(), pKiller->GetAbsOrigin(), victim->GetTeamNumber() );

			while ( pKiller->GetOwnerEntity() )
				pKiller = pKiller->GetOwnerEntity();

			CHL2MP_Player *pKillerPlayer = ToHL2MPPlayer( pKiller );
			if ( pKillerPlayer && PlayerRelationship( pKillerPlayer, pVictim ) == GR_NOTTEAMMATE )
			{
				DropMoney( pVictim->GetAbsOrigin(), goldReward, pKillerPlayer );
			}
		}
	}

#endif
}

void CHL2MPRules::RewardXPinArea( int xpReward, Vector location1, Vector location2, int victimsTeam )
{
#ifndef CLIENT_DLL

	CUtlVector<CHL2MP_Player*> pScoringPlayers;

	CHL2MP_Player *pScorer = NULL;
	CBaseEntity *pTarget = NULL;
	CBaseEntity *pTargetOwner;
	while ( ( pTarget = gEntList.FindEntityInSphere( pTarget, location1, 128 ) ) != NULL )
	{		
		pTargetOwner = pTarget;
		while ( pTargetOwner->GetOwnerEntity() )
			pTargetOwner = pTargetOwner->GetOwnerEntity();

		pScorer = ToHL2MPPlayer( pTargetOwner );
		if ( pScorer && pScorer->IsAlive() && pScorer->GetTeamNumber() != victimsTeam )
		{
			pScoringPlayers.AddToTail( pScorer );
		}
	}

	pTarget = NULL;
	while ( ( pTarget = gEntList.FindEntityInSphere( pTarget, location2, 128 ) ) != NULL )
	{
		pTargetOwner = pTarget;
		while ( pTargetOwner->GetOwnerEntity() )
			pTargetOwner = pTargetOwner->GetOwnerEntity();

		pScorer = ToHL2MPPlayer( pTargetOwner );
		if ( pScorer && pScorer->IsAlive() && pScorer->GetTeamNumber() != victimsTeam )
		{
			pScoringPlayers.AddToTail( pScorer );
		}
	}

	for( int i = 0 ; i < pScoringPlayers.Count() ; i++ )
	{
		pScoringPlayers[i]->AddXP( xpReward / pScoringPlayers.Count() );
	}

#endif
}

void CHL2MPRules::Think( void )
{

#ifndef CLIENT_DLL
	
	CGameRules::Think();

	if ( g_fGameOver )   // someone else quit the game already
	{
		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->curtime )
		{
			if ( !m_bChangelevelDone )
			{
				ChangeLevel(); // intermission is over
				m_bChangelevelDone = true;
			}
		}

		return;
	}

//	float flTimeLimit = mp_timelimit.GetFloat() * 60;
	float flFragLimit = fraglimit.GetFloat();
	
	if ( GetMapRemainingTime() < 0 )
	{
		GoToIntermission();
		return;
	}

	if ( flFragLimit )
	{
		if( IsTeamplay() == true )
		{
			CTeam *pCombine = g_Teams[TEAM_COMBINE];
			CTeam *pRebels = g_Teams[TEAM_REBELS];

			if ( pCombine->GetScore() >= flFragLimit || pRebels->GetScore() >= flFragLimit )
			{
				GoToIntermission();
				return;
			}
		}
		else
		{
			// check if any player is over the frag limit
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if ( pPlayer && pPlayer->FragCount() >= flFragLimit )
				{
					GoToIntermission();
					return;
				}
			}
		}
	}

	if ( gpGlobals->curtime > m_tmNextPeriodicThink )
	{		
		if ( m_bAwaitingReadyRestart )
			CheckAllPlayersReady();
		CheckRestartGame();
		m_tmNextPeriodicThink = gpGlobals->curtime + 1.0;
	}

	if ( m_flRestartGameTime > 0.0f && m_flRestartGameTime <= gpGlobals->curtime )
	{
		RestartGame();
	}

	if( m_bAwaitingReadyRestart && m_bHeardAllPlayersReady )
	{
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "All players ready. Game will restart in 5 seconds" );
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "All players ready. Game will restart in 5 seconds" );

		m_flRestartGameTime = gpGlobals->curtime + 5;
		m_bAwaitingReadyRestart = false;
	}

	ManageObjectRelocation();

#endif
}

void CHL2MPRules::GoToIntermission( void )
{
#ifndef CLIENT_DLL
	if ( g_fGameOver )
		return;

	g_fGameOver = true;

	m_flIntermissionEndTime = gpGlobals->curtime + mp_chattime.GetInt();

	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
		pPlayer->AddFlag( FL_FROZEN );
	}
#endif
	
}

bool CHL2MPRules::CheckGameOver()
{
#ifndef CLIENT_DLL
	if ( g_fGameOver )   // someone else quit the game already
	{
		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->curtime )
		{
			ChangeLevel(); // intermission is over			
		}

		return true;
	}
#endif

	return false;
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHL2MPRules::FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	if ( pWeapon && (pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
	{
		if ( gEntList.NumberOfEntities() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}
#endif
	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHL2MPRules::VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	CWeaponHL2MPBase *pHL2Weapon = dynamic_cast< CWeaponHL2MPBase*>( pWeapon );

	if ( pHL2Weapon )
	{
		return pHL2Weapon->GetOriginalSpawnOrigin();
	}
#endif
	
	return pWeapon->GetAbsOrigin();
}

#ifndef CLIENT_DLL

CItem* IsManagedObjectAnItem( CBaseEntity *pObject )
{
	return dynamic_cast< CItem*>( pObject );
}

CWeaponHL2MPBase* IsManagedObjectAWeapon( CBaseEntity *pObject )
{
	return dynamic_cast< CWeaponHL2MPBase*>( pObject );
}

bool GetObjectsOriginalParameters( CBaseEntity *pObject, Vector &vOriginalOrigin, QAngle &vOriginalAngles )
{
	if ( CItem *pItem = IsManagedObjectAnItem( pObject ) )
	{
		if ( pItem->m_flNextResetCheckTime > gpGlobals->curtime )
			 return false;
		
		vOriginalOrigin = pItem->GetOriginalSpawnOrigin();
		vOriginalAngles = pItem->GetOriginalSpawnAngles();

		pItem->m_flNextResetCheckTime = gpGlobals->curtime + sv_hl2mp_item_respawn_time.GetFloat();
		return true;
	}
	else if ( CWeaponHL2MPBase *pWeapon = IsManagedObjectAWeapon( pObject )) 
	{
		if ( pWeapon->m_flNextResetCheckTime > gpGlobals->curtime )
			 return false;

		vOriginalOrigin = pWeapon->GetOriginalSpawnOrigin();
		vOriginalAngles = pWeapon->GetOriginalSpawnAngles();

		pWeapon->m_flNextResetCheckTime = gpGlobals->curtime + sv_hl2mp_weapon_respawn_time.GetFloat();
		return true;
	}

	return false;
}

bool CHL2MPRules::GameStarted()
{
	if ( !IsIntermission() && !m_bAwaitingReadyRestart && !mp_readyrestart.GetBool() )
	{
		return true;	
	}
	return false;
}

void CHL2MPRules::ManageObjectRelocation( void )
{
	int iTotal = m_hRespawnableItemsAndWeapons.Count();

	if ( iTotal > 0 )
	{
		for ( int i = 0; i < iTotal; i++ )
		{
			CBaseEntity *pObject = m_hRespawnableItemsAndWeapons[i].Get();
			
			if ( pObject )
			{
				Vector vSpawOrigin;
				QAngle vSpawnAngles;

				if ( GetObjectsOriginalParameters( pObject, vSpawOrigin, vSpawnAngles ) == true )
				{
					float flDistanceFromSpawn = (pObject->GetAbsOrigin() - vSpawOrigin ).Length();

					if ( flDistanceFromSpawn > WEAPON_MAX_DISTANCE_FROM_SPAWN )
					{
						bool shouldReset = false;
						IPhysicsObject *pPhysics = pObject->VPhysicsGetObject();

						if ( pPhysics )
						{
							shouldReset = pPhysics->IsAsleep();
						}
						else
						{
							shouldReset = (pObject->GetFlags() & FL_ONGROUND) ? true : false;
						}

						if ( shouldReset )
						{
							pObject->Teleport( &vSpawOrigin, &vSpawnAngles, NULL );
							pObject->EmitSound( "AlyxEmp.Charge" );

							IPhysicsObject *pPhys = pObject->VPhysicsGetObject();

							if ( pPhys )
							{
								pPhys->Wake();
							}
						}
					}
				}
			}
		}
	}
}

//=========================================================
//AddLevelDesignerPlacedWeapon
//=========================================================
void CHL2MPRules::AddLevelDesignerPlacedObject( CBaseEntity *pEntity )
{
	if ( m_hRespawnableItemsAndWeapons.Find( pEntity ) == -1 )
	{
		m_hRespawnableItemsAndWeapons.AddToTail( pEntity );
	}
}

//=========================================================
//RemoveLevelDesignerPlacedWeapon
//=========================================================
void CHL2MPRules::RemoveLevelDesignerPlacedObject( CBaseEntity *pEntity )
{
	if ( m_hRespawnableItemsAndWeapons.Find( pEntity ) != -1 )
	{
		m_hRespawnableItemsAndWeapons.FindAndRemove( pEntity );
	}
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHL2MPRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

//=========================================================
// What angles should this item use to respawn?
//=========================================================
QAngle CHL2MPRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHL2MPRules::FlItemRespawnTime( CItem *pItem )
{
	return sv_hl2mp_item_respawn_time.GetFloat();
}


//=========================================================
// CanHaveWeapon - returns false if the player is not allowed
// to pick up this weapon
//=========================================================
bool CHL2MPRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem )
{
	if ( weaponstay.GetInt() > 0 )
	{
		if ( pPlayer->Weapon_OwnsThisType( pItem->GetClassname(), pItem->GetSubType() ) )
			 return false;
	}

	return BaseClass::CanHavePlayerItem( pPlayer, pItem );
}
float CHL2MPRules::FlHealthChargerRechargeTime( void )
{
	return 1;
}
float CHL2MPRules::FlHEVChargerRechargeTime( void )
{
	return 1;
}

bool CHL2MPRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{	
	return GameStarted();
}

int CHL2MPRules::ItemShouldRespawn( CItem *pItem )
{
	return GR_ITEM_RESPAWN_NO;
}
#endif

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHL2MPRules::WeaponShouldRespawn( CBaseCombatWeapon *pWeapon )
{
	return GR_WEAPON_RESPAWN_NO;
}

//-----------------------------------------------------------------------------
// Purpose: Player has just left the game
//-----------------------------------------------------------------------------
void CHL2MPRules::ClientDisconnected( edict_t *pClient )
{
#ifndef CLIENT_DLL
	// Msg( "CLIENT DISCONNECTED, REMOVING FROM TEAM.\n" );

	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );
	if ( pPlayer )
	{
		// Remove the player from his team
		if ( pPlayer->GetTeam() )
		{
			pPlayer->GetTeam()->RemovePlayer( pPlayer );
		}
	}

	BaseClass::ClientDisconnected( pClient );

#endif
}


//=========================================================
// Deathnotice. 
//=========================================================
void CHL2MPRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_ID = 0;

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor );

	// Custom kill type?
	if ( info.GetDamageCustom() )
	{
		killer_weapon_name = GetDamageCustomString( info );
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
		}
	}
	else
	{
		// Is the killer a client?
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
			
			if ( pInflictor )
			{
				if ( pInflictor == pScorer )
				{
					// If the inflictor is the killer,  then it must be their current weapon doing the damage
					if ( pScorer->GetActiveWeapon() )
					{
						killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname();
					}
				}
				else
				{
					killer_weapon_name = pInflictor->GetClassname();  // it's just that easy
				}
			}
		}
		else
		{
			killer_weapon_name = pInflictor->GetClassname();
		}

		// strip the NPC_* or weapon_* from the inflictor's classname
		if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		{
			killer_weapon_name += 7;
		}
		else if ( strncmp( killer_weapon_name, "npc_", 4 ) == 0 )
		{
			killer_weapon_name += 4;
		}
		else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		{
			killer_weapon_name += 5;
		}
		else if ( strstr( killer_weapon_name, "physics" ) )
		{
			killer_weapon_name = "physics";
		}

		if ( strcmp( killer_weapon_name, "prop_combine_ball" ) == 0 )
		{
			killer_weapon_name = "combine_ball";
		}
		else if ( strcmp( killer_weapon_name, "grenade_ar2" ) == 0 )
		{
			killer_weapon_name = "smg1_grenade";
		}
		else if ( strcmp( killer_weapon_name, "satchel" ) == 0 || strcmp( killer_weapon_name, "tripmine" ) == 0)
		{
			killer_weapon_name = "slam";
		}


	}

	IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
	if( event )
	{
		event->SetInt("userid", pVictim->GetUserID() );
		event->SetInt("attacker", killer_ID );
		event->SetString("weapon", killer_weapon_name );
		event->SetInt( "priority", 7 );
		gameeventmanager->FireEvent( event );
	}
#endif

}

void CHL2MPRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	
	//CHL2MP_Player *pHL2Player = ToHL2MPPlayer( pPlayer );

	//if ( pHL2Player == NULL )
	//	return;

	//const char *pCurrentModel = modelinfo->GetModelName( pPlayer->GetModel() );
	//const char *szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_playermodel" );

	////If we're different.
	//if ( stricmp( szModelName, pCurrentModel ) )
	//{
	//	//Too soon, set the cvar back to what it was.
	//	//Note: this will make this function be called again
	//	//but since our models will match it'll just skip this whole dealio.
	//	if ( pHL2Player->GetNextModelChangeTime() >= gpGlobals->curtime )
	//	{
	//		char szReturnString[512];

	//		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", pCurrentModel );
	//		engine->ClientCommand ( pHL2Player->edict(), szReturnString );

	//		Q_snprintf( szReturnString, sizeof( szReturnString ), "Please wait %d more seconds before trying to switch.\n", (int)(pHL2Player->GetNextModelChangeTime() - gpGlobals->curtime) );
	//		ClientPrint( pHL2Player, HUD_PRINTTALK, szReturnString );
	//		return;
	//	}

	//	if ( HL2MPRules()->IsTeamplay() == false )
	//	{
	//		pHL2Player->SetPlayerModel();

	//		const char *pszCurrentModelName = modelinfo->GetModelName( pHL2Player->GetModel() );

	//		char szReturnString[128];
	//		Q_snprintf( szReturnString, sizeof( szReturnString ), "Your player model is: %s\n", pszCurrentModelName );

	//		ClientPrint( pHL2Player, HUD_PRINTTALK, szReturnString );
	//	}
	//	else
	//	{
	//		if ( Q_stristr( szModelName, "models/human") )
	//		{
	//			pHL2Player->ChangeTeam( TEAM_REBELS );
	//		}
	//		else
	//		{
	//			pHL2Player->ChangeTeam( TEAM_COMBINE );
	//		}
	//	}
	//}
	//if ( sv_report_client_settings.GetInt() == 1 )
	//{
	//	UTIL_LogPrintf( "\"%s\" cl_cmdrate = \"%s\"\n", pHL2Player->GetPlayerName(), engine->GetClientConVarValue( pHL2Player->entindex(), "cl_cmdrate" ));
	//}

	BaseClass::ClientSettingsChanged( pPlayer );
#endif
	
}

int CHL2MPRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
#ifndef CLIENT_DLL
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() || IsTeamplay() == false )
		return GR_NOTTEAMMATE;

	if ( (*GetTeamID(pPlayer) != '\0') && (*GetTeamID(pTarget) != '\0') && !stricmp( GetTeamID(pPlayer), GetTeamID(pTarget) ) )
	{
		return GR_TEAMMATE;
	}
#endif

	return GR_NOTTEAMMATE;
}

const char *CHL2MPRules::GetGameDescription( void )
{ 
	return "DotA: Source"; 
} 

bool CHL2MPRules::IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer )
{
	return true;
}
 
float CHL2MPRules::GetMapRemainingTime()
{
	// if timelimit is disabled, return 0
	if ( mp_timelimit.GetInt() <= 0 )
		return 0;

	// timelimit is in minutes

	float timeleft = (m_flGameStartTime + mp_timelimit.GetInt() * 60.0f ) - gpGlobals->curtime;

	return timeleft;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPRules::Precache( void )
{
	CBaseEntity::PrecacheScriptSound( "AlyxEmp.Charge" );
}

bool CHL2MPRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		V_swap(collisionGroup0,collisionGroup1);
	}

	if ( (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 

}

bool CHL2MPRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
#ifndef CLIENT_DLL
	if( BaseClass::ClientCommand( pEdict, args ) )
		return true;


	CHL2MP_Player *pPlayer = (CHL2MP_Player *) pEdict;

	if ( pPlayer->ClientCommand( args ) )
		return true;
#endif

	return false;
}

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef *GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;
	
	if ( !bInitted )
	{
		bInitted = true;
//																								plr dmg		npc dmg	max carry	impulse
		def.AddAmmoType("AR2",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			15,		60,			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("AR2AltFire",		DMG_DISSOLVE,				TRACER_NONE,			0,			0,		3,			0,							0 );
		def.AddAmmoType("Pistol",			DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			9,		150,		BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("SMG1",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			5,		225,		BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("357",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			40,		12,			BULLET_IMPULSE(800, 5000),	0 );
		def.AddAmmoType("XBowBolt",			DMG_BULLET,					TRACER_LINE,			0,			70,		10,			BULLET_IMPULSE(800, 8000),	0 );
		def.AddAmmoType("Buckshot",			DMG_BULLET | DMG_BUCKSHOT,	TRACER_LINE,			0,			8,		30,			BULLET_IMPULSE(400, 1200),	0 );
		def.AddAmmoType("RPG_Round",		DMG_BURN,					TRACER_NONE,			0,			90,		3,			0,							0 );
		def.AddAmmoType("SMG1_Grenade",		DMG_BURN,					TRACER_NONE,			0,			55,		3,			0,							0 );
		def.AddAmmoType("Grenade",			DMG_BURN,					TRACER_NONE,			0,			70,		5,			0,							0 );
		def.AddAmmoType("slam",				DMG_BURN,					TRACER_NONE,			0,			75,		5,			0,							0 );


		def.AddAmmoType("AlyxGun",			DMG_BULLET,					TRACER_LINE,			"sk_plr_dmg_alyxgun",		"sk_npc_dmg_alyxgun",		"sk_max_alyxgun",		BULLET_IMPULSE(200, 1225), 0 );
		def.AddAmmoType("SniperRound",		DMG_BULLET | DMG_SNIPER,	TRACER_NONE,			"sk_plr_dmg_sniper_round",	"sk_npc_dmg_sniper_round",	"sk_max_sniper_round",	BULLET_IMPULSE(650, 6000), 0 );
		def.AddAmmoType("SniperPenetratedRound", DMG_BULLET | DMG_SNIPER, TRACER_NONE,			"sk_dmg_sniper_penetrate_plr", "sk_dmg_sniper_penetrate_npc", "sk_max_sniper_round", BULLET_IMPULSE(150, 6000), 0 );
		def.AddAmmoType("Grenade",			DMG_BURN,					TRACER_NONE,			"sk_plr_dmg_grenade",		"sk_npc_dmg_grenade",		"sk_max_grenade",		0, 0);
		def.AddAmmoType("Thumper",			DMG_SONIC,					TRACER_NONE,			10, 10, 2, 0, 0 );
		def.AddAmmoType("Gravity",			DMG_CLUB,					TRACER_NONE,			0,	0, 8, 0, 0 );
		def.AddAmmoType("Battery",			DMG_CLUB,					TRACER_NONE,			NULL, NULL, NULL, 0, 0 );
		//def.AddAmmoType("GaussEnergy",		DMG_SHOCK,					TRACER_NONE,			"sk_jeep_gauss_damage",		"sk_jeep_gauss_damage", "sk_max_gauss_round", BULLET_IMPULSE(650, 8000), 0 ); // hit like a 10kg weight at 400 in/s
		def.AddAmmoType("CombineCannon",	DMG_BULLET,					TRACER_LINE,			"sk_npc_dmg_gunship_to_plr", "sk_npc_dmg_gunship", NULL, 1.5 * 750 * 12, 0 ); // hit like a 1.5kg weight at 750 ft/s
		def.AddAmmoType("AirboatGun",		DMG_AIRBOAT,				TRACER_LINE,			"sk_plr_dmg_airboat",		"sk_npc_dmg_airboat",		NULL,					BULLET_IMPULSE(10, 600), 0 );
		def.AddAmmoType("StriderMinigun",	DMG_BULLET,					TRACER_LINE,			5, 15,15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED ); // hit like a 1.0kg weight at 750 ft/s
		def.AddAmmoType("StriderMinigunDirect",	DMG_BULLET,				TRACER_LINE,			2, 2, 15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED ); // hit like a 1.0kg weight at 750 ft/s
		def.AddAmmoType("HelicopterGun",	DMG_BULLET,					TRACER_LINE_AND_WHIZ,	"sk_npc_dmg_helicopter_to_plr", "sk_npc_dmg_helicopter",	"sk_max_smg1",	BULLET_IMPULSE(400, 1225), AMMO_FORCE_DROP_IF_CARRIED | AMMO_INTERPRET_PLRDAMAGE_AS_DAMAGE_TO_PLAYER );
#ifdef HL2_EPISODIC
		def.AddAmmoType("Hopwire",			DMG_BLAST,					TRACER_NONE,			"sk_plr_dmg_grenade",		"sk_npc_dmg_grenade",		"sk_max_hopwire",		0, 0);
		def.AddAmmoType("CombineHeavyCannon",	DMG_BULLET,				TRACER_LINE,			40,	40, NULL, 10 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED ); // hit like a 10 kg weight at 750 ft/s
		def.AddAmmoType("ammo_proto1",			DMG_BULLET,				TRACER_LINE,			0, 0, 10, 0, 0 );
#endif // HL2_EPISODIC
	}

	return &def;
}

CHeroDef *GetHeroDef()
{
	static CHeroDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;

		def.AddHeroType("freeman",		100,		0.75,		1.00,	"weapon_crowbar",		"weapon_rpg" );
		def.AddHeroType("alyx",			100,		1.25,		1.25,	"weapon_physcannon",	"weapon_frag" );
		def.AddHeroType("barney",		100,		0.75,		1.00,	"weapon_smg1",			"smg1_grenade" );
		def.AddHeroType("eli",			100,		1.25,		0.75,	"weapon_ar2",			"AR2AltFire" );
		def.AddHeroType("monk",			100,		0.80,		1.50,	"weapon_shotgun",		"weapon_slam" );

		def.AddHeroType("gman",			100,		1.35,		0.75,	"weapon_crossbow",		"weapon_slam" );
		def.AddHeroType("mossman",		100,		1.25,		0.75,	"weapon_smg1",			"smg1_grenade" );
		def.AddHeroType("elite",		100,		0.60,		1.00,	"weapon_ar2",			"AR2AltFire" );
		def.AddHeroType("metropolice",	100,		0.75,		1.00,	"weapon_stunstick",		"weapon_rpg" );
		def.AddHeroType("breen",		100,		1.25,		1.00,	"weapon_357",			"weapon_frag" );
	}

	return &def;
}

CItemDef *GetItemDef()
{
	static CItemDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;

		def.AddItemType("weapon_pistol",		900, NULL);
		def.AddItemType("weapon_357",			1200, NULL);
		def.AddItemType("weapon_smg1",			2100, NULL);
		def.AddItemType("weapon_ar2",			3000, NULL);
		def.AddItemType("weapon_shotgun",		2300, NULL);
		def.AddItemType("weapon_crossbow",		2700, NULL);
		def.AddItemType("weapon_physcannon",	1700, NULL);

		def.AddItemType("rpg_round",			700, "weapon_rpg");
		def.AddItemType("slam",					100, "weapon_slam");
		def.AddItemType("grenade",				300, "weapon_frag");

		def.AddItemType("AR2AltFire",			1500, "weapon_ar2");
		def.AddItemType("smg1_grenade",			800, "weapon_smg1");		

		def.AddItemType("medevac",				100, NULL);
	}

	return &def;
}

const char *CHL2MPRules::GetSkillClassForHero( int heroType, int index )
{
	Hero_t * hero = GetHeroDef()->GetHeroOfIndex( heroType );
	if( !hero )
		return NULL;
	
	if ( FStrEq( hero->pName, "freeman" ) )
	{
		switch( index ) {	case 1: return "dota_skill_stun";
							case 2: return "dota_skill_find_health";
							case 3: return "dota_skill_find_armor";
							case 4: return "dota_skill_reload"; }
	}
	else if ( FStrEq( hero->pName, "alyx" ) )
	{
		switch( index ) {	case 1: return "dota_skill_speed_boost";
							case 2: return "dota_skill_evasion";
							case 3: return "dota_skill_sneeky";
							case 4: return "dota_skill_summondog"; }
	}
	else if ( FStrEq( hero->pName, "barney" ) )
	{
		switch( index ) {	case 1: return "dota_skill_throw_explosive";
							case 2: return "dota_skill_take_bribe";
							case 3: return "dota_skill_smg_ammo";
							case 4: return "dota_skill_infiltrate"; }
	}
	else if ( FStrEq( hero->pName, "eli" ) )
	{
		switch( index ) {	case 1: return "dota_skill_heal";
							case 2: return "dota_skill_ar2_ammo";
							case 3: return "dota_skill_fake_death";
							case 4: return "dota_skill_summonrebs"; }
	}
	else if ( FStrEq( hero->pName, "monk" ) )
	{
		switch( index ) {	case 1: return "dota_skill_shotgun_ammo";
							case 2: return "dota_skill_summonheadcrab";
							case 3: return "dota_skill_zombie_invasion";
							case 4: return "dota_skill_summonzombies"; }
	}
	else if ( FStrEq( hero->pName, "gman" ) )
	{
		switch( index ) {	case 1: return "dota_skill_phonehome";
							case 2: return "dota_skill_swap";
							case 3: return "dota_skill_blink";
							case 4: return "dota_skill_time_lapse"; }
	}
	else if ( FStrEq( hero->pName, "mossman" ) )
	{
		switch( index ) {	case 1: return "dota_skill_backstab";
							case 2: return "dota_skill_poison";
							case 3: return "dota_skill_trick";
							case 4: return "dota_skill_infiltrate";}
	}
	else if ( FStrEq( hero->pName, "elite" ) )
	{
		switch( index ) {	case 1: return "dota_skill_drop_wall";
							case 2: return "dota_skill_drop_turret";
							case 3: return "dota_skill_ar2_ammo";
							case 4: return "dota_skill_summon_elites";}
	}
	else if ( FStrEq( hero->pName, "metropolice" ) )
	{
		switch( index ) {	case 1: return "dota_skill_take_bribe";
							case 2: return "dota_skill_summonmanhacks";
							case 3: return "dota_skill_infect";
							case 4: return "dota_skill_summon_police";}
	}
	else if ( FStrEq( hero->pName, "breen" ) )
	{
		switch( index ) {	case 1: return "dota_skill_mass_heal";
							case 2: return "dota_skill_personal_guard";
							case 3: return "dota_skill_colt_ammo";
							case 4: return "dota_skill_rallytroops";}
	}

	return NULL;
}
const char *CHL2MPRules::GetSkillName( const char * skillClass )
{
	if (	  FStrEq( skillClass,  "dota_skill_stun" ) ) return "Stun";
	else if ( FStrEq( skillClass,  "dota_skill_find_health" ) ) return "Find Health";
	else if ( FStrEq( skillClass,  "dota_skill_find_armor" ) ) return "Find Armor";
	else if ( FStrEq( skillClass,  "dota_skill_reload" ) ) return "Reload Saved Game"; 

	else if ( FStrEq( skillClass,  "dota_skill_speed_boost" ) ) return "Speed Boost";
	else if ( FStrEq( skillClass,  "dota_skill_evasion" ) ) return "Evasion";
	else if ( FStrEq( skillClass,  "dota_skill_sneeky" ) ) return "Sneaky";
	else if ( FStrEq( skillClass,  "dota_skill_summondog" ) ) return "Summon Dog"; 
	
	else if ( FStrEq( skillClass,  "dota_skill_throw_explosive" ) ) return "Throw Explosive";
	else if ( FStrEq( skillClass,  "dota_skill_take_bribe" ) ) return "Take Bribe";
	else if ( FStrEq( skillClass,  "dota_skill_smg_ammo" ) ) return "SMG Ammo";
	else if ( FStrEq( skillClass,  "dota_skill_infiltrate" ) ) return "Infiltrate"; 

	else if ( FStrEq( skillClass,  "dota_skill_heal" ) ) return "Heal";
	else if ( FStrEq( skillClass,  "dota_skill_ar2_ammo" ) ) return "AR2 Ammo";
	else if ( FStrEq( skillClass,  "dota_skill_fake_death" ) ) return "Fake Death";
	else if ( FStrEq( skillClass,  "dota_skill_summonrebs" ) ) return "Summon Rebs"; 

	else if ( FStrEq( skillClass,  "dota_skill_shotgun_ammo" ) ) return "Annibelle";
	else if ( FStrEq( skillClass,  "dota_skill_summonheadcrab" ) ) return "Summon Headcrabs";
	else if ( FStrEq( skillClass,  "dota_skill_zombie_invasion" ) ) return "Zombie Invasion";
	else if ( FStrEq( skillClass,  "dota_skill_summonzombies" ) ) return "Summon Zombies"; 

	else if ( FStrEq( skillClass,  "dota_skill_phonehome" ) ) return "Phone Home";
	else if ( FStrEq( skillClass,  "dota_skill_swap" ) ) return "Swap";
	else if ( FStrEq( skillClass,  "dota_skill_blink" ) ) return "Blink";
	else if ( FStrEq( skillClass,  "dota_skill_time_lapse" ) ) return "Time Lapse"; 

	else if ( FStrEq( skillClass,  "dota_skill_backstab" ) ) return "Backstab";
	else if ( FStrEq( skillClass,  "dota_skill_poison" ) ) return "Poison Arrow";
	else if ( FStrEq( skillClass,  "dota_skill_trick" ) ) return "Trick";

	else if ( FStrEq( skillClass,  "dota_skill_drop_wall" ) ) return "Drop Wall";
	else if ( FStrEq( skillClass,  "dota_skill_drop_turret" ) ) return "Drop Turret";
	else if ( FStrEq( skillClass,  "dota_skill_summon_elites" ) ) return "Call Elite Backup";

	else if ( FStrEq( skillClass,  "dota_skill_summonmanhacks" ) ) return "Summon Manhacks";
	else if ( FStrEq( skillClass,  "dota_skill_infect" ) ) return "Infect";
	else if ( FStrEq( skillClass,  "dota_skill_summon_police" ) ) return "Call Police Backup";

	else if ( FStrEq( skillClass,  "dota_skill_mass_heal" ) ) return "Mass Heal";
	else if ( FStrEq( skillClass,  "dota_skill_personal_guard" ) ) return "Personal Guard";
	else if ( FStrEq( skillClass,  "dota_skill_colt_ammo" ) ) return "Colt Ammo";
	else if ( FStrEq( skillClass,  "dota_skill_rallytroops" ) ) return "Rally Troops";

	return NULL;
}
const char *CHL2MPRules::GetSkillDesc( const char * skillClass )
{
	if (	  FStrEq( skillClass, "dota_skill_stun" ) ) return "Re-orient enemy hero's view with the crowbar";
	else if ( FStrEq( skillClass, "dota_skill_find_health" ) ) return "Creeps killed by freeman drop health";
	else if ( FStrEq( skillClass,  "dota_skill_find_armor" ) ) return "Creeps killed by freeman drop armor";
	else if ( FStrEq( skillClass,  "dota_skill_reload" ) ) return "Some say Freeman’s success only possible because he can reload a saved game.  There is no telling how many times he’s been killed or lost his mind in unspeakable scientist killing rampages, only to reload a saved game and go on to become a hero as if it never happed!  When killed Freeman will respawn with no delay and without rewarding the killer."; 

	else if ( FStrEq( skillClass,  "dota_skill_speed_boost" ) ) return "Activate to gain a temporary speed increase";
	else if ( FStrEq( skillClass,  "dota_skill_evasion" ) ) return "Alyx has a chance to dodge bullets. (only bullet damage)";
	else if ( FStrEq( skillClass,  "dota_skill_sneeky" ) ) return "Alyx blends in and becomes harder to see.";
	else if ( FStrEq( skillClass,  "dota_skill_summondog" ) ) return "Alyx can summon her powerful ally, and pet, Dog.   He will briefly wreak some havoc on the combine."; 
	
	else if ( FStrEq( skillClass,  "dota_skill_throw_explosive" ) ) return "Throw an explosive canister that will explode when damaged.";
	else if ( FStrEq( skillClass,  "dota_skill_take_bribe" ) ) return "Kill a creep with the crowbar for extra money.";
	else if ( FStrEq( skillClass,  "dota_skill_smg_ammo" ) ) return "Get SMG ammo. (does not count as buying the gun)";
	else if ( FStrEq( skillClass,  "dota_skill_infiltrate" ) ) return "Creeps see you as an ally and will not attack you."; 

	else if ( FStrEq( skillClass,  "dota_skill_heal" ) ) return "Throw health packs.  Eli cannot use them himself.";
	else if ( FStrEq( skillClass,  "dota_skill_ar2_ammo" ) ) return "Get AR2 Ammo. (does not count as buying the gun)";
	else if ( FStrEq( skillClass,  "dota_skill_fake_death" ) ) return "Dead body falls, real hero is breifly invisible.";
	else if ( FStrEq( skillClass,  "dota_skill_summonrebs" ) ) return "Summon rebel citizen to fight, heal and ammo resupply."; 

	else if ( FStrEq( skillClass,  "dota_skill_shotgun_ammo" ) ) return "Get Shotgun ammo. (does not count as buying the gun)";
	else if ( FStrEq( skillClass,  "dota_skill_summonheadcrab" ) ) return "Summon headcrabs to attack the enemy.";
	else if ( FStrEq( skillClass,  "dota_skill_zombie_invasion" ) ) return "Summon a zombie behind each enemy hero.";
	else if ( FStrEq( skillClass,  "dota_skill_summonzombies" ) ) return "Summon a horde of zombies to fight."; 

	else if ( FStrEq( skillClass,  "dota_skill_phonehome" ) ) return "Instantly takes you to your base.";
	else if ( FStrEq( skillClass,  "dota_skill_swap" ) ) return "Change places with any hero or creep.";
	else if ( FStrEq( skillClass,  "dota_skill_blink" ) ) return "Instantly teleport toward where you are aiming.";
	else if ( FStrEq( skillClass,  "dota_skill_time_lapse" ) ) return "Go back 5 seconds in time. Only your own health/armor and position is lapsed"; 

	else if ( FStrEq( skillClass,  "dota_skill_backstab" ) ) return "Do extra damage if you are not in your enemies line of sight.";
	else if ( FStrEq( skillClass,  "dota_skill_poison" ) ) return "Activate to make your next crossbow bolt posionous. Posion dramatically reduces enemy health, but only temporaroly";
	else if ( FStrEq( skillClass,  "dota_skill_trick" ) ) return "Change a creep to your own team.";

	else if ( FStrEq( skillClass,  "dota_skill_drop_wall" ) ) return "Drop a wall to hide behind.";
	else if ( FStrEq( skillClass,  "dota_skill_drop_turret" ) ) return "Drop a deadly turret. The turret is destroyed by being knocked over.";
	else if ( FStrEq( skillClass,  "dota_skill_summon_elites" ) ) return "Summon elite soldiers.";

	else if ( FStrEq( skillClass,  "dota_skill_summonmanhacks" ) ) return "Summon manhacks to attack your enemy.";
	else if ( FStrEq( skillClass,  "dota_skill_infect" ) ) return "Slow your enemy down.";
	else if ( FStrEq( skillClass,  "dota_skill_summon_police" ) ) return "Summon Police. They also take bribes. Also adds a chance that stunstick will holster the enemies weapon.";

	else if ( FStrEq( skillClass,  "dota_skill_mass_heal" ) ) return "Heal every ally around you.";
	else if ( FStrEq( skillClass,  "dota_skill_personal_guard" ) ) return "Summon extra combine soldiers";
	else if ( FStrEq( skillClass,  "dota_skill_colt_ammo" ) ) return "Get Colt ammo. (does not count as buying the gun)";
	else if ( FStrEq( skillClass,  "dota_skill_rallytroops" ) ) return "Increase the health of all combine soldiers on the map.";

	return NULL;
}
int			CHL2MPRules::GetSkillCooldown( const char * skillClass,  int level )
{
	if		( FStrEq( skillClass,  "dota_skill_reload" ) )			return 360-(level*60); 

	else if ( FStrEq( skillClass,  "dota_skill_speed_boost" ) )		return 60;
	else if ( FStrEq( skillClass,  "dota_skill_summondog" ) )		return 300; 
	
	else if ( FStrEq( skillClass,  "dota_skill_throw_explosive" ) ) return 300-(level*60);
	else if ( FStrEq( skillClass,  "dota_skill_smg_ammo" ) )		return 210-(level*30);
	else if ( FStrEq( skillClass,  "dota_skill_infiltrate" ) )		return 180; 

	else if ( FStrEq( skillClass,  "dota_skill_heal" ) )			return 35 - (level * 5);
	else if ( FStrEq( skillClass,  "dota_skill_ar2_ammo" ) )		return 210-(level*30);
	else if ( FStrEq( skillClass,  "dota_skill_fake_death" ) )		return 210-(level*30);
	else if ( FStrEq( skillClass,  "dota_skill_summonrebs" ) )		return 300; 

	else if ( FStrEq( skillClass,  "dota_skill_shotgun_ammo" ) )	return 210-(level*30);
	else if ( FStrEq( skillClass,  "dota_skill_summonheadcrab" ) )	return 240;
	else if ( FStrEq( skillClass,  "dota_skill_zombie_invasion" ) ) return 300;
	else if ( FStrEq( skillClass,  "dota_skill_summonzombies" ) )	return 300; 

	else if ( FStrEq( skillClass,  "dota_skill_phonehome" ) )		return 85-(level*10);
	else if ( FStrEq( skillClass,  "dota_skill_swap" ) )			return 85-(level*10);
	else if ( FStrEq( skillClass,  "dota_skill_blink" ) )			return 14-(2*level);
	else if ( FStrEq( skillClass,  "dota_skill_time_lapse" ) )		return 70-(level*10); 

	else if ( FStrEq( skillClass,  "dota_skill_poison" ) )			return 120;
	else if ( FStrEq( skillClass,  "dota_skill_trick" ) )			return 360-(level*60);

	else if ( FStrEq( skillClass,  "dota_skill_drop_wall" ) )		return 180;
	else if ( FStrEq( skillClass,  "dota_skill_drop_turret" ) )		return 210-(level*30);
	else if ( FStrEq( skillClass,  "dota_skill_summon_elites" ) )	return 300;

	else if ( FStrEq( skillClass,  "dota_skill_summonmanhacks" ) )	return 300;
	else if ( FStrEq( skillClass,  "dota_skill_infect" ) )			return 60;
	else if ( FStrEq( skillClass,  "dota_skill_summon_police" ) )	return 300;

	else if ( FStrEq( skillClass,  "dota_skill_mass_heal" ) )		return 120;
	else if ( FStrEq( skillClass,  "dota_skill_personal_guard" ) )	return 180;
	else if ( FStrEq( skillClass,  "dota_skill_colt_ammo" ) )		return 150-(level*30);
	else if ( FStrEq( skillClass,  "dota_skill_rallytroops" ) )		return 300;

	return 0;
}
int			CHL2MPRules::GetSkillDuration( const char * skillClass,  int level )
{
	if (	  FStrEq( skillClass, "dota_skill_speed_boost" ) )	return 5+(level*5);
	else if ( FStrEq( skillClass, "dota_skill_summondog" ) )	return 180;	
	else if ( FStrEq( skillClass, "dota_skill_infiltrate" ) )	return level*10; 
	else if ( FStrEq( skillClass, "dota_skill_infect" ) )		return (level*5);
	else if ( FStrEq( skillClass, "dota_skill_fake_death" ) )	return 5;
	else if ( FStrEq( skillClass, "dota_skill_poison" ) )		return 1000;
	else if ( FStrEq( skillClass, "dota_skill_infect" ) )		return level*5;

	return 0;
}
float		CHL2MPRules::GetSkillFormula( const char * skillClass,  int level )
{
	float fLevel = level;

	if (	  FStrEq( skillClass,  "dota_skill_find_health" ) )		return fLevel*25.0f;
	else if ( FStrEq( skillClass,  "dota_skill_find_armor" ) )		return fLevel*15.0f;

	else if ( FStrEq( skillClass,  "dota_skill_speed_boost" ) )		return 1.0f + (fLevel * 0.5f);
	else if ( FStrEq( skillClass,  "dota_skill_evasion" ) )			return fLevel * 10.0f;
	else if ( FStrEq( skillClass,  "dota_skill_sneeky" ) )			return fLevel * 50.0f;
	else if ( FStrEq( skillClass,  "dota_skill_summondog" ) )		return 250.0f + (250.0f * fLevel); 
	
	else if ( FStrEq( skillClass,  "dota_skill_take_bribe" ) )		return fLevel * 100.0f;
	else if ( FStrEq( skillClass,  "dota_skill_smg_ammo" ) )		return 60.0f;

	else if ( FStrEq( skillClass,  "dota_skill_heal" ) )			return 5.0f + (5.0f * fLevel);
	else if ( FStrEq( skillClass,  "dota_skill_ar2_ammo" ) )		return 60.0f;

	else if ( FStrEq( skillClass,  "dota_skill_shotgun_ammo" ) )	return 8.0f;
	else if ( FStrEq( skillClass,  "dota_skill_zombie_invasion" ) ) return 25.0f * fLevel;

	else if ( FStrEq( skillClass,  "dota_skill_blink" ) )			return (4.0f + (3.0f * fLevel)) * 36.0f;

	else if ( FStrEq( skillClass,  "dota_skill_backstab" ) )		return (0.2f * fLevel);
	else if ( FStrEq( skillClass,  "dota_skill_poison" ) )			return ( 0.8f - (0.19f * fLevel));

	else if ( FStrEq( skillClass,  "dota_skill_drop_wall" ) )		return 100.0f * fLevel;

	else if ( FStrEq( skillClass,  "dota_skill_summonmanhacks" ) )	return 5.0f * fLevel ;
	else if ( FStrEq( skillClass,  "dota_skill_summon_police" ) )	return 75.0f + (fLevel * 25.0f);

	else if ( FStrEq( skillClass,  "dota_skill_personal_guard" ) )	return fLevel * 50.0f;
	else if ( FStrEq( skillClass,  "dota_skill_colt_ammo" ) )		return 15.0f;
	else if ( FStrEq( skillClass,  "dota_skill_rallytroops" ) )		return fLevel;

	return 0.0f;
}
const char *CHL2MPRules::GetSkillFormulaLabel( const char * skillClass,  int level )
{
	if (	  FStrEq( skillClass,  "dota_skill_stun" ) )
		switch( level ) {	case 1: return "25% chance reorient victim's view";
							case 2: return "additional 25% chance to send them flying";
							case 3: return "additional 25% chance to make them face directly away from you";
							case 4: return "additional 25% chance to make them face directly away and look striaght up.";}
	else if ( FStrEq( skillClass,  "dota_skill_find_health" ) ) return "health";
	else if ( FStrEq( skillClass,  "dota_skill_find_armor" ) )	return "armor";

	else if ( FStrEq( skillClass,  "dota_skill_speed_boost" ) ) return "times increase";
	else if ( FStrEq( skillClass,  "dota_skill_evasion" ) )		return "% chance";
	else if ( FStrEq( skillClass,  "dota_skill_sneeky" ) )		return "transparent out of 255";
	else if ( FStrEq( skillClass,  "dota_skill_summondog" ) )	return "health"; 
	
	else if ( FStrEq( skillClass,  "dota_skill_take_bribe" ) )	return "extra money";
	else if ( FStrEq( skillClass,  "dota_skill_smg_ammo" ) )	return "bullets";

	else if ( FStrEq( skillClass,  "dota_skill_heal" ) )		return "health";
	else if ( FStrEq( skillClass,  "dota_skill_ar2_ammo" ) )	return "bullets";
	else if ( FStrEq( skillClass,  "dota_skill_summonrebs" ) )	
		switch( level ) {	case 1: return "3 citizens";
							case 2: return "3 additional medics";
							case 3: return "3 additional ammo suppliers";}

	else if ( FStrEq( skillClass,  "dota_skill_shotgun_ammo" ) ) return "Shells";
	else if ( FStrEq( skillClass,  "dota_skill_summonheadcrab" ) )
		switch( level ) {	case 1: return "4 slow";
							case 2: return "9 slow";
							case 3: return "9 fast";
							case 4: return "14 fast";}
	else if ( FStrEq( skillClass,  "dota_skill_zombie_invasion" ) ) return "health each";
	else if ( FStrEq( skillClass,  "dota_skill_summonzombies" ) )
		switch( level ) {	case 1: return "4 classic";
							case 2: return "4 poision";
							case 3: return "4 fast";}

	else if ( FStrEq( skillClass,  "dota_skill_blink" ) )		return "distance (16units/foot)";

	else if ( FStrEq( skillClass,  "dota_skill_backstab" ) )	return "times damage exta";
	else if ( FStrEq( skillClass,  "dota_skill_poison" ) )		return "victim's health remaining";

	else if ( FStrEq( skillClass,  "dota_skill_drop_wall" ) )	return "health";
	else if ( FStrEq( skillClass,  "dota_skill_summon_elites" ) )
		switch( level ) {	case 1: return "summoned at your base";
							case 2: return "summoned near you";
							case 3: return "summoned where you aim";}

	else if ( FStrEq( skillClass,  "dota_skill_summonmanhacks" ) ) return "hacks";
	else if ( FStrEq( skillClass,  "dota_skill_summon_police" ) ) return "health each";

	else if ( FStrEq( skillClass,  "dota_skill_mass_heal" ) )
		switch( level ) {	case 1: return "50% health";
							case 2: return "100% health";
							case 3: return "100% health and 50% armor";
							case 4: return "100% health and 100% armor";}
	else if ( FStrEq( skillClass,  "dota_skill_personal_guard" ) ) return "health each";
	else if ( FStrEq( skillClass,  "dota_skill_colt_ammo" ) )	return "bullets";
	else if ( FStrEq( skillClass,  "dota_skill_rallytroops" ) ) return "hundred % health increase";

	return NULL;
}
#ifdef CLIENT_DLL

	ConVar cl_autowepswitch(
		"cl_autowepswitch",
		"1",
		FCVAR_ARCHIVE | FCVAR_USERINFO,
		"Automatically switch to picked up weapons (if more powerful)" );

#else

#ifdef DEBUG

	// Handler for the "bot" command.
	void Bot_f()
	{		
		// Look at -count.
		int count = 1;
		count = clamp( count, 1, 16 );

		int iTeam = TEAM_COMBINE;
				
		// Look at -frozen.
		bool bFrozen = false;
			
		// Ok, spawn all the bots.
		while ( --count >= 0 )
		{
			BotPutInServer( bFrozen, iTeam );
		}
	}


	ConCommand cc_Bot( "bot", Bot_f, "Add a bot.", FCVAR_CHEAT );

#endif

	bool CHL2MPRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
	{		
		if ( pPlayer->GetActiveWeapon() && pPlayer->IsNetClient() )
		{
			// Player has an active item, so let's check cl_autowepswitch.
			const char *cl_autowepswitch = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_autowepswitch" );
			if ( cl_autowepswitch && atoi( cl_autowepswitch ) <= 0 )
			{
				return false;
			}
		}

		return BaseClass::FShouldSwitchWeapon( pPlayer, pWeapon );
	}

#endif

#ifndef CLIENT_DLL

void CHL2MPRules::RestartGame()
{
	// bounds check
	if ( mp_timelimit.GetInt() < 0 )
	{
		mp_timelimit.SetValue( 0 );
	}
	m_flGameStartTime = gpGlobals->curtime;
	if ( !IsFinite( m_flGameStartTime.Get() ) )
	{
		Warning( "Trying to set a NaN game start time\n" );
		m_flGameStartTime.GetForModify() = 0.0f;
	}

	CleanUpMap();
	
	// now respawn all players
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		if ( pPlayer->GetActiveWeapon() )
		{
			pPlayer->GetActiveWeapon()->Holster();
		}
		pPlayer->RemoveAllItems( true );
		if ( !pPlayer->IsObserver() )
			respawn( pPlayer, false );
		pPlayer->Reset();
		//pPlayer->ShowViewPortPanel( PANEL_TEAM, true, NULL );	
	}

	// Respawn entities (glass, doors, etc..)

	CTeam *pRebels = GetGlobalTeam( TEAM_REBELS );
	CTeam *pCombine = GetGlobalTeam( TEAM_COMBINE );

	if ( pRebels )
	{
		pRebels->SetScore( 0 );
	}

	if ( pCombine )
	{
		pCombine->SetScore( 0 );
	}

	m_flIntermissionEndTime = 0;
	m_flRestartGameTime = 0.0;		
	m_bCompleteReset = false;

	IGameEvent * event = gameeventmanager->CreateEvent( "round_start" );
	if ( event )
	{
		event->SetInt("fraglimit", 0 );
		event->SetInt( "priority", 6 ); // HLTV event priority, not transmitted

		event->SetString("objective","DEATHMATCH");

		gameeventmanager->FireEvent( event );
	}
}

void CHL2MPRules::CleanUpMap()
{
	// Recreate all the map entities from the map data (preserving their indices),
	// then remove everything else except the players.

	// Get rid of all entities except players.
	CBaseEntity *pCur = gEntList.FirstEnt();
	while ( pCur )
	{
		CBaseHL2MPCombatWeapon *pWeapon = dynamic_cast< CBaseHL2MPCombatWeapon* >( pCur );
		// Weapons with owners don't want to be removed..
		if ( pWeapon )
		{
			if ( !pWeapon->GetPlayerOwner() )
			{
				UTIL_Remove( pCur );
			}
		}
		// remove entities that has to be restored on roundrestart (breakables etc)
		else if ( !FindInList( s_PreserveEnts, pCur->GetClassname() ) )
		{
			//don't remove the skill entities
			CBaseSkill * pSkill = dynamic_cast< CBaseSkill* >( pCur );
			if( !pSkill )
			{
				UTIL_Remove( pCur );
			}
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();

	// Cancel all queued events, in case a func_bomb_target fired some delayed outputs that
	// could kill respawning CTs
	g_EventQueue.Clear();

	// Now reload the map entities.
	class CHL2MPMapEntityFilter : public IMapEntityFilter
	{
	public:
		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			// Don't recreate the preserved entities.
			if ( !FindInList( s_PreserveEnts, pClassname ) )
			{
				return true;
			}
			else
			{
				// Increment our iterator since it's not going to call CreateNextEntity for this ent.
				if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
					m_iIterator = g_MapEntityRefs.Next( m_iIterator );

				return false;
			}
		}


		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CCSMapLoadEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert( false );
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

				if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName( pClassname );
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator; // Iterator into g_MapEntityRefs.
	};
	CHL2MPMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// DO NOT CALL SPAWN ON info_node ENTITIES!

	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );
}

void CHL2MPRules::CheckChatForReadySignal( CHL2MP_Player *pPlayer, const char *chatmsg )
{
	if( m_bAwaitingReadyRestart && FStrEq( chatmsg, mp_ready_signal.GetString() ) )
	{
		if( pPlayer->GetHeroType() != -1 )
		{
			if( !pPlayer->IsReady() )
			{
				pPlayer->SetReady( true );
			}
		}
	}
}

void CHL2MPRules::CheckRestartGame( void )
{
	// Restart the game if specified by the server
	int iRestartDelay = mp_restartgame.GetInt();

	if ( iRestartDelay > 0 )
	{
		if ( iRestartDelay > 60 )
			iRestartDelay = 60;


		// let the players know
		char strRestartDelay[64];
		Q_snprintf( strRestartDelay, sizeof( strRestartDelay ), "%d", iRestartDelay );
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "Game will restart in %s1 %s2", strRestartDelay, iRestartDelay == 1 ? "SECOND" : "SECONDS" );
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "Game will restart in %s1 %s2", strRestartDelay, iRestartDelay == 1 ? "SECOND" : "SECONDS" );

		m_flRestartGameTime = gpGlobals->curtime + iRestartDelay;
		m_bCompleteReset = true;
		mp_restartgame.SetValue( 0 );
	}

	if( mp_readyrestart.GetBool() )
	{
		m_bAwaitingReadyRestart = true;
		m_bHeardAllPlayersReady = false;
		

		const char *pszReadyString = mp_ready_signal.GetString();


		// Don't let them put anything malicious in there
		if( pszReadyString == NULL || Q_strlen(pszReadyString) > 16 )
		{
			pszReadyString = "ready";
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "hl2mp_ready_restart" );
		if ( event )
			gameeventmanager->FireEvent( event );

		mp_readyrestart.SetValue( 0 );

		// cancel any restart round in progress
		m_flRestartGameTime = -1;		
	}
}

void CHL2MPRules::CheckAllPlayersReady( void )
{
	bool atLeastOnePlayerReady = false;

	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;
		if ( !pPlayer->IsReady() )
		{
			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();
			UTIL_ClientPrintFilter( user, HUD_PRINTCENTER, "Type ""READY"" in the chat to begin the game" );
			return;
		}
		else
		{
			atLeastOnePlayerReady = true;
		}
	}
	if ( atLeastOnePlayerReady )
		m_bHeardAllPlayersReady = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CHL2MPRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
	{
		return NULL;
	}

	const char *pszFormat = NULL;

	// team only
	if ( bTeamOnly == TRUE )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "HL2MP_Chat_Spec";
		}
		else
		{
			const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
			if ( chatLocation && *chatLocation )
			{
				pszFormat = "HL2MP_Chat_Team_Loc";
			}
			else
			{
				pszFormat = "HL2MP_Chat_Team";
			}
		}
	}
	// everyone
	else
	{
		if ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
		{
			pszFormat = "HL2MP_Chat_All";	
		}
		else
		{
			pszFormat = "HL2MP_Chat_AllSpec";
		}
	}

	return pszFormat;
}

void CHL2MPRules::InitDefaultAIRelationships( void )
{
	int i, j;

	//  Allocate memory for default relationships
	CBaseCombatCharacter::AllocateDefaultRelationships();

	// --------------------------------------------------------------
	// First initialize table so we can report missing relationships
	// --------------------------------------------------------------
	for (i=0;i<NUM_AI_CLASSES;i++)
	{
		for (j=0;j<NUM_AI_CLASSES;j++)
		{
			// By default all relationships are neutral of priority zero
			CBaseCombatCharacter::SetDefaultRelationship( (Class_T)i, (Class_T)j, D_NU, 0 );
		}
	}

	// ------------------------------------------------------------
	//	> CLASS_ANTLION
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PROTOSNIPER,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_ANTLION,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_BARNACLE
	//
	//  In this case, the relationship D_HT indicates which characters
	//  the barnacle will try to eat.
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BARNACLE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_MANHACK,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_EARTH_FAUNA,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_BULLSEYE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_ANTLION,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_VORTIGAUNT,		D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_BULLSQUID
	// ------------------------------------------------------------
	/*
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BULLSEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HEADCRAB,			D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HOUNDEYE,			D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MANHACK,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HACKED_ROLLERMINE,D_HT, 0);
	*/
	// ------------------------------------------------------------
	//	> CLASS_CITIZEN_PASSIVE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_COMBINE_HUNTER,	D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HEADCRAB,			D_FR, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HOUNDEYE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_MANHACK,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_MISSILE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_VORTIGAUNT,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_ZOMBIE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HACKED_ROLLERMINE,D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_CITIZEN_REBEL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_MISSILE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_VORTIGAUNT,		D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HACKED_ROLLERMINE,D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_COMBINE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_COMBINE_GUNSHIP,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_COMBINE_HUNTER,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE_GUNSHIP
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_COMBINE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_COMBINE_GUNSHIP,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_COMBINE_HUNTER,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_MISSILE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE_HUNTER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_COMBINE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_COMBINE_GUNSHIP,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_COMBINE_HUNTER,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_CONSCRIPT
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_VORTIGAUNT,		D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HACKED_ROLLERMINE,D_NU, 0);
	
	// ------------------------------------------------------------
	//	> CLASS_FLARE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_ANTLION,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_VORTIGAUNT,		D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_HEADCRAB
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HACKED_ROLLERMINE,D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_HOUNDEYE
	// ------------------------------------------------------------
	/*
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HEADCRAB,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);
	*/

	// ------------------------------------------------------------
	//	> CLASS_MANHACK
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_HEADCRAB,			D_HT,-1);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,		CLASS_HOUNDEYE,			D_HT,-1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_METROPOLICE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_MILITARY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_MISSILE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_NONE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_ANTLION,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BULLSEYE,			D_NU, 0);	
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_VORTIGAUNT,		D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HACKED_ROLLERMINE,D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_BARNACLE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_BULLSEYE,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_CITIZEN_PASSIVE,	D_LI, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_CITIZEN_REBEL,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_COMBINE_GUNSHIP,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_VORTIGAUNT,		D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PROTOSNIPER,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_HACKED_ROLLERMINE,D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PLAYER,			D_LI, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_BARNACLE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_HEADCRAB,			D_FR, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_VORTIGAUNT,		D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_ZOMBIE,			D_FR, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PROTOSNIPER,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_HACKED_ROLLERMINE,D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY_VITAL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER,			D_LI, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BARNACLE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_COMBINE_HUNTER,	D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_VORTIGAUNT,		D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PROTOSNIPER,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HACKED_ROLLERMINE,D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_SCANNER
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,		CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_COMBINE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_COMBINE_GUNSHIP,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_COMBINE_HUNTER,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,		CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_MANHACK,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_METROPOLICE,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_MILITARY,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_SCANNER,			D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_STALKER,			D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PROTOSNIPER,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_STALKER
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,		CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_VORTIGAUNT
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PLAYER,			D_LI, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_CITIZEN_PASSIVE,	D_LI, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_CITIZEN_REBEL,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_VORTIGAUNT,		D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_HACKED_ROLLERMINE,D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_ZOMBIE
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,		CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,		CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_MANHACK,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_MILITARY,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_PROTOSNIPER
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,		CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,		CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_MISSILE,			D_NU, 5);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_EARTH_FAUNA
	//
	// Hates pretty much everything equally except other earth fauna.
	// This will make the critter choose the nearest thing as its enemy.
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_NONE,				D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PLAYER,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_COMBINE_GUNSHIP,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_FLARE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_MISSILE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_SCANNER,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_STALKER,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_VORTIGAUNT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PROTOSNIPER,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_HACKED_ROLLERMINE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PLAYER,			D_LI, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_VORTIGAUNT,		D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_ZOMBIE,			D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_EARTH_FAUNA,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HACKED_ROLLERMINE,D_LI, 0);
}

#endif
