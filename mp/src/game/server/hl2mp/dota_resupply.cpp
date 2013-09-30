#include "cbase.h"
#include "dota_resupply.h"
#include "hl2mp_player.h"
#include "eventlist.h"
#include "npcevent.h"
#include "ammodef.h"
#include "gamerules.h"
#include "itemdef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( dota_resupply, Dota_Resupply );

BEGIN_DATADESC( Dota_Resupply )

	DEFINE_FIELD( m_flCloseTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_hActivator, FIELD_EHANDLE ),

	DEFINE_THINKFUNC( CrateThink ),

END_DATADESC()

#define	AMMO_CRATE_CLOSE_DELAY	1.5f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Dota_Resupply::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	SetModel( STRING( GetModelName() ) );
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_VPHYSICS );
	CreateVPhysics();

	ResetSequence( LookupSequence( "Idle" ) );
	SetBodygroup( 1, true );

	m_flCloseTime = gpGlobals->curtime;
	m_flAnimTime = gpGlobals->curtime;
	m_flPlaybackRate = 0.0;
	SetCycle( 0 );

	m_takedamage = DAMAGE_EVENTS_ONLY;
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool Dota_Resupply::CreateVPhysics( void )
{
	return ( VPhysicsInitStatic() != NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Dota_Resupply::Precache( void )
{
	SetModelName( AllocPooledString( "models/items/ammocrate_ar2.mdl" ) );

	PrecacheModel( STRING( GetModelName() ) );

	PrecacheScriptSound( "AmmoCrate.Open" );
	PrecacheScriptSound( "AmmoCrate.Close" );
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void Dota_Resupply::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( pActivator );

	if ( pPlayer == NULL )
		return;

	int iSequence = LookupSequence( "Open" );

	// See if we're not opening already
	if ( GetSequence() != iSequence )
	{
		Vector mins, maxs;
		trace_t tr;

		CollisionProp()->WorldSpaceAABB( &mins, &maxs );

		Vector vOrigin = GetAbsOrigin();
		vOrigin.z += ( maxs.z - mins.z );
		mins = (mins - GetAbsOrigin()) * 0.2f;
		maxs = (maxs - GetAbsOrigin()) * 0.2f;
		mins.z = ( GetAbsOrigin().z - vOrigin.z );  
		
		UTIL_TraceHull( vOrigin, vOrigin, mins, maxs, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.startsolid || tr.allsolid )
			 return;
			
		m_hActivator = pPlayer;
		m_hActivator->SetCanShop( true );

		// Animate!
		ResetSequence( iSequence );

		// Make sound
		CPASAttenuationFilter sndFilter( this, "AmmoCrate.Open" );
		EmitSound( sndFilter, entindex(), "AmmoCrate.Open" );

		// Start thinking to make it return
		SetThink( &Dota_Resupply::CrateThink );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}

	// Don't close again for two seconds
	m_flCloseTime = gpGlobals->curtime + AMMO_CRATE_CLOSE_DELAY;
}

//-----------------------------------------------------------------------------
// Purpose: allows the crate to open up when hit by a crowbar
//-----------------------------------------------------------------------------
int Dota_Resupply::OnTakeDamage( const CTakeDamageInfo &info )
{
	// if it's the player hitting us with a crowbar, open up
	CBasePlayer *player = ToBasePlayer(info.GetAttacker());
	if (player)
	{
		CBaseCombatWeapon *weapon = player->GetActiveWeapon();

		if (weapon && !stricmp(weapon->GetName(), "weapon_crowbar"))
		{
			// play the normal use sound
			player->EmitSound( "HL2Player.Use" );
			// open the crate
			Use(info.GetAttacker(), info.GetAttacker(), USE_TOGGLE, 0.0f);
		}
	}

	// don't actually take any damage
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific messages that occur when tagged
//			animation frames are played.
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void Dota_Resupply::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_AMMOCRATE_PICKUP_AMMO )
	{
		if ( m_hActivator )
		{
			bool gotSomething = Dota_Resupply::ReSupplyPlayer( m_hActivator );
			if ( gotSomething )
			{
				SetBodygroup( 1, false );
			}
			//m_hActivator = NULL;
		}
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Dota_Resupply::CrateThink( void )
{
	StudioFrameAdvance();
	DispatchAnimEvents( this );

	SetNextThink( gpGlobals->curtime + 0.1f );

	// Start closing if we're not already
	if ( GetSequence() != LookupSequence( "Close" ) )
	{
		// Not ready to close?
		if ( m_flCloseTime <= gpGlobals->curtime )
		{
			//m_hActivator = NULL;

			ResetSequence( LookupSequence( "Close" ) );
		}
	}
	else
	{
		// See if we're fully closed
		if ( IsSequenceFinished() )
		{
			// Stop thinking
			SetThink( NULL );
			CPASAttenuationFilter sndFilter( this, "AmmoCrate.Close" );
			EmitSound( sndFilter, entindex(), "AmmoCrate.Close" );

			// FIXME: We're resetting the sequence here
			// but setting Think to NULL will cause this to never have
			// StudioFrameAdvance called. What are the consequences of that?
			ResetSequence( LookupSequence( "Idle" ) );
			SetBodygroup( 1, true );

			m_hActivator->SetCanShop( false );
			m_hActivator = NULL;
		}
	}
}
bool Dota_Resupply::ReSupplyPlayer( CHL2MP_Player * pPlayer )
{
	CBaseCombatWeapon * weapon;
	int weaponLevel;
	int iAmmoIndex;
	bool gotSomething = false;
	
	for ( int i = 1; i < MAX_AMMO_TYPES; i++ )
	{
		Item_t * item = GetItemDef()->GetItemOfIndex(i);
		if ( item && item->pWeaponNeeded == NULL  )
		{
			weapon = pPlayer->Weapon_OwnsThisType( item->pName );
			weaponLevel = pPlayer->GetWeaponLevel( item->pName );
			if ( weapon && weaponLevel > 0 )
			{
				iAmmoIndex = weapon->GetPrimaryAmmoType();
				if ( iAmmoIndex < 0 || iAmmoIndex >= MAX_AMMO_SLOTS )
					continue;

				int iMax = (GetAmmoDef()->MaxCarry(iAmmoIndex) / 4) * weaponLevel;

				if ( weapon->UsesClipsForAmmo1() ) {
					int missingFromClip1 = weapon->GetMaxClip1() - weapon->Clip1();
					iMax += missingFromClip1;
				}
								
				int iAdd = iMax - pPlayer->GetAmmoCount(iAmmoIndex);
				if ( iAdd >= 1 )
					gotSomething |= (pPlayer->GiveAmmo( iAdd, weapon->GetPrimaryAmmoType() ) != 0);						
			}
		}
	}	
	return gotSomething;
}