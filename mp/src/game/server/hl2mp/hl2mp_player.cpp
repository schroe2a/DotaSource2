//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL2.
//
//=============================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "hl2mp_player.h"
#include "globalstate.h"
#include "game.h"
#include "gamerules.h"
#include "hl2mp_player_shared.h"
#include "predicted_viewmodel.h"
#include "in_buttons.h"
#include "hl2mp_gamerules.h"
#include "KeyValues.h"
#include "team.h"
#include "ammodef.h"
#include "HeroDef.h"
#include "ItemDef.h"
#include "weapon_hl2mpbase.h"
#include "grenade_satchel.h"
#include "eventqueue.h"
#include "gamestats.h"
#include "ai_behavior_standoff.h"
#include "npc_turret_floor.h"
#include "Skills.h"

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "vgui/ISurface.h"
#include <vgui_controls/Controls.h>
#include "ilagcompensationmanager.h"

int g_iLastCitizenModel = 0;
int g_iLastCombineModel = 0;

CBaseEntity	 *g_pLastCombineSpawn = NULL;
CBaseEntity	 *g_pLastRebelSpawn = NULL;
extern CBaseEntity				*g_pLastSpawn;

#define HL2MP_COMMAND_MAX_RATE 0.3

void DropPrimedFragGrenade( CHL2MP_Player *pPlayer, CBaseCombatWeapon *pGrenade );

LINK_ENTITY_TO_CLASS( player, CHL2MP_Player );

LINK_ENTITY_TO_CLASS( info_player_combine, CPointEntity );
LINK_ENTITY_TO_CLASS( info_player_rebel, CPointEntity );

IMPLEMENT_SERVERCLASS_ST(CHL2MP_Player, DT_HL2MP_Player)

	SendPropFloat( SENDINFO( maxWalkSpeed ) ),
	SendPropFloat( SENDINFO( maxNormalSpeed ) ),
	SendPropFloat( SENDINFO( maxSprintSpeed ) ),

	SendPropEHandle( SENDINFO( m_hSkill1 ) ),
	SendPropEHandle( SENDINFO( m_hSkill2 ) ),
	SendPropEHandle( SENDINFO( m_hSkill3 ) ),
	SendPropEHandle( SENDINFO( m_hSkill4 ) ),

	SendPropInt( SENDINFO( m_canShop ) ),

	SendPropInt( SENDINFO( m_iHasPistol ) ),
	SendPropInt( SENDINFO( m_iHasSMG ) ),
	SendPropInt( SENDINFO( m_iHasAR2 ) ),
	SendPropInt( SENDINFO( m_iHasBuckshot ) ),
	SendPropInt( SENDINFO( m_iHas357 ) ),
	SendPropInt( SENDINFO( m_iHasXBow ) ),
	SendPropInt( SENDINFO( m_iHasPhysCannon ) ),

	SendPropInt( SENDINFO( m_iMoney ) ),

	SendPropInt( SENDINFO( m_HeroType ) ),
	SendPropInt( SENDINFO( m_iSkillPoints ) ),
	SendPropInt( SENDINFO( m_iStatLevel ) ),	

	SendPropInt( SENDINFO( m_iExp ) ),
	SendPropInt( SENDINFO( m_iLevel ) ),

	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 11, SPROP_CHANGES_OFTEN ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 11, SPROP_CHANGES_OFTEN ),
	SendPropEHandle( SENDINFO( m_hRagdoll ) ),
	SendPropInt( SENDINFO( m_iSpawnInterpCounter), 4 ),
	SendPropInt( SENDINFO( m_iPlayerSoundType), 3 ),
	
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),

//	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
//	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),
	
END_SEND_TABLE()

BEGIN_DATADESC( CHL2MP_Player )
END_DATADESC()

#define MAX_COMBINE_MODELS 4
#define MODEL_CHANGE_INTERVAL 5.0f
#define TEAM_CHANGE_INTERVAL 5.0f

#define HL2MPPLAYER_PHYSDAMAGE_SCALE 4.0f

#pragma warning( disable : 4355 )

CON_COMMAND( addxp, "gives xp" )
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );

	if ( args.ArgC() != 2 )
		return;

	pPlayer->AddXP( Q_atoi( args[ 1 ] ) );
}
CON_COMMAND( addmoney, "gives money" )
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );

	if ( args.ArgC() != 2 )
		return;

	pPlayer->AddMoney( Q_atoi( args[ 1 ] ) );
}
CON_COMMAND( skill1, "execute your skill")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if ( pPlayer && pPlayer->GetSkill(1) && pPlayer->IsAlive() )
	{
		pPlayer->GetSkill(1)->Use();
	}
}
CON_COMMAND( skill2, "execute your skill")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if ( pPlayer && pPlayer->GetSkill(2) && pPlayer->IsAlive() )
	{
		pPlayer->GetSkill(2)->Use();
	}
}
CON_COMMAND( skill3, "execute your skill")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if ( pPlayer && pPlayer->GetSkill(3) && pPlayer->IsAlive() )
	{
		pPlayer->GetSkill(3)->Use();
	}
}
CON_COMMAND( skill4, "execute your skill")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if ( pPlayer && pPlayer->GetSkill(4) && pPlayer->IsAlive() )
	{
		pPlayer->GetSkill(4)->Use();
	}
}
CBaseSkill * CHL2MP_Player::GetSkill(int index)
{
	switch ( index )
	{
	case 1:
		return dynamic_cast< CBaseSkill* >( m_hSkill1.Get() );
	case 2:
		return dynamic_cast< CBaseSkill* >( m_hSkill2.Get() );
	case 3:
		return dynamic_cast< CBaseSkill* >( m_hSkill3.Get() );
	case 4:
		return dynamic_cast< CBaseSkill* >( m_hSkill4.Get() );
	}
	return NULL;
}
void CHL2MP_Player::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	for ( int i = 1; i<=4; i++ )
	{
		if (this->GetSkill(i))
		{
			this->GetSkill(i)->Event_KilledOther( pVictim, info );
		}
	}
}
CHL2MP_Player::CHL2MP_Player() : m_PlayerAnimState( this )
{
	m_angEyeAngles.Init();

	m_iLastWeaponFireUsercmd = 0;

	m_flNextModelChangeTime = 0.0f;
	m_flNextTeamChangeTime = 0.0f;

	m_iSpawnInterpCounter = 0;

    m_bEnterObserver = false;
	m_bReady = false;

	m_nextRegen = 0;

	BaseClass::ChangeTeam( 0 );

	m_iExp = 0;
	m_iLevel = 0;
		
	m_HeroType = -1;
	
	m_iMoney = 0;

	m_iSkillPoints = 0;
	m_iStatLevel = 0;

	m_iHasPistol = 0;
	m_iHasSMG = 0;
	m_iHasAR2 = 0;
	m_iHasBuckshot = 0;
	m_iHas357 = 0;
	m_iHasXBow = 0;
	m_iHasPhysCannon = 0;

	m_canShop = 0;

	//	UseClientSideAnimation();
}

