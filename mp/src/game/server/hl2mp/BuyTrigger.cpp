#include "cbase.h"
#include "BuyTrigger.h"
#include "hl2mp_player.h"

BEGIN_DATADESC( CBuyTrigger )
END_DATADESC()

LINK_ENTITY_TO_CLASS( dota_shop_trigger, CBuyTrigger );

void CBuyTrigger::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();
}

void CBuyTrigger::StartTouch(CBaseEntity *pOther)
{
	CHL2MP_Player *player = ToHL2MPPlayer(pOther);
	if ( player )
	{
		player->SetCanShop( true );
	}
}
void CBuyTrigger::EndTouch(CBaseEntity *pOther)
{
	CHL2MP_Player *player = ToHL2MPPlayer(pOther);
	if ( player )
	{
		player->SetCanShop( false );
	}
}
