#pragma once
#include "triggers.h"

class CBuyTrigger :	public CBaseTrigger
{
public:
	CBuyTrigger(void) {}

	DECLARE_CLASS( CBuyTrigger, CBaseTrigger );

	void Spawn( void );

	virtual void StartTouch(CBaseEntity *pOther);
	virtual void EndTouch(CBaseEntity *pOther);

	DECLARE_DATADESC();
};