CHL2MP_Player::~CHL2MP_Player( void )
{

}

#define MAX_LEVEL 25
void CHL2MP_Player::CheckLevel()
{
	bool bShouldLevel = false;
	while ( GetLevel() < MAX_LEVEL && GetXP() >= (GetLevel() + 1) * 100 )
	{
		m_iExp -= (GetLevel() + 1) * 100;
		m_iLevel++;
		m_iSkillPoints++;
		bShouldLevel = true;
	}	

	if ( bShouldLevel )
	{
		OnStatsChanged(); // and then adjust their settings (speed, health, damage) to reflect the change
		ClientPrint( this, HUD_PRINTTALK, UTIL_VarArgs("You have reached level %i\n", GetLevel()) ); // write it on their screen 
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, UTIL_VarArgs("%s has reached level %i\n", GetPlayerName(), GetLevel()) ); // write it in everyone's console
		if (m_iLevel!=6 && m_iLevel!=10 && m_iLevel!=16) { // Issue #12: AMP - 2013-09-22 - Play sound when leveling up
			vgui::surface()->PlaySound( "player/levelUp1.mp3" ); // Play normal level-up sound
		} else {
			vgui::surface()->PlaySound( "player/levelUp2.mp3" ); // Play "ultimate" level-up sound
		}
	}
}

extern CSuitPowerDevice SuitDeviceSprint;
void CHL2MP_Player::OnStatsChanged()
{
	if ( GetHeroType() == -1 )
		return;

	SuitDeviceSprint = CSuitPowerDevice( bits_SUIT_DEVICE_SPRINT, 25.0f -(m_iStatLevel * 1.25 * GetHeroDef()->SpeedBonus( GetHeroType() ) ) );
}

int CHL2MP_Player::GetMaxHealth() const
{
	if ( m_HeroType != -1 )
	{
		return GetHeroDef()->BaseHealth( m_HeroType ) * (1.0f + (m_iStatLevel * 0.1f) );
	}

	return BaseClass::GetMaxHealth();
}
int CHL2MP_Player::GetWeaponLevel( const char *pszWeapon )
{
	if ( Q_strcmp(pszWeapon, "weapon_pistol") == 0 ) return m_iHasPistol;
	else if ( Q_strcmp(pszWeapon, "weapon_357") == 0 ) return m_iHas357;
	else if ( Q_strcmp(pszWeapon, "weapon_smg1") == 0 ) return m_iHasSMG;
	else if ( Q_strcmp(pszWeapon, "weapon_ar2") == 0 ) return m_iHasAR2;
	else if ( Q_strcmp(pszWeapon, "weapon_shotgun") == 0 ) return m_iHasBuckshot;
	else if ( Q_strcmp(pszWeapon, "weapon_crossbow") == 0 ) return m_iHasXBow;
	
	return 0;
}
void CHL2MP_Player::SetWeaponLevel( const char *pszWeapon, int level )
{
	if ( Q_strcmp(pszWeapon, "weapon_pistol") == 0 ) m_iHasPistol = level;
	else if ( Q_strcmp(pszWeapon, "weapon_357") == 0 ) m_iHas357 = level;
	else if ( Q_strcmp(pszWeapon, "weapon_smg1") == 0 ) m_iHasSMG = level;
	else if ( Q_strcmp(pszWeapon, "weapon_ar2") == 0 ) m_iHasAR2 = level;
	else if ( Q_strcmp(pszWeapon, "weapon_shotgun") == 0 ) m_iHasBuckshot = level;
	else if ( Q_strcmp(pszWeapon, "weapon_crossbow") == 0 ) m_iHasXBow = level;
}
void CHL2MP_Player::UpdateOnRemove( void )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	BaseClass::UpdateOnRemove();
}

void CHL2MP_Player::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel ( "sprites/glow01.vmt" );
	
	char szModelName[512];

	for ( int i = 1; i < MAX_AMMO_TYPES; i++ )
	{
		if ( GetHeroDef()->GetHeroOfIndex(i) )
		{
			Q_snprintf( szModelName, sizeof (szModelName ), "models/player/%s/%s.mdl", GetHeroDef()->GetHeroOfIndex(i)->pName, GetHeroDef()->GetHeroOfIndex(i)->pName );
			PrecacheModel ( szModelName );
		}
	}

	PrecacheFootStepSounds();

	PrecacheScriptSound( "NPC_MetroPolice.Die" );
	PrecacheScriptSound( "NPC_CombineS.Die" );
	PrecacheScriptSound( "NPC_Citizen.die" );

	PrecacheScriptSound( "npc_citizen.abouttime01" );
	PrecacheScriptSound( "streetwar.al_letsgo" );
	PrecacheScriptSound( "npc_barney.ba_oldtimes" );
	PrecacheScriptSound( "citadel.eli_genocide" );
	PrecacheScriptSound( "ravenholm.cartrap_iamgrig" );

	PrecacheScriptSound( "gman_misc.gman_03" );
	PrecacheScriptSound( "novaprospekt.mo_onlyway" );
	PrecacheScriptSound( "prison.sradio_D7_controlroom" );
	PrecacheScriptSound( "NPC_MetroPolice.Cupcop.Intro" );
	PrecacheScriptSound( "breencast.br_tofreeman07" );

	PrecacheScriptSound( "prison.overwatch_antlions_inside_prison1" );

	UTIL_PrecacheOther( "npc_dog" );
	UTIL_PrecacheOther( "npc_zombie" );
	UTIL_PrecacheOther( "npc_poisonzombie" );
	UTIL_PrecacheOther( "npc_fastzombie" );
	UTIL_PrecacheOther( "npc_turret_floor" );
	UTIL_PrecacheOther( "npc_manhack" );
	UTIL_PrecacheOther( "npc_citizen" );
	UTIL_PrecacheOther( "npc_combine_s" );
	UTIL_PrecacheOther( "npc_metropolice" );

	PrecacheScriptSound( "breencast.br_overwatch07" );
	PrecacheScriptSound( "Weapon_StunStick.Activate" );
	PrecacheScriptSound( "Weapon_StunStick.Deactivate" );

	PrecacheModel( "models/props_c17/canister01a.mdl" );
	PrecacheModel( "models/combine_super_soldier.mdl" );
	PrecacheModel( "models/props_combine/combine_barricade_short01a.mdl" );
}

