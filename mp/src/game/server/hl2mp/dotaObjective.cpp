#include "cbase.h"

#include "datacache/imdlcache.h"
#include "dotaObjective.h"
#include "CreepMaker.h"
#include "hl2mp_gamerules.h"
#include "ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( dota_objective, DotaObjective );

BEGIN_DATADESC( DotaObjective )

	DEFINE_KEYFIELD( m_iTeamNum,				FIELD_INTEGER,	"Team" ),

	DEFINE_KEYFIELD( m_creepMakerName,		FIELD_STRING,	"CreepMaker" ),\

	DEFINE_KEYFIELD( m_guardianName,		FIELD_STRING,	"Guardian" ),
	
END_DATADESC()

DotaObjective::DotaObjective(void)
{
	m_timesHit = 0;
	m_bMet = false;
}

void DotaObjective::Spawn( )
{
	BaseClass::Spawn();

	PropSetAnim( "Open" );
}

int DotaObjective::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	if ( m_bMet )
		return 0;	

	if ( !(inputInfo.GetDamageType() & DMG_BULLET) )
	{
		CHL2MP_Player * playerAttacker = ToHL2MPPlayer( inputInfo.GetAttacker() );

		CreepMaker * maker = (CreepMaker*)gEntList.FindEntityByName( NULL, m_creepMakerName );
		if ( !maker )
			AssertMsg( false, "Objective can't find its creepmaker!\n" );
		
		CAI_BaseNPC * guardian = (CAI_BaseNPC*)gEntList.FindEntityByName( NULL, m_guardianName );	
		if( guardian && guardian->IsAlive() )
		{
			if( playerAttacker )
				ClientPrint( playerAttacker, HUD_PRINTTALK, UTIL_VarArgs("The guradian is alive in this lane, you can't hurt the gate.\n") );
		}
		else
		{
			m_timesHit++;

			CRecipientFilter user;
			user.AddRecipientsByTeam( this->GetTeam() );
			user.MakeReliable();
			char szText[200];
			Q_snprintf( szText, sizeof(szText), "Your aAlly gate is under attack from an enemy!" );
			UTIL_ClientPrintFilter( user, HUD_PRINTCENTER, szText );
			
			if( playerAttacker )
				ClientPrint( playerAttacker, HUD_PRINTTALK, UTIL_VarArgs("Gate has %i health left.\n", 30 - m_timesHit) );

			if (m_timesHit >= 30)
			{
				PropSetAnim( "Close" );

				m_bMet = true;			
				
				//turn off the maker
				if( maker )
					maker->m_nMaxLiveChildren = 0;

				//see if I'm the last one, if so, game over
				bool foundOne = false;
				for ( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt(pEntity) )
				{
					DotaObjective * obj = dynamic_cast<DotaObjective*>( pEntity );
					if ( obj && obj->GetTeamNumber() == this->GetTeamNumber() && !obj->m_bMet )
						foundOne = true;
				}
				if ( !foundOne )
				{
					CHL2MPRules *rules = (CHL2MPRules*)g_pGameRules;
					if ( rules && !rules->IsIntermission() )
					{
						rules->EndMultiplayerGame();
					}
				}
			}
		}
	}

	return 0;
}