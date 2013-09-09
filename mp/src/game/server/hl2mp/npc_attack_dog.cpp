#include "cbase.h"
#include "ai_baseactor.h"
#include "ai_behavior_follow.h"
#include "npcevent.h"
#include "props.h"
#include "vehicle_base.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
enum Attack_Dog_Schedules
{
	SCHED_DOG_CHASE_ENEMY = LAST_SHARED_SCHEDULE,
	SCHED_DOG_MELEE_ATTACK1,
	SCHED_DOG_POST_MELEE_WAIT,

	LAST_ATTACK_DOG_SCHEDULE,
};

enum Attack_Dog_Tasks
{
	TASK_DOG_WAIT_POST_MELEE = LAST_SHARED_TASK,

	LAST_ATTACK_DOG_TASK,
};

enum Attack_Dog_Conds
{
	COND_DOG_LOCAL_MELEE_OBSTRUCTION = LAST_SHARED_CONDITION,

	LAST_ATTACK_DOG_CONDITION,
};
extern int ACT_DOG_THROW;
extern int AE_DOG_THROW;
class CNPC_Attack_Dog : public CAI_BaseActor
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CNPC_Attack_Dog, CAI_BaseActor );
	Class_T Classify ( void ) { return CLASS_PLAYER_ALLY; }
	void Spawn( void );
	void Precache( void );
	int	 SelectSchedule( void );
	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	void HandleAnimEvent( animevent_t *pEvent );
	int TranslateSchedule( int scheduleType );
	void BuildScheduleTestBits( void );
	void GatherConditions( void );
	Activity NPC_TranslateActivity( Activity baseAct );

	int MeleeAttack1Conditions ( float flDot, float flDist );
	CBaseEntity *ClawAttack( float flDist, int iDamage, QAngle &qaViewPunch, Vector &vecVelocityPunch );
	virtual float GetClawAttackRange() const { return 75; }
	// No range attacks
	int RangeAttack1Conditions ( float flDot, float flDist ) { return( 0 ); }
	
	void AttackHitSound( void ) { EmitSound( "Zombie.AttackHit" ); }
	void AttackMissSound( void ) { EmitSound( "Zombie.AttackMiss" ); }

protected:
	virtual bool CreateBehaviors( void );
	CAI_FollowBehavior		m_FollowBehavior;

protected:

	DEFINE_CUSTOM_AI;
};
LINK_ENTITY_TO_CLASS( npc_attack_dog, CNPC_Attack_Dog );
BEGIN_DATADESC( CNPC_Attack_Dog )
END_DATADESC()
bool			CNPC_Attack_Dog::CreateBehaviors( void )
{
	AddBehavior( &m_FollowBehavior );

	return BaseClass::CreateBehaviors();
}

void			CNPC_Attack_Dog::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	SetModel( "models/dog.mdl" );

	SetHullType( HULL_WIDE_HUMAN );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_MECH );

	m_iHealth			= 1;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )	

	m_takedamage		= DAMAGE_YES;
	
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE | bits_CAP_INNATE_MELEE_ATTACK1 );

	NPCInit();

	m_NPCState			= NPC_STATE_ALERT;
}