void CHL2MP_Player::GiveAllItems( void )
{
	EquipSuit();

	CBasePlayer::GiveAmmo( 255,	"Pistol");
	CBasePlayer::GiveAmmo( 255,	"AR2" );
	CBasePlayer::GiveAmmo( 5,	"AR2AltFire" );
	CBasePlayer::GiveAmmo( 255,	"SMG1");
	CBasePlayer::GiveAmmo( 1,	"smg1_grenade");
	CBasePlayer::GiveAmmo( 255,	"Buckshot");
	CBasePlayer::GiveAmmo( 32,	"357" );
	CBasePlayer::GiveAmmo( 3,	"rpg_round");

	CBasePlayer::GiveAmmo( 1,	"grenade" );
	CBasePlayer::GiveAmmo( 2,	"slam" );

	GiveNamedItem( "weapon_crowbar" );
	GiveNamedItem( "weapon_stunstick" );
	GiveNamedItem( "weapon_pistol" );
	GiveNamedItem( "weapon_357" );

	GiveNamedItem( "weapon_smg1" );
	GiveNamedItem( "weapon_ar2" );
	
	GiveNamedItem( "weapon_shotgun" );
	GiveNamedItem( "weapon_frag" );
	
	GiveNamedItem( "weapon_crossbow" );
	
	GiveNamedItem( "weapon_rpg" );

	GiveNamedItem( "weapon_slam" );

	GiveNamedItem( "weapon_physcannon" );
	
}

void CHL2MP_Player::GiveDefaultItems( void )
{
	EquipSuit();

	if ( GetTeamNumber() == TEAM_COMBINE )
		GiveNamedItem( "weapon_stunstick" );
	else
		GiveNamedItem( "weapon_crowbar" );
	
	if ( m_iHasPistol )
		GiveNamedItem( "weapon_pistol" );

	if ( m_iHasSMG )
		GiveNamedItem( "weapon_smg1" );

	if ( m_iHasAR2 )
		GiveNamedItem( "weapon_ar2" );

	if ( m_iHasBuckshot )
		GiveNamedItem( "weapon_shotgun" );

	if ( m_iHas357 )
		GiveNamedItem( "weapon_357" );

	if ( m_iHasXBow )
		GiveNamedItem( "weapon_crossbow" );

	if ( m_iHasPhysCannon )
		GiveNamedItem( "weapon_physcannon" );					
}

void CHL2MP_Player::PickDefaultSpawnTeam( void )
{
	if ( GetTeamNumber() == 0 )
    {
        ChangeTeam( TEAM_SPECTATOR );        
    }
}

void CHL2MP_Player::Spawn(void)
{
	m_flNextModelChangeTime = 0.0f;
	m_flNextTeamChangeTime = 0.0f;

	PickDefaultSpawnTeam();
	
	BaseClass::Spawn();

	if ( !IsObserver() )
	{
		pl.deadflag = false;
		RemoveSolidFlags( FSOLID_NOT_SOLID );

		RemoveEffects( EF_NODRAW );
		
		GiveDefaultItems();
	}

	OnStatsChanged();

	SetNumAnimOverlays( 3 );
	ResetAnimation();

	m_nRenderFX = kRenderNormal;

	m_Local.m_iHideHUD = 0;
	
	AddFlag(FL_ONGROUND); // set the player on the ground at the start of the round.

	m_impactEnergyScale = HL2MPPLAYER_PHYSDAMAGE_SCALE;

	if ( HL2MPRules()->IsIntermission() )
	{
		AddFlag( FL_FROZEN );
	}
	else
	{
		RemoveFlag( FL_FROZEN );
	}

	m_iSpawnInterpCounter = (m_iSpawnInterpCounter + 1) % 8;

	m_Local.m_bDucked = false;

	SetPlayerUnderwater(false);

	//m_bReady = false;

	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		StopObserverMode();
		PlaySpawnSound( STRING( GetModelName() ) );

		this->SetHealth( this->GetMaxHealth() );
		this->SetArmorValue( this->GetMaxHealth() );

		maxWalkSpeed = 150;
		maxNormalSpeed = 190;
		maxSprintSpeed = 320;
	}
	else
	{
		//if we are a spectator then go into roaming mode
		StartObserverMode( OBS_MODE_ROAMING );
	}

	for ( int i = 1; i<=4; i++ )
	{
		if (this->GetSkill(i))
		{
			this->GetSkill(i)->OnPlayerSpawn();
		}
	}
}
void CHL2MP_Player::PlaySpawnSound( const char *pModelName )
{
	if ( Q_stristr(pModelName, "freeman" ) )
		EmitSound( "npc_citizen.abouttime01" );
	else if ( Q_stristr(pModelName, "alyx" ) )
		EmitSound( "streetwar.al_letsgo" );
	else if ( Q_stristr(pModelName, "barney" ) )
		EmitSound( "npc_barney.ba_oldtimes" );
	else if ( Q_stristr(pModelName, "monk" ) )
		EmitSound( "ravenholm.cartrap_iamgrig" );
	else if ( Q_stristr(pModelName, "gman" ) )
		EmitSound("gman_misc.gman_03");
	else if ( Q_stristr(pModelName, "mossman" ) )
		EmitSound("novaprospekt.mo_onlyway");
	else if ( Q_stristr(pModelName, "elite" ) )
		EmitSound("prison.sradio_D7_controlroom");
	else if ( Q_stristr(pModelName, "metropolice" ) )
		EmitSound("NPC_MetroPolice.Cupcop.Intro");
	else if ( Q_stristr(pModelName, "breen" ) )
		EmitSound("breencast.br_tofreeman07");
	else if ( Q_stristr(pModelName, "eli" ) )
		EmitSound( "citadel.eli_genocide" );	
}
void CHL2MP_Player::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
	
}

void CHL2MP_Player::SetupPlayerSoundsByModel( const char *pModelName )
{
	if ( Q_stristr(pModelName, "police" ) )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_METROPOLICE;
	}
	else if ( Q_stristr(pModelName, "elite" ) )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_COMBINESOLDIER;
	}
	else
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_CITIZEN;
	}	
}

void CHL2MP_Player::ResetAnimation( void )
{
	if ( IsAlive() )
	{
		SetSequence ( -1 );
		SetActivity( ACT_INVALID );

		if (!GetAbsVelocity().x && !GetAbsVelocity().y)
			SetAnimation( PLAYER_IDLE );
		else if ((GetAbsVelocity().x || GetAbsVelocity().y) && ( GetFlags() & FL_ONGROUND ))
			SetAnimation( PLAYER_WALK );
		else if (GetWaterLevel() > 1)
			SetAnimation( PLAYER_WALK );
	}
}


bool CHL2MP_Player::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	bool bRet = BaseClass::Weapon_Switch( pWeapon, viewmodelindex );

	if ( bRet == true )
	{
		ResetAnimation();
	}

	return bRet;
}

