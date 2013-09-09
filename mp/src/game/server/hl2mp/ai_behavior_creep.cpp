#include "cbase.h"
#include "ai_behavior_creep.h"

#include "team.h"
#include "hl2mp_gamerules.h"
#include "npc_combine.h"
#include "dotaObjective.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar dota_debug_creep("dota_debug_creep", "0");

BEGIN_DATADESC( CCreepWaypoint )
	DEFINE_KEYFIELD( m_NextAssaultPointName, FIELD_STRING, "nextwaypoint" ),
	DEFINE_FIELD( m_flTimeLastUsed, FIELD_TIME ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( npc_creep_waypoint, CCreepWaypoint );

CAI_CreepBehavior::CAI_CreepBehavior(void)
{
	m_bHitAssaultPoint = false;

	m_bDiverting = false;
	m_flLastSawAnEnemyAt = 0;
}

void CAI_CreepBehavior::InitializeBehavior( string_t pFirstWaypoint )
{
	m_AssaultPointName = pFirstWaypoint;

	CCreepWaypoint *pAssaultEnt = FindAssaultPoint( m_AssaultPointName );

	if( pAssaultEnt )
	{
		SetAssaultPoint(pAssaultEnt);
	}
	else
	{
		DevMsg("**ERROR: Can't find any assault points named: %s\n", STRING( m_AssaultPointName ));

		// Bomb out of assault behavior.
		SetAssaultPoint(pAssaultEnt);
		ClearSchedule( "Can't find assault point" );
		return;
	}

	// Slam the NPC's schedule so that he starts picking Assault schedules right now.
	ClearSchedule( "Initializing creep behavior" );
}

bool CAI_CreepBehavior::CanSelectSchedule()
{
	if ( !m_hAssaultPoint )
		return false;

	if ( !GetOuter()->IsInterruptable() )
		return false;

	if ( GetOuter()->HasCondition( COND_RECEIVED_ORDERS ) )
		return false;

	// We're letting other AI run for a little while because the assault AI failed recently.
	if ( m_flTimeDeferScheduleSelection > gpGlobals->curtime )
		return false;

	// If we've seen an enemy in the last few seconds, and we're allowed to divert,
	// let the base AI decide what I should do.
	if ( GetEnemy() )
	{
		// Return true, but remember that we're actually allowing them to divert
		// This is done because we don't want the assault behaviour to think it's finished with the assault.
		m_bDiverting = true;
	}
	else if ( m_bDiverting )
	{
		// If we were diverting, provoke us to make a new schedule selection
		SetCondition( COND_PROVOKED );

		m_bDiverting = false;
	}

	// If we're diverting, let the base AI decide everything
	if ( m_bDiverting )
		return false;

	return true;
}

CCreepWaypoint *CAI_CreepBehavior::FindAssaultPoint( string_t iszAssaultPointName )
{
	CUtlVector<CCreepWaypoint*>pAssaultPoints;
	CUtlVector<CCreepWaypoint*>pClearAssaultPoints;

	CCreepWaypoint *pAssaultEnt = (CCreepWaypoint *)gEntList.FindEntityByName( NULL, iszAssaultPointName );

	while( pAssaultEnt != NULL )
	{
		pAssaultPoints.AddToTail( pAssaultEnt );
		pAssaultEnt = (CCreepWaypoint *)gEntList.FindEntityByName( pAssaultEnt, iszAssaultPointName );
	}

	// Didn't find any?!
	if( pAssaultPoints.Count() < 1 )
		return NULL;

	// Only found one, just return it.
	if( pAssaultPoints.Count() == 1 )
		return pAssaultPoints[0];

	// Throw out any nodes that I cannot fit my bounding box on.
	for( int i = 0 ; i < pAssaultPoints.Count() ; i++ )
	{
		trace_t tr;
		CAI_BaseNPC *pNPC = GetOuter();
		CCreepWaypoint *pAssaultPoint = pAssaultPoints[i];

		AI_TraceHull ( pAssaultPoint->GetAbsOrigin(), pAssaultPoint->GetAbsOrigin(), pNPC->WorldAlignMins(), pNPC->WorldAlignMaxs(), MASK_SOLID, pNPC, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1.0 )
		{
			// Copy this into the list of clear points.
			pClearAssaultPoints.AddToTail(pAssaultPoint);
		}
	}

	// Only one clear assault point left!
	if( pClearAssaultPoints.Count() == 1 )
		return pClearAssaultPoints[0];

	// NONE left. Just return a random assault point, knowing that it's blocked. This is the old behavior, anyway.
	if( pClearAssaultPoints.Count() < 1 )
		return pAssaultPoints[ random->RandomInt(0, (pAssaultPoints.Count() - 1)) ];

	// We found several! First throw out the one most recently used.
	// This prevents picking the same point at this branch twice in a row.
	float flMostRecentTime = -1.0f; // Impossibly old
	int iMostRecentIndex = -1;
	for( int i = 0 ; i < pClearAssaultPoints.Count() ; i++ )
	{
		if( pClearAssaultPoints[i]->m_flTimeLastUsed > flMostRecentTime )
		{
			flMostRecentTime = pClearAssaultPoints[i]->m_flTimeLastUsed;
			iMostRecentIndex = i;
		}
	}

	Assert( iMostRecentIndex > -1 );

	// Remove the most recently used 
	pClearAssaultPoints.Remove( iMostRecentIndex );
	return pClearAssaultPoints[ random->RandomInt(0, (pClearAssaultPoints.Count() - 1)) ];
}

void CAI_CreepBehavior::SetAssaultPoint( CCreepWaypoint *pAssaultPoint )
{
	m_hAssaultPoint = pAssaultPoint;
	if ( pAssaultPoint )
		pAssaultPoint->m_flTimeLastUsed = gpGlobals->curtime;
}

int CAI_CreepBehavior::TranslateSchedule( int scheduleType )
{	
	switch( scheduleType )
	{
	case SCHED_CREEP_MOVE_TO_ASSAULT_POINT:
		{
		float flDist = ( m_hAssaultPoint->GetAbsOrigin() - GetAbsOrigin() ).Length();
		if ( flDist <= 12.0f )
			return SCHED_CREEP_AT_ASSAULT_POINT;
		}
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

void CAI_CreepBehavior::ClearSchedule( const char *szReason )
{
	GetOuter()->ClearSchedule( szReason );
}

int CAI_CreepBehavior::SelectSchedule()
{
	if( HasCondition( COND_PLAYER_PUSHING ) )
		return SCHED_CREEP_MOVE_AWAY; 

	if( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
		return SCHED_MELEE_ATTACK1;

	// If you're empty, reload before trying to carry out any assault functions.
	if( HasCondition( COND_NO_PRIMARY_AMMO ) )
		return SCHED_RELOAD;

	if( !m_bHitAssaultPoint )
	{
		GetOuter()->SpeakSentence( ASSAULT_SENTENCE_SQUAD_ADVANCE_TO_ASSAULT );
		return SCHED_CREEP_MOVE_TO_ASSAULT_POINT;
	}

	return BaseClass::SelectSchedule();
}

void CAI_CreepBehavior::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		BaseClass::StartTask( pTask );
		break;

	case TASK_CREEP_DEFER_SCHEDULE_SELECTION:
		m_flTimeDeferScheduleSelection = gpGlobals->curtime + pTask->flTaskData;
		TaskComplete();
		break;

	case TASK_CREEP_ANNOUNCE_CLEAR:
		ClearAssaultPoint();
		TaskComplete();
		break;

	case TASK_CREEP_GET_PATH_TO_ASSAULT_POINT:
		{
			AI_NavGoal_t goal( m_hAssaultPoint->GetAbsOrigin() );
			goal.pTarget = m_hAssaultPoint;
			if ( GetNavigator()->SetGoal( goal ) == false )
			{
				// Try and get as close as possible otherwise
				AI_NavGoal_t nearGoal( GOALTYPE_LOCATION_NEAREST_NODE, m_hAssaultPoint->GetAbsOrigin(), AIN_DEF_ACTIVITY, 256 );
				if ( GetNavigator()->SetGoal( nearGoal, AIN_CLEAR_PREVIOUS_STATE ) )
				{
					//FIXME: HACK! The internal pathfinding is setting this without our consent, so override it!
					ClearCondition( COND_TASK_FAILED );
					GetNavigator()->SetArrivalDirection( m_hAssaultPoint->GetAbsAngles() );
					TaskComplete();
					return;
				}
			}
			GetNavigator()->SetArrivalDirection( m_hAssaultPoint->GetAbsAngles() );
		}
		break;

	case TASK_CREEP_FACE_ASSAULT_POINT:
		{
			if( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			{
				// If I can already fight when I arrive, don't bother running any facing code. Let
				// The combat AI do that. Turning here will only make the NPC look dumb in a combat
				// situation because it will take time to turn before attacking.
				TaskComplete();
			}
			else
			{
				GetMotor()->SetIdealYaw( m_hAssaultPoint->GetAbsAngles().y );
				GetOuter()->SetTurnActivity(); 
			}
		}
		break;

	case TASK_CREEP_HIT_ASSAULT_POINT:
		OnHitAssaultPoint();
		TaskComplete();
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

void CAI_CreepBehavior::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_CREEP_FACE_ASSAULT_POINT:
		GetMotor()->UpdateYaw();

		if( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		{
			// Out early if the NPC can attack.
			TaskComplete();
		}

		if ( GetOuter()->FacingIdeal() )
		{
			TaskComplete();
		}
		break;
	case TASK_WAIT_FOR_MOVEMENT:
		if ( dota_debug_creep.GetBool() )
		{
			if ( IsCurSchedule( SCHED_CREEP_MOVE_TO_ASSAULT_POINT ) )
			{
				NDebugOverlay::Line( WorldSpaceCenter(), GetNavigator()->GetGoalPos(), 255,0,0, true,0.1);
				NDebugOverlay::Box( GetNavigator()->GetGoalPos(), -Vector(10,10,10), Vector(10,10,10), 255,0,0, 8, 0.1 );
			}
		}

		BaseClass::RunTask( pTask );
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

void CAI_CreepBehavior::ClearAssaultPoint( void )
{
	// keep track of the name of the assault point
	m_AssaultPointName = NULL_STRING;	

	if( GetOuter()->GetTeamNumber() == TEAM_REBELS )
	{
		//The rebel team goes through the points in reverse order...
		CCreepWaypoint *pPoint = NULL;
		while( (pPoint = dynamic_cast< CCreepWaypoint * >(gEntList.FindEntityByClassname( pPoint, "npc_creep_waypoint" ))) != NULL )
		{
			if( pPoint->m_NextAssaultPointName == m_hAssaultPoint.Get()->GetEntityName() )
			{
				m_AssaultPointName = pPoint->GetEntityName();
			}
		}
	}
	else
	{
		m_AssaultPointName = m_hAssaultPoint->m_NextAssaultPointName;
	}

	// Do we need to move to another assault point?
	if( m_AssaultPointName != NULL_STRING )
	{
		CCreepWaypoint *pNextPoint = FindAssaultPoint( m_AssaultPointName );
		
		if( pNextPoint )
		{
			SetAssaultPoint( pNextPoint );
			
			// Send our NPC to the next assault point!
			m_bHitAssaultPoint = false;

			return;
		}
		else
		{
			Warning("**ERROR: Can't find next assault point: %s\n", m_AssaultPointName );

			// Bomb out of assault behavior.
			ClearSchedule( "Can't find next assault point" );
			return;
		}
	}
	else
	{
		DotaObjective *obj = NULL;
		while( (obj = dynamic_cast< DotaObjective * >(gEntList.FindEntityByClassname( obj, "dota_objective" ))) != NULL )
		{
			if( !obj->m_bMet )
			{
				break;
			}
		}

		if(obj && !obj->m_bMet )
		{
			CNPC_Combine * combine = (CNPC_Combine*)GetOuter();
			if ( combine )
			{
				combine->m_hForcedGrenadeTarget = obj;
				combine->m_flNextGrenadeCheck = 0;
				combine->ClearSchedule( "Told to throw grenade via ai_behavior_creep" );
			}			
			SetAssaultPoint( m_hAssaultPoint );
			m_bHitAssaultPoint = false;
		}
		else
		{
			//Can't find an unmet objective, the game should be over?
			CNPC_Combine * combine = (CNPC_Combine*)GetOuter();
			if ( combine )
			{
				inputdata_t x;
				combine->InputStartPatrolling( x );			
			}
			SetAssaultPoint(NULL);
		}
	}
}

void CAI_CreepBehavior::OnHitAssaultPoint( void )
{
	GetOuter()->SpeakSentence( ASSAULT_SENTENCE_HIT_ASSAULT_POINT );
	m_bHitAssaultPoint = true;
}

int CAI_CreepBehavior::DrawDebugTextOverlays( int text_offset )
{
	char	tempstr[ 512 ];
	int		offset;

	offset = BaseClass::DrawDebugTextOverlays( text_offset );
	if ( GetOuter()->m_debugOverlays & OVERLAY_TEXT_BIT )
	{	
		Q_snprintf( tempstr, sizeof(tempstr), "Creep Waypoint: %s %s", STRING( m_AssaultPointName ), VecToString( m_hAssaultPoint->GetAbsOrigin() ) );
		GetOuter()->EntityText( offset, tempstr, 0 );
		offset++;
	}

	return offset;
}

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER(CAI_CreepBehavior)

	DECLARE_TASK(TASK_CREEP_GET_PATH_TO_ASSAULT_POINT)
	DECLARE_TASK(TASK_CREEP_FACE_ASSAULT_POINT)
	DECLARE_TASK(TASK_CREEP_ANNOUNCE_CLEAR)
	DECLARE_TASK(TASK_CREEP_HIT_ASSAULT_POINT)
	DECLARE_TASK(TASK_CREEP_DEFER_SCHEDULE_SELECTION)

	DEFINE_SCHEDULE
	(
		SCHED_CREEP_FAILED_TO_MOVE,

		"	Tasks"
		"		TASK_CREEP_DEFER_SCHEDULE_SELECTION	1"
		"	"
		"	Interrupts"
	)

	DEFINE_SCHEDULE 
	(
		SCHED_CREEP_MOVE_TO_ASSAULT_POINT,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE					SCHEDULE:SCHED_CREEP_FAILED_TO_MOVE"
		"		TASK_GATHER_CONDITIONS					0"
		"		TASK_CREEP_GET_PATH_TO_ASSAULT_POINT			0"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		"		TASK_CREEP_FACE_ASSAULT_POINT					0"
		"		TASK_CREEP_HIT_ASSAULT_POINT					0"
		"		TASK_CREEP_ANNOUNCE_CLEAR						0"
		"	"
		"	Interrupts"
		"		COND_NO_PRIMARY_AMMO"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_LIGHT_DAMAGE"
	)

	DEFINE_SCHEDULE 
	(
		SCHED_CREEP_AT_ASSAULT_POINT,

		"	Tasks"
		"		TASK_CREEP_FACE_ASSAULT_POINT					0"
		"		TASK_CREEP_HIT_ASSAULT_POINT					0"
		"		TASK_CREEP_ANNOUNCE_CLEAR						0"
		"	"
		"	Interrupts"
		"		COND_NO_PRIMARY_AMMO"		
	)

	DEFINE_SCHEDULE 
	(
		SCHED_CREEP_MOVE_AWAY,

		"	Tasks"
		"		TASK_MOVE_AWAY_PATH						120"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		"	"
		"	Interrupts"
	)

AI_END_CUSTOM_SCHEDULE_PROVIDER()