#include "cbase.h"

#include "datacache/imdlcache.h"
#include "ai_basenpc.h"
#include "CreepMaker.h"
#include "ai_behavior_creep.h"
#include "ai_behavior_standoff.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

struct AI_StandoffParams_t;

static const char *g_ppszRandomHeads[] = 
{
	"male_01.mdl",
	"male_02.mdl",
	"female_01.mdl",
	"male_03.mdl",
	"female_02.mdl",
	"male_04.mdl",
	"female_03.mdl",
	"male_05.mdl",
	"female_04.mdl",
	"male_06.mdl",
	"female_06.mdl",
	"male_07.mdl",
	"female_07.mdl",
	"male_08.mdl",
	"male_09.mdl",
};

static void DispatchActivate( CBaseEntity *pEntity )
{
	bool bAsyncAnims = mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, false );
	pEntity->Activate();
	mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, bAsyncAnims );
}

LINK_ENTITY_TO_CLASS( npc_creep_maker, CreepMaker );

BEGIN_DATADESC( CreepMaker )

	DEFINE_KEYFIELD( m_iTeamNum,				FIELD_INTEGER,	"Team" ),
	DEFINE_KEYFIELD( m_nMaxLiveChildren,		FIELD_INTEGER,	"MaxLiveChildren" ),
	DEFINE_KEYFIELD( m_flSpawnFrequency,		FIELD_FLOAT,	"SpawnFrequency" ),
	DEFINE_KEYFIELD( m_spawnEquipment,			FIELD_STRING,	"additionalequipment" ),

	DEFINE_THINKFUNC( MakerThink ),

	DEFINE_KEYFIELD( m_flRadius,				FIELD_FLOAT,	"radius" ),
	DEFINE_KEYFIELD( m_nGroupSize,				FIELD_INTEGER,	"GroupSize" ),

	DEFINE_KEYFIELD( m_AssaultPointName,		FIELD_STRING,	"FirstWaypointName" ),

END_DATADESC()

CreepMaker::CreepMaker(void)
{
	m_spawnEquipment = NULL_STRING;
	m_GroupCount = 0;
	m_enabled = true;
}

void CreepMaker::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( "models/combine_soldier.mdl" );

	for ( int i = 0; i < ARRAYSIZE(g_ppszRandomHeads); i++ )
	{
		PrecacheModel( CFmtStr( "models/Humans/Group03/%s", g_ppszRandomHeads[i] ) );
	}

	UTIL_PrecacheOther( "npc_creep" );
}