void CHL2MP_Player::PreThink( void )
{
	QAngle vOldAngles = GetLocalAngles();
	QAngle vTempAngles = GetLocalAngles();

	vTempAngles = EyeAngles();

	if ( vTempAngles[PITCH] > 180.0f )
	{
		vTempAngles[PITCH] -= 360.0f;
	}

	SetLocalAngles( vTempAngles );

	BaseClass::PreThink();
	State_PreThink();

	if ( HL2MPRules()->GameStarted() && !IsDead() && !IsObserver() && GetTeamNumber() != TEAM_SPECTATOR && GetHeroType() != -1 )
	{
		if ( m_nextRegen < gpGlobals->curtime )
		{
			this->AddMoney( 8 );

			if ( this->GetHealth() < this->GetMaxHealth() )
				this->SetHealth( this->GetHealth() + 1);
			else
				this->IncrementArmorValue( 1, this->GetMaxHealth() );

			m_nextRegen = gpGlobals->curtime + 3.5f;
		}		
	}

	//Reset bullet force accumulator, only lasts one frame
	m_vecTotalBulletForce = vec3_origin;
	SetLocalAngles( vOldAngles );
}

void CHL2MP_Player::PostThink( void )
{
	BaseClass::PostThink();
	
	if ( GetFlags() & FL_DUCKING )
	{
		SetCollisionBounds( VEC_CROUCH_TRACE_MIN, VEC_CROUCH_TRACE_MAX );
	}

	m_PlayerAnimState.Update();

	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );
}

void CHL2MP_Player::PlayerDeathThink()
{
	if( !IsObserver() )
	{
		BaseClass::PlayerDeathThink();
	}
}

void CHL2MP_Player::FireBullets ( const FireBulletsInfo_t &info )
{
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( this, this->GetCurrentCommand() );

	FireBulletsInfo_t modinfo = info;

	CWeaponHL2MPBase *pWeapon = dynamic_cast<CWeaponHL2MPBase *>( GetActiveWeapon() );

	if ( pWeapon )
	{
		if( GetAmmoDef()->GetAmmoOfIndex( modinfo.m_iAmmoType ) )
			modinfo.m_iPlayerDamage = modinfo.m_flDamage = GetAmmoDef()->NPCDamage( modinfo.m_iAmmoType );
		else
			modinfo.m_iPlayerDamage = modinfo.m_flDamage = pWeapon->GetHL2MPWpnData().m_iPlayerDamage;
	}

	NoteWeaponFired();

	BaseClass::FireBullets( modinfo );

	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( this );
}

void CHL2MP_Player::NoteWeaponFired( void )
{
	Assert( m_pCurrentCommand );
	if( m_pCurrentCommand )
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}
}

extern ConVar sv_maxunlag;
bool CHL2MP_Player::WantsLagCompensationOnEntity( const CBaseEntity *pEntity, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	// No need to lag compensate at all if we're not attacking in this command and
	// we haven't attacked recently.
	if ( !( pCmd->buttons & IN_ATTACK ) && (pCmd->command_number - m_iLastWeaponFireUsercmd > 5) )
		return false;

	// If this entity hasn't been transmitted to us and acked, then don't bother lag compensating it.
	if ( pEntityTransmitBits && !pEntityTransmitBits->Get( pEntity->entindex() ) )
		return false;

	const Vector &vMyOrigin = GetAbsOrigin();
	const Vector &vHisOrigin = pEntity->GetAbsOrigin();

	// get max distance player could have moved within max lag compensation time, 
	// multiply by 1.5 to to avoid "dead zones"  (sqrt(2) would be the exact value)
	float maxspeed;
	CBasePlayer *pPlayer = ToBasePlayer((CBaseEntity*)pEntity);
	if ( pPlayer )
		maxspeed = pPlayer->MaxSpeed();
	else
		maxspeed = 600;
	float maxDistance = 1.5 * maxspeed * sv_maxunlag.GetFloat();

	// If the player is within this distance, lag compensate them in case they're running past us.
	if ( vHisOrigin.DistTo( vMyOrigin ) < maxDistance )
		return true;

	// If their origin is not within a 45 degree cone in front of us, no need to lag compensate.
	Vector vForward;
	AngleVectors( pCmd->viewangles, &vForward );
	
	Vector vDiff = vHisOrigin - vMyOrigin;
	VectorNormalize( vDiff );

	float flCosAngle = 0.707107f;	// 45 degree angle
	if ( vForward.Dot( vDiff ) < flCosAngle )
		return false;

	return true;
}

Activity CHL2MP_Player::TranslateTeamActivity( Activity ActToTranslate )
{
	//if ( m_iModelType == TEAM_COMBINE )
	//	 return ActToTranslate;
	
	if ( ActToTranslate == ACT_RUN )
		 return ACT_RUN_AIM_AGITATED;

	if ( ActToTranslate == ACT_IDLE )
		 return ACT_IDLE_AIM_AGITATED;

	if ( ActToTranslate == ACT_WALK )
		 return ACT_WALK_AIM_AGITATED;

	return ActToTranslate;
}

extern ConVar hl2_normspeed;
void CHL2MP_Player::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;

	float speed;

	speed = GetAbsVelocity().Length2D();

	
	// bool bRunning = true;

	//Revisit!
/*	if ( ( m_nButtons & ( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT ) ) )
	{
		if ( speed > 1.0f && speed < hl2_normspeed.GetFloat() - 20.0f )
		{
			bRunning = false;
		}
	}*/

	if ( GetFlags() & ( FL_FROZEN | FL_ATCONTROLS ) )
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	Activity idealActivity = ACT_HL2MP_RUN;

	// This could stand to be redone. Why is playerAnim abstracted from activity? (sjb)
	if ( playerAnim == PLAYER_JUMP )
	{
		idealActivity = ACT_HL2MP_JUMP;
	}
	else if ( playerAnim == PLAYER_DIE )
	{
		if ( m_lifeState == LIFE_ALIVE )
		{
			return;
		}
	}
	else if ( playerAnim == PLAYER_ATTACK1 )
	{
		if ( GetActivity( ) == ACT_HOVER	|| 
			 GetActivity( ) == ACT_SWIM		||
			 GetActivity( ) == ACT_HOP		||
			 GetActivity( ) == ACT_LEAP		||
			 GetActivity( ) == ACT_DIESIMPLE )
		{
			idealActivity = GetActivity( );
		}
		else
		{
			idealActivity = ACT_HL2MP_GESTURE_RANGE_ATTACK;
		}
	}
	else if ( playerAnim == PLAYER_RELOAD )
	{
		idealActivity = ACT_HL2MP_GESTURE_RELOAD;
	}
	else if ( playerAnim == PLAYER_IDLE || playerAnim == PLAYER_WALK )
	{
		if ( !( GetFlags() & FL_ONGROUND ) && GetActivity( ) == ACT_HL2MP_JUMP )	// Still jumping
		{
			idealActivity = GetActivity( );
		}
		/*
		else if ( GetWaterLevel() > 1 )
		{
			if ( speed == 0 )
				idealActivity = ACT_HOVER;
			else
				idealActivity = ACT_SWIM;
		}
		*/
		else
		{
			if ( GetFlags() & FL_DUCKING )
			{
				if ( speed > 0 )
				{
					idealActivity = ACT_HL2MP_WALK_CROUCH;
				}
				else
				{
					idealActivity = ACT_HL2MP_IDLE_CROUCH;
				}
			}
			else
			{
				if ( speed > 0 )
				{
					/*
					if ( bRunning == false )
					{
						idealActivity = ACT_WALK;
					}
					else
					*/
					{
						idealActivity = ACT_HL2MP_RUN;
					}
				}
				else
				{
					idealActivity = ACT_HL2MP_IDLE;
				}
			}
		}

		idealActivity = TranslateTeamActivity( idealActivity );
	}
	
	if ( idealActivity == ACT_HL2MP_GESTURE_RANGE_ATTACK )
	{
		RestartGesture( Weapon_TranslateActivity( idealActivity ) );

		// FIXME: this seems a bit wacked
		Weapon_SetActivity( Weapon_TranslateActivity( ACT_RANGE_ATTACK1 ), 0 );

		return;
	}
	else if ( idealActivity == ACT_HL2MP_GESTURE_RELOAD )
	{
		RestartGesture( Weapon_TranslateActivity( idealActivity ) );
		return;
	}
	else
	{
		SetActivity( idealActivity );

		animDesired = SelectWeightedSequence( Weapon_TranslateActivity ( idealActivity ) );

		if (animDesired == -1)
		{
			animDesired = SelectWeightedSequence( idealActivity );

			if ( animDesired == -1 )
			{
				animDesired = 0;
			}
		}
	
		// Already using the desired animation?
		if ( GetSequence() == animDesired )
			return;

		m_flPlaybackRate = 1.0;
		ResetSequence( animDesired );
		SetCycle( 0 );
		return;
	}

	// Already using the desired animation?
	if ( GetSequence() == animDesired )
		return;

	//Msg( "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	ResetSequence( animDesired );
	SetCycle( 0 );
}


