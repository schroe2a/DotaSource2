#pragma once
#include "triggers.h"

class CReplenishTrigger : public CBaseTrigger
{
public:
	CReplenishTrigger(void) {}

	DECLARE_CLASS( CTriggerHurt, CBaseTrigger );

	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void ReplenishThink( void );
	int  ReplenishAllTouchers( float dt );
	bool ReplenishEntity( CBaseEntity *pOther, float dt );

	DECLARE_DATADESC();
};