void CreepMaker::Spawn( void )
{
	SetSolid( SOLID_NONE );
	m_nLiveChildren		= 0;
	Precache();

	SetThink ( &CreepMaker::MakerThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

bool CreepMaker::HumanHullFits( const Vector &vecLocation )
{
	EHANDLE m_hIgnoreEntity;

	trace_t tr;
	UTIL_TraceHull( vecLocation,
					vecLocation + Vector( 0, 0, 1 ),
					NAI_Hull::Mins(HULL_HUMAN),
					NAI_Hull::Maxs(HULL_HUMAN),
					MASK_NPCSOLID,
					m_hIgnoreEntity,
					COLLISION_GROUP_NONE,
					&tr );

	if( tr.fraction == 1.0 )
		return true;

	return false;
}

bool CreepMaker::CanMakeNPC( bool bIgnoreSolidEntities )
{
	if ( m_nLiveChildren >= m_nMaxLiveChildren || !m_enabled )
	{// not allowed to make a new one yet. Too many live ones out right now.
		return false;
	}

	CHL2MPRules *rules = (CHL2MPRules*)g_pGameRules;
	if ( !rules || !rules->GameStarted() )
	{
		return false;
	}

	return true;
}

void CreepMaker::MakerThink ( void )
{
	CheckConfig();
	
	SetNextThink( gpGlobals->curtime + m_flSpawnFrequency );

	if ( !CanMakeNPC(true))
		return;

	MakeMultipleNPCS( m_nGroupSize );
}

// Issue#3: AMP - 2013-09-19 - Server variables
ConVar creepGroupSize( "sv_creepGroupSize", "2", FCVAR_SERVER_CAN_EXECUTE | FCVAR_NOTIFY, "Creep group size", NULL );
ConVar creepSpawnFrequency( "sv_creepSpawnFrequency", "30", FCVAR_SERVER_CAN_EXECUTE | FCVAR_NOTIFY, "Creep spawn frequency", NULL );
ConVar creepMaxLiveChildren( "sv_creepMaxLiveChildren", "10", FCVAR_SERVER_CAN_EXECUTE | FCVAR_NOTIFY, "Creep max live children", NULL );
//ConVar creepEquipment( "sv_creepEquipment", "SMG1", FCVAR_SERVER_CAN_EXECUTE | FCVAR_NOTIFY, "Creep equipment", NULL ); // Currently only weapon_smg1 (default), and weapon_ar2 work

/** Check cvars for changes to configuration: sv_creepGroupSize, sv_creepSpawnFrequency, sv_creepMaxLiveChildren */
void CreepMaker::CheckConfig ( void ) {
	
	int tempCreepSpawnFrequency = creepSpawnFrequency.GetInt();
	if (tempCreepSpawnFrequency > 0 && tempCreepSpawnFrequency != m_flSpawnFrequency )
		m_flSpawnFrequency = tempCreepSpawnFrequency;

	int tempCreepGroupSize = creepGroupSize.GetInt();
	if (tempCreepGroupSize > 0 && tempCreepGroupSize != m_nGroupSize )
		m_nGroupSize = tempCreepGroupSize;

	int tempCreepMaxLiveChildren = creepMaxLiveChildren.GetInt();
	if (tempCreepMaxLiveChildren > 0 && tempCreepMaxLiveChildren != m_nMaxLiveChildren )
		m_nMaxLiveChildren = tempCreepMaxLiveChildren;

	//const char * tempCreepEquipment = creepEquipment.GetString();
	//if (m_spawnEquipment.ToCStr()!=tempCreepEquipment && tempCreepEquipment!=NULL && strlen(tempCreepEquipment)>0)
	//	m_spawnEquipment = MAKE_STRING(tempCreepEquipment); // Actually assign to cvar

}

void CreepMaker::DeathNotice( CBaseEntity *pVictim )
{
	// ok, we've gotten the deathnotice from our child
	m_nLiveChildren--;

	// Issue#7: JSM - 2013-10-06 - tracking of creep entities in client
	IGameEvent *pEvent = gameeventmanager->CreateEvent( "creep_death" );
	if ( pEvent )
	{
		pEvent->SetInt( "entindex", pVictim->entindex() );
		gameeventmanager->FireEvent( pEvent );
	}

	// If we're here, we're getting erroneous death messages from children we haven't created
	AssertMsg( m_nLiveChildren >= 0, "CreepMaker receiving child death notice but thinks has no children\n" );	
}

void CreepMaker::MakeNPCInRadius( void )
{
	CAI_BaseNPC	*pent = (CAI_BaseNPC*)CreateEntityByName( "npc_creep" );

	if ( !pent )
	{
		Warning("NULL Ent in CreepMaker!\n" );
		return;
	}
	
	if ( !PlaceNPCInRadius( pent ) )
	{
		// Failed to place the NPC. Abort
		UTIL_RemoveImmediate( (CBaseEntity *)pent );
		return;
	}

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
	pent->AddSpawnFlags( SF_NPC_FADE_CORPSE );

	pent->m_spawnEquipment	= m_spawnEquipment;
	pent->SetSquadName( AllocPooledString( CFmtStr( "%s_%i_%i", this->m_AssaultPointName, this->GetTeamNumber(), this->m_GroupCount ) ) );
	pent->ChangeTeam( this->GetTeamNumber() );

	CAI_CreepBehavior *pBehavior;
	if ( pent->GetBehavior( &pBehavior ) )
		pBehavior->InitializeBehavior( m_AssaultPointName );

	CAI_StandoffBehavior *pBehaviorStandoff;
	if ( pent->GetBehavior( &pBehaviorStandoff ) )
	{
		AI_StandoffParams_t params;
		params.fCoverOnReload = true;
		params.fPlayerIsBattleline = false;
		params.minShots = 4.0;
		params.maxTimeShots = 8.0;
		params.minShots = 2;
		params.maxShots = 4;
		params.oddsCover = 50;
		params.fStayAtCover = false;
		params.flAbandonTimeLimit = 0;
		params.hintChangeReaction = AIHCR_DEFAULT_AI;
		
		/*pBehaviorStandoff->SetParameters( params, this );
		pBehaviorStandoff->OnChangeTacticalConstraints();
		pBehaviorStandoff->SetActive( true );*/
	}

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );
	DispatchActivate( pent );

	// Issue#7: JSM - 2013-10-06 - tracking of creep entities in client
	IGameEvent *pEvent = gameeventmanager->CreateEvent( "creep_spawn" );
	if ( pEvent )
	{
		pEvent->SetInt( "entindex", pent->entindex() );
		gameeventmanager->FireEvent( pEvent );
	}

	m_nLiveChildren++;// count this NPC	
}

//-----------------------------------------------------------------------------
// Purpose: Find a place to spawn an npc within my radius.
//			Right now this function tries to place them on the perimeter of radius.
// Output : false if we couldn't find a spot!
//-----------------------------------------------------------------------------
bool CreepMaker::PlaceNPCInRadius( CAI_BaseNPC *pNPC )
{
	Vector vPos;

	if ( CAI_BaseNPC::FindSpotForNPCInRadius( &vPos, GetAbsOrigin(), pNPC, m_flRadius ) )
	{
		pNPC->SetAbsOrigin( vPos );
		return true;
	}

	DevMsg("**Failed to place NPC in radius!\n");
	return false;
}

void CreepMaker::MakeMultipleNPCS( int nNPCs )
{
	while ( nNPCs-- )
	{
		MakeNPCInRadius();
	}
	m_GroupCount++;
}