extern int	gEvilImpulse101;
bool CHL2MP_Player::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if ( !IsAllowedToPickupWeapons() )
		return false;

	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( pWeapon );
		}
		return false;
	}

	// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	if( !pWeapon->FVisible( this, MASK_SOLID ) && !(GetFlags() & FL_NOTARGET) )
	{
		return false;
	}

	bool bOwnsWeaponAlready = !!Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType());

	if ( bOwnsWeaponAlready == true ) 
	{
		//If we have room for the ammo, then "take" the weapon too.
		 if ( Weapon_EquipAmmoOnly( pWeapon ) )
		 {
			 pWeapon->CheckRespawn();

			 UTIL_Remove( pWeapon );
			 return true;
		 }
		 else
		 {
			 return false;
		 }
	}

	pWeapon->CheckRespawn();
	Weapon_Equip( pWeapon );

	return true;
}

int CHL2MP_Player::GetHeroType()
{
	return m_HeroType;
}

void CHL2MP_Player::ChangeTeam( int iTeam )
{
	bool bKill = false;

	if ( HL2MPRules()->IsTeamplay() == true )
	{
		if ( iTeam != GetTeamNumber() && GetTeamNumber() != TEAM_UNASSIGNED )
		{
			bKill = true;
		}
	}

	BaseClass::ChangeTeam( iTeam );

	Reset();

	if ( m_HeroType != -1 )
	{
		GetHeroDef()->GetHeroOfIndex( m_HeroType )->pTakenBy = NULL;
	}

	m_HeroType = -1;

	UTIL_RemoveImmediate( m_hSkill1 );
	m_hSkill1 = NULL;
	UTIL_RemoveImmediate( m_hSkill2 );
	m_hSkill2 = NULL;
	UTIL_RemoveImmediate( m_hSkill3 );
	m_hSkill3 = NULL;
	UTIL_RemoveImmediate( m_hSkill4 );
	m_hSkill4 = NULL;

	if ( iTeam == TEAM_SPECTATOR )
	{
		RemoveAllItems( true );

		SetModel( "models/player/freeman/freeman.mdl" );

		State_Transition( STATE_OBSERVER_MODE );
	}

	if ( bKill == true )
	{
		CommitSuicide();
	}
}

bool CHL2MP_Player::HandleCommand_JoinTeam( int team )
{
	//auto assign if you join team 0
	if ( team == 0 )
	{
		if ( g_Teams[TEAM_COMBINE]->GetNumPlayers() > g_Teams[TEAM_REBELS]->GetNumPlayers() )
			team = TEAM_REBELS;
		else
			team = TEAM_COMBINE;
	}

	if ( !GetGlobalTeam( team ) || team == 0 )
	{
		Warning( "HandleCommand_JoinTeam( %d ) - invalid team index.\n", team );
		return false;
	}

	if ( team == TEAM_SPECTATOR )
	{
		// Prevent this is the cvar is set
		if ( !mp_allowspectators.GetInt() )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
			return false;
		}

		if ( GetTeamNumber() != TEAM_UNASSIGNED && !IsDead() )
		{
			m_fNextSuicideTime = gpGlobals->curtime;	// allow the suicide to work

			CommitSuicide();

			// add 1 to frags to balance out the 1 subtracted for killing yourself
			IncrementFragCount( 1 );
		}

		ChangeTeam( TEAM_SPECTATOR );

		return true;
	}
	else
	{
		StopObserverMode();
		State_Transition(STATE_ACTIVE);
		// popup classmenu when joining a team
	   if ( team == 2 )
	   {
		  ShowViewPortPanel( "c_class", true );
	   }
	   else if ( team == 3 )
	   {
		  ShowViewPortPanel( "r_class", true );
	   }		
	}

	// Switch their actual team...
	ChangeTeam( team );

	return true;
}
ConVar dota_allow_same_hero( "dota_allow_same_hero", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY );
bool CHL2MP_Player::HandleCommand_JoinClass( int hero )
{
	if ( hero == -1 )
	{
		Warning( "HandleCommand_JoinClass( %d ) - invalid hero index.\n", hero );
		return false;
	}

	if( m_HeroType != -1 )
	{
		Warning( "Cannot change your hero once it's been set.\n");
		return false;
	}

	Hero_t * heroStruct = GetHeroDef()->GetHeroOfIndex( hero );
	if( !heroStruct )
		return false;

	if( heroStruct->pTakenBy && !dota_allow_same_hero.GetInt() == 1 )
	{		
		Warning( "That hero is taken buy %s.\n", heroStruct->pTakenBy->GetPlayerName() );
		return false;
	}

	m_HeroType = hero;

	heroStruct->pTakenBy = this;

	char szModelName[512];
	Q_snprintf( szModelName, sizeof (szModelName ), "models/player/%s/%s.mdl", heroStruct->pName, heroStruct->pName );
	SetModel( szModelName );

	SetupPlayerSoundsByModel( szModelName );

	CreateSkills( m_HeroType );

	OnStatsChanged();

	return true;
}

