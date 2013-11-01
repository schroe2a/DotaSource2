#pragma once
#include "props.h"

#define OBJECTIVE_HEALTHI 30   // This is the number of times the objective has to take mele damage in order to be "killed" (objective reached)
#define OBJECTIVE_HEALTHF 30.0 // This is the number of times the objective has to take mele damage in order to be "killed" (objective reached)

class DotaObjective : public CDynamicProp
{
	DECLARE_DATADESC();
	DECLARE_CLASS( DotaObjective, CDynamicProp );

public:
	DotaObjective(void);

	virtual void Spawn();

	virtual int OnTakeDamage( const CTakeDamageInfo &info );

	virtual int TakeAction( int dotaAction ); // Open/close the lane
	virtual BOOL CheckLaneMode( ); // Determine if a lane is allowed to be opened

	string_t m_creepMakerName;
	string_t m_guardianName;

	bool	m_bMet;

	enum {
		DOBJ_ACTION_NONE, // Do nothing
		DOBJ_ACTION_OPEN, // Open objective door
		DOBJ_ACTION_CLOSE // Close objective door
	};

protected:
	int		m_timesHit;
	const char* GetLane();

private:
	bool		m_bInit;
};