void			CNPC_Attack_Dog::Precache( void )
{
	PrecacheModel( "models/dog.mdl" );
	
	PrecacheScriptSound( "Weapon_PhysCannon.Launch" );

	PrecacheModel( "sprites/orangelight1.vmt" );
	PrecacheModel( "sprites/physcannon_bluelight2.vmt" );
	PrecacheModel( "sprites/glow04_noz.vmt" );

	BaseClass::Precache();
}
int				CNPC_Attack_Dog::SelectSchedule ( void )
{
	if ( HasCondition(COND_CAN_MELEE_ATTACK1) )
		return SCHED_MELEE_ATTACK1;

	if ( BehaviorSelectSchedule() )
		return BaseClass::SelectSchedule();

	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		if ( HasCondition( COND_LOST_ENEMY ) || HasCondition( COND_ENEMY_UNREACHABLE ) )
		{
			// Set state to alert and recurse!
			SetState( NPC_STATE_ALERT );
			return SelectSchedule();
		}
		break;

	case NPC_STATE_ALERT:
		if ( HasCondition( COND_LOST_ENEMY ) || HasCondition( COND_ENEMY_UNREACHABLE ) )
		{
			ClearCondition( COND_LOST_ENEMY );
			ClearCondition( COND_ENEMY_UNREACHABLE );
			SetEnemy( NULL );
		}
		break;
	}

	return BaseClass::SelectSchedule();
}
void			CNPC_Attack_Dog::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_MELEE_ATTACK1:
		{
			ResetIdealActivity( (Activity)ACT_DOG_THROW );
		}
		break;

	case TASK_DOG_WAIT_POST_MELEE:
		{
			TaskComplete();
			return;			
		}
		break;

	default:
		BaseClass::StartTask( pTask );
	}
}
void			CNPC_Attack_Dog::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_DOG_WAIT_POST_MELEE:
		{
			if ( IsWaitFinished() )
			{
				TaskComplete();
			}
		}
		break;
	default:
		BaseClass::RunTask( pTask );
		break;
	}
}
void			CNPC_Attack_Dog::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_DOG_THROW )
	{
		Vector right, forward;
		AngleVectors( GetLocalAngles(), &forward, &right, NULL );
		
		right = right * 100;
		forward = forward * 200;

		ClawAttack( GetClawAttackRange(), 25, QAngle( -15, -20, -10 ), right + forward );
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}
int				CNPC_Attack_Dog::MeleeAttack1Conditions ( float flDot, float flDist )
{
	float range = GetClawAttackRange();

	if (flDist > range )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}

	// Build a cube-shaped hull, the same hull that ClawAttack() is going to use.
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	Vector forward;
	GetVectors( &forward, NULL, NULL );

	trace_t	tr;
	CTraceFilterNav traceFilter( this, false, this, COLLISION_GROUP_NONE );
	AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + forward * GetClawAttackRange(), vecMins, vecMaxs, MASK_NPCSOLID, &traceFilter, &tr );

	if( tr.fraction == 1.0 || !tr.m_pEnt )
	{
		// This attack would miss completely. Trick the zombie into moving around some more.
		return COND_TOO_FAR_TO_ATTACK;
	}

	if( tr.m_pEnt == GetEnemy() || 
		tr.m_pEnt->IsNPC() || 
		( tr.m_pEnt->m_takedamage == DAMAGE_YES && (dynamic_cast<CBreakableProp*>(tr.m_pEnt) ) ) )
	{
		// -Let the zombie swipe at his enemy if he's going to hit them.
		// -Also let him swipe at NPC's that happen to be between the zombie and the enemy. 
		//  This makes mobs of zombies seem more rowdy since it doesn't leave guys in the back row standing around.
		// -Also let him swipe at things that takedamage, under the assumptions that they can be broken.
		return COND_CAN_MELEE_ATTACK1;
	}

	Vector vecTrace = tr.endpos - tr.startpos;
	float lenTraceSq = vecTrace.Length2DSqr();

	if ( GetEnemy() && GetEnemy()->MyCombatCharacterPointer() && tr.m_pEnt == static_cast<CBaseCombatCharacter *>(GetEnemy())->GetVehicleEntity() )
	{
		if ( lenTraceSq < Square( GetClawAttackRange() * 0.75f ) )
		{
			return COND_CAN_MELEE_ATTACK1;
		}
	}

	if( tr.m_pEnt->IsBSPModel() )
	{
		// The trace hit something solid, but it's not the enemy. If this item is closer to the zombie than
		// the enemy is, treat this as an obstruction.
		Vector vecToEnemy = GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter();

		if( lenTraceSq < vecToEnemy.Length2DSqr() )
		{
			return COND_DOG_LOCAL_MELEE_OBSTRUCTION;
		}
	}

	// Move around some more
	return COND_TOO_FAR_TO_ATTACK;
}
CBaseEntity *	CNPC_Attack_Dog::ClawAttack( float flDist, int iDamage, QAngle &qaViewPunch, Vector &vecVelocityPunch )
{
	// Added test because claw attack anim sometimes used when for cases other than melee
	int iDriverInitialHealth = -1;
	CBaseEntity *pDriver = NULL;
	if ( GetEnemy() )
	{
		trace_t	tr;
		AI_TraceHull( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction < 1.0f )
			return NULL;

		// CheckTraceHullAttack() can damage player in vehicle as side effect of melee attack damaging physics objects, which the car forwards to the player
		// need to detect this to get correct damage effects
		CBaseCombatCharacter *pCCEnemy = ( GetEnemy() != NULL ) ? GetEnemy()->MyCombatCharacterPointer() : NULL;
		CBaseEntity *pVehicleEntity;
		if ( pCCEnemy != NULL && ( pVehicleEntity = pCCEnemy->GetVehicleEntity() ) != NULL )
		{
			if ( pVehicleEntity->GetServerVehicle() && dynamic_cast<CPropVehicleDriveable *>(pVehicleEntity) )
			{
				pDriver = static_cast<CPropVehicleDriveable *>(pVehicleEntity)->GetDriver();
				if ( pDriver && pDriver->IsPlayer() )
				{
					iDriverInitialHealth = pDriver->GetHealth();
				}
				else
				{
					pDriver = NULL;
				}
			}
		}
	}

	//
	// Trace out a cubic section of our hull and see what we hit.
	//
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	CBaseEntity *pHurt = NULL;
	if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
	{ 
		// We always hit bullseyes we're targeting
		pHurt = GetEnemy();
		CTakeDamageInfo info( this, this, vec3_origin, GetAbsOrigin(), iDamage, DMG_SLASH );
		pHurt->TakeDamage( info );
	}
	else 
	{
		// Try to hit them with a trace
		pHurt = CheckTraceHullAttack( flDist, vecMins, vecMaxs, iDamage, DMG_SLASH );
	}

	if ( pDriver && iDriverInitialHealth != pDriver->GetHealth() )
	{
		pHurt = pDriver;
	}
	
	if ( pHurt )
	{
		AttackHitSound();

		CBasePlayer *pPlayer = ToBasePlayer( pHurt );

		if ( pPlayer != NULL && !(pPlayer->GetFlags() & FL_GODMODE ) )
		{
			pPlayer->ViewPunch( qaViewPunch );
			
			pPlayer->VelocityPunch( vecVelocityPunch );
		}
		else if( !pPlayer && UTIL_ShouldShowBlood(pHurt->BloodColor()) )
		{
			// Hit an NPC. Bleed them!
			Vector vecBloodPos;
			if( GetAttachment( "blood_left", vecBloodPos ) )
				SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), min( iDamage, 30 ) );			
		}
	}
	else 
	{
		AttackMissSound();
	}

	return pHurt;
}

