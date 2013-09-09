#pragma once

#include "ai_behavior.h"
#include "ai_behavior_assault.h"

class CCreepWaypoint : public CPointEntity
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CCreepWaypoint, CPointEntity );

public:
	CCreepWaypoint() {}

	string_t		m_NextAssaultPointName;
	float			m_flTimeLastUsed;
};

class CAI_CreepBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_CreepBehavior, CAI_SimpleBehavior );
	DEFINE_CUSTOM_SCHEDULE_PROVIDER;

public:
	CAI_CreepBehavior(void);

	virtual void InitializeBehavior( string_t pFirstWaypoint );

	virtual const char *GetName() {	return "Creep"; }
	virtual int	DrawDebugTextOverlays( int text_offset );

	virtual bool 	CanSelectSchedule();

	bool HasHitAssaultPoint() { return m_bHitAssaultPoint; }

	void ClearAssaultPoint( void );
	void OnHitAssaultPoint( void );

	CCreepWaypoint *FindAssaultPoint( string_t iszAssaultPointName );
	void SetAssaultPoint( CCreepWaypoint *pAssaultPoint );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	int TranslateSchedule( int scheduleType );
	void ClearSchedule( const char *szReason );

	CHandle<CCreepWaypoint> m_hAssaultPoint;
	
	enum
	{
		SCHED_CREEP_MOVE_TO_ASSAULT_POINT = BaseClass::NEXT_SCHEDULE,
		SCHED_CREEP_FAILED_TO_MOVE,
		SCHED_CREEP_AT_ASSAULT_POINT,
		SCHED_CREEP_MOVE_AWAY,
		NEXT_SCHEDULE,

		TASK_CREEP_GET_PATH_TO_ASSAULT_POINT = BaseClass::NEXT_TASK,
		TASK_CREEP_FACE_ASSAULT_POINT,
		TASK_CREEP_HIT_ASSAULT_POINT,
		TASK_CREEP_ANNOUNCE_CLEAR,
		TASK_CREEP_DEFER_SCHEDULE_SELECTION,
		NEXT_TASK,
	};

private:
	virtual int		SelectSchedule();

	bool			m_bHitAssaultPoint;

	bool			m_bDiverting;
	float			m_flLastSawAnEnemyAt;

	float			m_flTimeDeferScheduleSelection;	

	string_t		m_AssaultPointName;
};
