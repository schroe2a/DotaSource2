#pragma once

#include "npc_combines.h"
#include "ai_behavior.h"
#include "ai_behavior_creep.h"

class CNPC_Creep : public CNPC_CombineS
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CNPC_Creep, CNPC_CombineS );

public:

	CNPC_Creep();

	bool			CreateBehaviors();

	void			Spawn();

	void			SelectRebelModel();

	bool 			IsMedic()				{ return false; }

	bool			IsInPlayerSquad() const { return false; }

	//WeaponProficiency_t CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );

private:
	CAI_CreepBehavior			m_CreepBehavior;

	int				m_iHead;
};