int				CNPC_Attack_Dog::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_CHASE_ENEMY:
		if ( HasCondition( COND_DOG_LOCAL_MELEE_OBSTRUCTION ) && !HasCondition(COND_TASK_FAILED) && IsCurSchedule( SCHED_DOG_CHASE_ENEMY, false ) )
		{
			return SCHED_COMBAT_PATROL;
		}
		return SCHED_DOG_CHASE_ENEMY;
		break;

	case SCHED_MELEE_ATTACK1:
		return SCHED_DOG_MELEE_ATTACK1;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}
void			CNPC_Attack_Dog::BuildScheduleTestBits( void )
{
	// Ignore damage if we were recently damaged or we're attacking.
	if ( GetActivity() == ACT_DOG_THROW )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}

	BaseClass::BuildScheduleTestBits();
}
void			CNPC_Attack_Dog::GatherConditions( void )
{
	ClearCondition( COND_DOG_LOCAL_MELEE_OBSTRUCTION );

	BaseClass::GatherConditions();	
}
Activity		CNPC_Attack_Dog::NPC_TranslateActivity( Activity baseAct )
{
	if ( baseAct == ACT_WALK && IsCurSchedule( SCHED_COMBAT_PATROL, false) )
		baseAct = ACT_RUN;	

	return BaseClass::NPC_TranslateActivity( baseAct );
}
AI_BEGIN_CUSTOM_NPC( npc_attack_dog, CNPC_Attack_Dog )

	DECLARE_USES_SCHEDULE_PROVIDER( CAI_FollowBehavior )

	DECLARE_TASK( TASK_DOG_WAIT_POST_MELEE )

	DECLARE_ACTIVITY( ACT_DOG_THROW )

	DECLARE_CONDITION( COND_DOG_LOCAL_MELEE_OBSTRUCTION )

	DECLARE_ANIMEVENT( AE_DOG_THROW )

	DEFINE_SCHEDULE
	(
		SCHED_DOG_CHASE_ENEMY,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		 TASK_SET_TOLERANCE_DISTANCE	24"
		"		 TASK_GET_CHASE_PATH_TO_ENEMY	600"
		"		 TASK_RUN_PATH					0"
		"		 TASK_WAIT_FOR_MOVEMENT			0"
		"		 TASK_FACE_ENEMY				0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_TASK_FAILED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_DOG_MELEE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
		"		TASK_MELEE_ATTACK1		0"
		"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_DOG_POST_MELEE_WAIT"
	)

	DEFINE_SCHEDULE
	(
		SCHED_DOG_POST_MELEE_WAIT,

		"	Tasks"
		"		TASK_DOG_WAIT_POST_MELEE		0"
	)

AI_END_CUSTOM_NPC()