void CHL2MP_Player::CreateSkills( int hero )
{
	m_hSkill1 = CreateSkillEntity( HL2MPRules()->GetSkillClassForHero( hero, 1 ) );
	m_hSkill2 = CreateSkillEntity( HL2MPRules()->GetSkillClassForHero( hero, 2 ) );
	m_hSkill3 = CreateSkillEntity( HL2MPRules()->GetSkillClassForHero( hero, 3 ) );
	m_hSkill4 = CreateSkillEntity( HL2MPRules()->GetSkillClassForHero( hero, 4 ) );
}
CBaseSkill * CHL2MP_Player::CreateSkillEntity( const char *pSkillEntityName )
{
	CBaseSkill * skill = dynamic_cast< CBaseSkill* >( CreateEntityByName( pSkillEntityName ) );
	//if ( skill )
	{
		skill->SetPlayer( this );
	}
	return skill;
}

bool CHL2MP_Player::HandleCommand_UpgradeSkill( int iSkill )
{
	if ( iSkill < 1 || iSkill > 5 )
	{
		Warning( "HandleCommand_UpgradeSkill( %d ) - invalid skill index.\n", iSkill );
		return false;
	}

	if( !HL2MPRules()->GameStarted() )
	{
		return false;
	}

	m_iSkillPoints--;
	
	if ( iSkill == 5 )
	{
		m_iStatLevel++;
		this->OnStatsChanged();
	}
	else
	{
		this->GetSkill(iSkill)->LevelUp();
	}
		
	return true;
}

bool CHL2MP_Player::HandleCommand_Buy( int iItem )
{
	if ( iItem < 1 )
	{
		Warning( "HandleCommand_Buy( %d ) - invalid item index.\n", iItem );
		return false;
	}

	Item_t *item = GetItemDef()->GetItemOfIndex( iItem );

	int currentLevel = this->GetWeaponLevel( item->pName );
	int totalCost = item->cost + ( currentLevel * (item->cost / 2) );

	if ( m_iMoney < totalCost )
	{
		Warning( "Can't afford Item.\n", iItem );
		return false;
	}	

	if (item->pWeaponNeeded)
	{
		GiveNamedItem(item->pWeaponNeeded);
		int iAmmoType = GetAmmoDef()->Index(item->pName);
		this->GiveAmmo( 1, iAmmoType, false );
	}
	else
	{
		if( FStrEq( item->pName, "medevac" ) )
		{
			HL2MPRules()->GetPlayerSpawnSpot( this );
		}
		else
		{
			GiveNamedItem(item->pName);
			this->SetWeaponLevel( item->pName, currentLevel + 1 );
		}		
	}
	
	m_iMoney -= totalCost;

	return true;
}

bool CHL2MP_Player::ClientCommand( const CCommand &args )
{
	if ( FStrEq( args[0], "spectate" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			// instantly join spectators
			HandleCommand_JoinTeam( TEAM_SPECTATOR );	
		}
		return true;
	}
	else if ( FStrEq( args[0], "jointeam" ) ) 
	{
		if ( args.ArgC() < 2 )
		{
			Warning( "Player sent bad jointeam syntax\n" );
		}

		if ( IsReady() )
		{
			ClientPrint( this, HUD_PRINTTALK, "You cannot change teams once you say ready" ); // write it on their screen
			return true;
		}

		if ( ShouldRunRateLimitedCommand( args ) )
		{
			int iTeam = atoi( args[1] );
			HandleCommand_JoinTeam( iTeam );
		}
		return true;
	}
	else if ( FStrEq( args[0], "joinclass" ) ) 
	{
		if ( args.ArgC() < 2 )
		{
			Warning( "Player sent bad joinclass syntax\n" );
		}

		if ( IsReady() )
		{
			ClientPrint( this, HUD_PRINTTALK, "You cannot change heroes once you say ready" ); // write it on their screen
			ChangeTeam( TEAM_SPECTATOR );
			return true;
		}

		if ( ShouldRunRateLimitedCommand( args ) )
		{
			int iHero = GetHeroDef()->Index( args[1] );
			if( !HandleCommand_JoinClass( iHero ) )
				ChangeTeam( TEAM_SPECTATOR );
		}
		return true;
	}
	else if ( FStrEq( args[0], "joingame" ) )
	{
		return true;
	}
	else if ( FStrEq( args[0], "upgradeskill" ) )
	{
		if ( args.ArgC() < 2 )
		{
			Warning( "Player sent bad upgradeskill syntax\n" );
		}
		
		int iSkill = atoi( args[1] );
		return HandleCommand_UpgradeSkill( iSkill );
	}
	else if ( FStrEq( args[0], "buy" ) )
	{
		if ( args.ArgC() < 2 )
		{
			Warning( "Player sent bad buy syntax\n" );
		}
		
		int iItem = GetItemDef()->Index( args[1] );
		return HandleCommand_Buy( iItem );
	}

	return BaseClass::ClientCommand( args );
}

void CHL2MP_Player::CheatImpulseCommands( int iImpulse )
{
	switch ( iImpulse )
	{
		case 101:
			{
				if( sv_cheats->GetBool() )
				{
					GiveAllItems();
				}
			}
			break;

		default:
			BaseClass::CheatImpulseCommands( iImpulse );
	}
}

bool CHL2MP_Player::ShouldRunRateLimitedCommand( const CCommand &args )
{
	int i = m_RateLimitLastCommandTimes.Find( args[0] );
	if ( i == m_RateLimitLastCommandTimes.InvalidIndex() )
	{
		m_RateLimitLastCommandTimes.Insert( args[0], gpGlobals->curtime );
		return true;
	}
	else if ( (gpGlobals->curtime - m_RateLimitLastCommandTimes[i]) < HL2MP_COMMAND_MAX_RATE )
	{
		// Too fast.
		return false;
	}
	else
	{
		m_RateLimitLastCommandTimes[i] = gpGlobals->curtime;
		return true;
	}
}

