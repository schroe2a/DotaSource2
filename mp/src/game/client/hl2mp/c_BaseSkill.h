#pragma once
#include "c_baseentity.h"
#include "hl2mp_gamerules.h"

class C_BaseSkill :	public C_BaseEntity
{
public:
	DECLARE_CLASS( C_BaseSkill, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_BaseSkill();

	int GetLevel() { return m_iLevel; }
	float GetCooldown() 
	{ 
		return m_iCooldown; 
	}

private:
	C_BaseSkill( const C_BaseSkill & ) {}

	EHANDLE m_hPlayer;
	int		m_iLevel;
	float	m_iCooldown;
};
