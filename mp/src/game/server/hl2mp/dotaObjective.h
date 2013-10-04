#pragma once
#include "props.h"

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
};
