#include "cbase.h"
#include "c_BaseSkill.h"

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_BaseSkill, DT_BaseSkill, CBaseSkill )
	RecvPropFloat( RECVINFO( m_iCooldown ) ),
	RecvPropInt( RECVINFO( m_iLevel ) ),
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),	
END_RECV_TABLE()

C_BaseSkill::C_BaseSkill()
{
	m_iLevel = 0;
	m_iCooldown = 0;
}