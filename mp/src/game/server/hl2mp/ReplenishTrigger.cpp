#include "cbase.h"
#include "ReplenishTrigger.h"
#include "hl2mp_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CReplenishTrigger )
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_replenish, CReplenishTrigger );

void CReplenishTrigger::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}

void CReplenishTrigger::Touch( CBaseEntity *pOther )
{
	if ( m_pfnThink == NULL )
	{
		SetThink( &CReplenishTrigger::ReplenishThink );
		SetNextThink( gpGlobals->curtime );
	}
}

void CReplenishTrigger::ReplenishThink( void )
{
	if ( ReplenishAllTouchers( 0.5 ) <= 0 )
	{
		SetThink(NULL);
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.5f );
	}
}

int CReplenishTrigger::ReplenishAllTouchers( float dt )
{
	int replenishCount = 0;

	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch )
			{
				if ( ReplenishEntity( pTouch, dt ) )
				{
					replenishCount++;
				}
			}
		}
	}

	return replenishCount;
}

bool CReplenishTrigger::ReplenishEntity( CBaseEntity *pOther, float dt )
{
	if ( !pOther->m_takedamage || !PassesTriggerFilters(pOther) )
		return false;

	if ( pOther->GetTeamNumber() == this->GetTeamNumber() )
	{
		CHL2MP_Player * player = ToHL2MPPlayer( pOther );
		if ( player )
		{
			//player->Replenish();
		}
	}

	return true;
}

