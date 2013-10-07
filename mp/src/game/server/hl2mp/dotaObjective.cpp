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

ConVar dotaLaneMode( "sv_dotaLaneMode", "-1", FCVAR_GAMEDLL | FCVAR_NOTIFY ); // Issue #24: AMP - 2013-10-04 - Conditionally open lanes

DotaObjective::DotaObjective(void)
{
	m_timesHit = 0;
	m_bMet = false;
}

void DotaObjective::Spawn( )
{
	BaseClass::Spawn();
	if (CheckLaneMode()) {
		TakeAction(DOBJ_ACTION_OPEN);
	} else {
		TakeAction(DOBJ_ACTION_CLOSE);
	}
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
			Q_snprintf( szText, sizeof(szText), "Your ally gate is under attack from an enemy!" );
			UTIL_ClientPrintFilter( user, HUD_PRINTCENTER, szText );
			
			if( playerAttacker )
				ClientPrint( playerAttacker, HUD_PRINTTALK, UTIL_VarArgs("Gate has %i health left.\n", 30 - m_timesHit) );

			if (m_timesHit >= 30)
			{
				TakeAction(DOBJ_ACTION_CLOSE);
			}
		}
	}

	return 0;
}

// Ability to alter the objective:
//   DOBJ_ACTION_OPEN: Opens the objective door and starts spawning creeps (if round has started)
//   DOBJ_ACTION_CLOSE: Closes the objective door and stops spawning creeps (if this is the last door, the round will end)
int DotaObjective::TakeAction( int dobjAction ) { // Issue #24: AMP - 2013-10-04 - Lane manipulation
	if (dobjAction & DOBJ_ACTION_CLOSE) {
		PropSetAnim( "Close" );

		m_bMet = true;			
				
		//turn off the maker
		CreepMaker * maker = (CreepMaker*)gEntList.FindEntityByName( NULL, m_creepMakerName );
		if( maker )
			maker->m_enabled = false;

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
		return true;
	} else if (dobjAction & DOBJ_ACTION_OPEN) {
		m_timesHit = 0;
		m_bMet = false;
		CreepMaker * maker = (CreepMaker*)gEntList.FindEntityByName( NULL, m_creepMakerName );
		maker->m_enabled = true;
		PropSetAnim( "Open" );
		return true;
	}
	return false;
}

// Returns true if the bitmask for the specified lane was enabled. If no lanes were enabled, all will return true.
BOOL DotaObjective::CheckLaneMode( ) { // Issue #24: AMP - 2013-10-04 - Conditionally open lanes
	int laneMode = dotaLaneMode.GetInt();
	if (laneMode<0) { // Default allow all lanes
		return true;
	}
	const char * objectiveName = this->m_creepMakerName.ToCStr();
	if (objectiveName==NULL) {
		return false;
	}

	const char * laneToFind;
	if (laneMode & 1) {
		laneToFind = "center";
	} else if (laneMode & 2) {
		laneToFind = "left";
	} else if (laneMode & 4) {
		laneToFind = "right";
	} else if (laneMode & 8) {
		laneToFind = "other";
	} else {
		return false;
	}
	return strstr(objectiveName, laneToFind)!=NULL;
}