void CHL2MP_Player::CreateViewModel( int index /*=0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	CPredictedViewModel *vm = ( CPredictedViewModel * )CreateEntityByName( "predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

bool CHL2MP_Player::BecomeRagdollOnClient( const Vector &force )
{
	return true;
}

class CHL2MPRagdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CHL2MPRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

public:
	// In case the client has the player entity, we transmit the player index.
	// In case the client doesn't have it, we transmit the player's model index, origin, and angles
	// so they can create a ragdoll in the right place.
	CNetworkHandle( CBaseEntity, m_hPlayer );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

LINK_ENTITY_TO_CLASS( hl2mp_ragdoll, CHL2MPRagdoll );
IMPLEMENT_SERVERCLASS_ST_NOBASE( CHL2MPRagdoll, DT_HL2MPRagdoll )
	SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt		( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) )
END_SEND_TABLE()
void CHL2MP_Player::CreateRagdollEntity( void )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	// If we already have a ragdoll, don't make another one.
	CHL2MPRagdoll *pRagdoll = dynamic_cast< CHL2MPRagdoll* >( m_hRagdoll.Get() );
	
	if ( !pRagdoll )
	{
		// create a new one
		pRagdoll = dynamic_cast< CHL2MPRagdoll* >( CreateEntityByName( "hl2mp_ragdoll" ) );
	}

	if ( pRagdoll )
	{
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_vecForce = m_vecTotalBulletForce;
		pRagdoll->SetAbsOrigin( GetAbsOrigin() );
	}

	// ragdolls will be removed on round restart automatically
	m_hRagdoll = pRagdoll;
}

int CHL2MP_Player::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

extern ConVar flashlight;
void CHL2MP_Player::FlashlightTurnOn( void )
{
	if( flashlight.GetInt() > 0 && IsAlive() )
	{
		AddEffects( EF_DIMLIGHT );
		EmitSound( "HL2Player.FlashlightOn" );
	}
}


void CHL2MP_Player::FlashlightTurnOff( void )
{
	RemoveEffects( EF_DIMLIGHT );
	
	if( IsAlive() )
	{
		EmitSound( "HL2Player.FlashlightOff" );
	}
}

void CHL2MP_Player::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity )
{
	//Drop a grenade if it's primed.
	if ( GetActiveWeapon() )
	{
		CBaseCombatWeapon *pGrenade = Weapon_OwnsThisType("weapon_frag");

		if ( GetActiveWeapon() == pGrenade )
		{
			if ( ( m_nButtons & IN_ATTACK ) || (m_nButtons & IN_ATTACK2) )
			{
				DropPrimedFragGrenade( this, pGrenade );
				return;
			}
		}
	}

	BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );
}


void CHL2MP_Player::DetonateTripmines( void )
{
	CBaseEntity *pEntity = NULL;

	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "npc_satchel" )) != NULL)
	{
		CSatchelCharge *pSatchel = dynamic_cast<CSatchelCharge *>(pEntity);
		if (pSatchel->m_bIsLive && pSatchel->GetThrower() == this )
		{
			g_EventQueue.AddEvent( pSatchel, "Explode", 0.20, this, this );
		}
	}

	// Play sound for pressing the detonator
	EmitSound( "Weapon_SLAM.SatchelDetonate" );
}

void CHL2MP_Player::Event_Killed( const CTakeDamageInfo &info )
{
	//update damage info with our accumulated physics force
	CTakeDamageInfo subinfo = info;
	subinfo.SetDamageForce( m_vecTotalBulletForce );

	SetNumAnimOverlays( 0 );

	// Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
	// because we still want to transmit to the clients in our PVS.
	CreateRagdollEntity();

	DetonateTripmines();

	BaseClass::Event_Killed( subinfo );

	if ( info.GetDamageType() & DMG_DISSOLVE )
	{
		if ( m_hRagdoll )
		{
			m_hRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
		}
	}

	CBaseEntity *pAttacker = info.GetAttacker();

	if ( pAttacker )
	{
		int iScoreToAdd = 1;

		if ( pAttacker == this )
		{
			iScoreToAdd = -1;
		}

		//GetGlobalTeam( pAttacker->GetTeamNumber() )->AddScore( iScoreToAdd );
	}

	FlashlightTurnOff();

	m_lifeState = LIFE_DEAD;

	RemoveEffects( EF_NODRAW );	// still draw player body
	StopZooming();
}

int CHL2MP_Player::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	//return here if the player is in the respawn grace period vs. slams.
	if ( gpGlobals->curtime < m_flSlamProtectTime &&  (inputInfo.GetDamageType() == DMG_BLAST ) )
		return 0;

	if( dynamic_cast<CHL2MP_Player*>(inputInfo.GetAttacker()) )
	{
		CRecipientFilter user;
		user.AddRecipientsByTeam( this->GetTeam() );
		user.MakeReliable();
		char szText[200];
		Q_snprintf( szText, sizeof(szText), "Your Ally %s is under attack from an enemy hero!", this->GetPlayerName() );
		UTIL_ClientPrintFilter( user, HUD_PRINTCENTER, szText );
	}
	CTakeDamageInfo adjustedInfo = inputInfo;

	for ( int i = 1; i<=4; i++ )
	{
		if (this->GetSkill(i))
		{
			this->GetSkill(i)->OnTakeDamage( adjustedInfo );
		}
	}

	m_vecTotalBulletForce += adjustedInfo.GetDamageForce();
	
	gamestats->Event_PlayerDamage( this, adjustedInfo );

	return BaseClass::OnTakeDamage( adjustedInfo );
}

void CHL2MP_Player::DeathSound( const CTakeDamageInfo &info )
{
	if ( m_hRagdoll && m_hRagdoll->GetBaseAnimating()->IsDissolving() )
		 return;

	char szStepSound[128];

	Q_snprintf( szStepSound, sizeof( szStepSound ), "%s.Die", GetPlayerModelSoundPrefix() );

	const char *pModelName = STRING( GetModelName() );

	CSoundParameters params;
	if ( GetParametersForSound( szStepSound, params, pModelName ) == false )
		return;

	Vector vecOrigin = GetAbsOrigin();
	
	CRecipientFilter filter;
	filter.AddRecipientsByPAS( vecOrigin );

	EmitSound_t ep;
	ep.m_nChannel = params.channel;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume = params.volume;
	ep.m_SoundLevel = params.soundlevel;
	ep.m_nFlags = 0;
	ep.m_nPitch = params.pitch;
	ep.m_pOrigin = &vecOrigin;

	EmitSound( filter, entindex(), ep );
}

CBaseEntity* CHL2MP_Player::EntSelectSpawnPoint( void )
{
	CBaseEntity *pSpot = NULL;
	CBaseEntity *pLastSpawnPoint = g_pLastSpawn;
	edict_t		*player = edict();
	const char *pSpawnpointName = "info_player_deathmatch";

	if ( HL2MPRules()->IsTeamplay() == true )
	{
		if ( GetTeamNumber() == TEAM_COMBINE )
		{
			pSpawnpointName = "info_player_combine";
			pLastSpawnPoint = g_pLastCombineSpawn;
		}
		else if ( GetTeamNumber() == TEAM_REBELS )
		{
			pSpawnpointName = "info_player_rebel";
			pLastSpawnPoint = g_pLastRebelSpawn;
		}

		if ( gEntList.FindEntityByClassname( NULL, pSpawnpointName ) == NULL )
		{
			pSpawnpointName = "info_player_deathmatch";
			pLastSpawnPoint = g_pLastSpawn;
		}
	}

	pSpot = pLastSpawnPoint;
	// Randomize the start spot
	for ( int i = random->RandomInt(1,5); i > 0; i-- )
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
	if ( !pSpot )  // skip over the null point
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );

	CBaseEntity *pFirstSpot = pSpot;

	do 
	{
		if ( pSpot )
		{
			// check if pSpot is valid
			if ( g_pGameRules->IsSpawnPointValid( pSpot, this ) )
			{
				if ( pSpot->GetLocalOrigin() == vec3_origin )
				{
					pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
					continue;
				}

				// if so, go to pSpot
				goto ReturnSpot;
			}
		}
		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
	} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

	// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
	if ( pSpot )
	{
		CBaseEntity *ent = NULL;
		for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 128 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
		{
			// if ent is a client, kill em (unless they are ourselves)
			if ( ent->IsPlayer() && !(ent->edict() == player) )
				ent->TakeDamage( CTakeDamageInfo( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 300, DMG_GENERIC ) );
		}
		goto ReturnSpot;
	}

	if ( !pSpot  )
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_start" );

		if ( pSpot )
			goto ReturnSpot;
	}

ReturnSpot:

	if ( HL2MPRules()->IsTeamplay() == true )
	{
		if ( GetTeamNumber() == TEAM_COMBINE )
		{
			g_pLastCombineSpawn = pSpot;
		}
		else if ( GetTeamNumber() == TEAM_REBELS ) 
		{
			g_pLastRebelSpawn = pSpot;
		}
	}

	g_pLastSpawn = pSpot;

	m_flSlamProtectTime = gpGlobals->curtime + 0.5;

	return pSpot;
} 


CON_COMMAND( timeleft, "prints the time remaining in the match" )
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );

	int iTimeRemaining = (int)HL2MPRules()->GetMapRemainingTime();
    
	if ( iTimeRemaining == 0 )
	{
		if ( pPlayer )
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "This game has no timelimit." );
		}
		else
		{
			Msg( "* No Time Limit *\n" );
		}
	}
	else
	{
		int iMinutes, iSeconds;
		iMinutes = iTimeRemaining / 60;
		iSeconds = iTimeRemaining % 60;

		char minutes[8];
		char seconds[8];

		Q_snprintf( minutes, sizeof(minutes), "%d", iMinutes );
		Q_snprintf( seconds, sizeof(seconds), "%2.2d", iSeconds );

		if ( pPlayer )
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "Time left in map: %s1:%s2", minutes, seconds );
		}
		else
		{
			Msg( "Time Remaining:  %s:%s\n", minutes, seconds );
		}
	}	
}


void CHL2MP_Player::Reset()
{	
	ResetDeathCount();
	ResetFragCount();

	m_iExp = 0;
	m_iLevel = 1;
	m_iMoney = 1000;
	m_iSkillPoints = 1;
	m_iStatLevel = 0;
	m_iHasPistol = 0;
	m_iHasSMG = 0;
	m_iHasAR2 = 0;
	m_iHasBuckshot = 0;
	m_iHas357 = 0;
	m_iHasXBow = 0;
	m_iHasPhysCannon = 0;	
}

bool CHL2MP_Player::IsReady()
{
	return m_bReady;
}

void CHL2MP_Player::SetReady( bool bReady )
{
	m_bReady = bReady;
}

void CHL2MP_Player::CheckChatText( char *p, int bufsize )
{
	//Look for escape sequences and replace

	char *buf = new char[bufsize];
	int pos = 0;

	// Parse say text for escape sequences
	for ( char *pSrc = p; pSrc != NULL && *pSrc != 0 && pos < bufsize-1; pSrc++ )
	{
		// copy each char across
		buf[pos] = *pSrc;
		pos++;
	}

	buf[pos] = '\0';

	// copy buf back into p
	Q_strncpy( p, buf, bufsize );

	delete[] buf;	

	const char *pReadyCheck = p;

	HL2MPRules()->CheckChatForReadySignal( this, pReadyCheck );
}

void CHL2MP_Player::State_Transition( HL2MPPlayerState newState )
{
	State_Leave();
	State_Enter( newState );
}


void CHL2MP_Player::State_Enter( HL2MPPlayerState newState )
{
	m_iPlayerState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
		(this->*m_pCurStateInfo->pfnEnterState)();
}


void CHL2MP_Player::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}


void CHL2MP_Player::State_PreThink()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnPreThink )
	{
		(this->*m_pCurStateInfo->pfnPreThink)();
	}
}


CHL2MPPlayerStateInfo *CHL2MP_Player::State_LookupInfo( HL2MPPlayerState state )
{
	// This table MUST match the 
	static CHL2MPPlayerStateInfo playerStateInfos[] =
	{
		{ STATE_ACTIVE,			"STATE_ACTIVE",			&CHL2MP_Player::State_Enter_ACTIVE, NULL, &CHL2MP_Player::State_PreThink_ACTIVE },
		{ STATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CHL2MP_Player::State_Enter_OBSERVER_MODE,	NULL, &CHL2MP_Player::State_PreThink_OBSERVER_MODE }
	};

	for ( int i=0; i < ARRAYSIZE( playerStateInfos ); i++ )
	{
		if ( playerStateInfos[i].m_iPlayerState == state )
			return &playerStateInfos[i];
	}

	return NULL;
}

bool CHL2MP_Player::StartObserverMode(int mode)
{
	//we only want to go into observer mode if the player asked to, not on a death timeout
	if ( m_bEnterObserver == true )
	{
		VPhysicsDestroyObject();
		return BaseClass::StartObserverMode( mode );
	}
	return false;
}

void CHL2MP_Player::StopObserverMode()
{
	m_bEnterObserver = false;
	BaseClass::StopObserverMode();
}

void CHL2MP_Player::State_Enter_OBSERVER_MODE()
{
	int observerMode = m_iObserverLastMode;
	if ( IsNetClient() )
	{
		const char *pIdealMode = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_spec_mode" );
		if ( pIdealMode )
		{
			observerMode = atoi( pIdealMode );
			if ( observerMode <= OBS_MODE_FIXED || observerMode > OBS_MODE_ROAMING )
			{
				observerMode = m_iObserverLastMode;
			}
		}
	}
	m_bEnterObserver = true;
	StartObserverMode( observerMode );
}

void CHL2MP_Player::State_PreThink_OBSERVER_MODE()
{
	// Make sure nobody has changed any of our state.
	//	Assert( GetMoveType() == MOVETYPE_FLY );
	Assert( m_takedamage == DAMAGE_NO );
	Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );
	//	Assert( IsEffectActive( EF_NODRAW ) );

	// Must be dead.
	Assert( m_lifeState == LIFE_DEAD );
	Assert( pl.deadflag );
}


void CHL2MP_Player::State_Enter_ACTIVE()
{
	SetMoveType( MOVETYPE_WALK );
	
	// md 8/15/07 - They'll get set back to solid when they actually respawn. If we set them solid now and mp_forcerespawn
	// is false, then they'll be spectating but blocking live players from moving.
	// RemoveSolidFlags( FSOLID_NOT_SOLID );
	
	m_Local.m_iHideHUD = 0;
}


void CHL2MP_Player::State_PreThink_ACTIVE()
{
	//we don't really need to do anything here. 
	//This state_prethink structure came over from CS:S and was doing an assert check that fails the way hl2dm handles death
}

bool CHL2MP_Player::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
	// can always hear the console unless we're ignoring all chat
	if ( !pPlayer )
		return false;

	return true;
}
