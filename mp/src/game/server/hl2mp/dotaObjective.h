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

	string_t m_creepMakerName;
	string_t m_guardianName;

	bool	m_bMet;

protected:
	int		m_timesHit;
};
