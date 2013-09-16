#include "cbase.h"
#include "ItemMoney.h"
#include "hl2mp_player.h"

LINK_ENTITY_TO_CLASS(item_money, CItemMoney);
PRECACHE_REGISTER(item_money);

CItemMoney::CItemMoney()
{
	m_iAmount = 0;
	m_taker = NULL;
}

void CItemMoney::Spawn( void )
{ 
	Precache( );
	SetModel( MONEY_MODEL );
	BaseClass::Spawn( );
}
void CItemMoney::Precache( void )
{
	PrecacheModel ( MONEY_MODEL );

	PrecacheScriptSound( "Buttons.snd2" );
	PrecacheScriptSound( "Grenade_Molotov.Detonate" );
}
bool CItemMoney::MyTouch( CBasePlayer *pPlayer )
{
	if ( pPlayer != m_taker )
		return false;

	CPASAttenuationFilter sndFilter( pPlayer );
	pPlayer->EmitSound( sndFilter, pPlayer->entindex(), "Buttons.snd2" );

	CHL2MP_Player * p = ToHL2MPPlayer( pPlayer );
	p->AddMoney( m_iAmount );

	return true;
}

void DropMoney( const Vector &vecOrigin, int amount, CBasePlayer * pTaker )
{
	Vector offsetVec = RandomVector(-4.0, 4.0);
	offsetVec.z = abs(offsetVec.z);
	//Vector newOrigin = vecOrigin + RandomVector(-4,4);
	Vector newOrigin = vecOrigin + offsetVec;

	CItemMoney * money = (CItemMoney*)CBaseEntity::Create( "item_money", newOrigin, vec3_angle );
	if ( money )
	{
		CPASAttenuationFilter filter( pTaker );
		pTaker->EmitSound( filter, pTaker->entindex(), "Grenade_Molotov.Detonate" );		
		
		money->ChangeTeam( pTaker->GetTeamNumber() );
		money->m_iAmount = amount;
		money->m_taker = pTaker;

		//==================================================================================
		// ItemMoney is not a VPhysObject so, the following is actually dead code... for now
		//==================================================================================
		//IPhysicsObject *pPhysicsObject = money->VPhysicsGetObject();
		//if ( pPhysicsObject )
		//{
		//	Vector			vel		= RandomVector( -64.0f, 64.0f );
		//	vel.z = abs(vel.z);
		//	AngularImpulse	angImp	= RandomAngularImpulse( -300.0f, 300.0f );

		//	// Angular velocity is always applied in local space in vphysics
		//	AngularImpulse localAngImp;
		//	pPhysicsObject->WorldToLocalVector( &localAngImp, angImp );
		//	pPhysicsObject->AddVelocity( &vel, &localAngImp );
		//}
	}
}