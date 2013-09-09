#pragma once
#include "items.h"

#define MONEY_MODEL "models/items/boxflares.mdl"

class CItemMoney : public CItem
{
public:
	DECLARE_CLASS( CItemMoney, CItem );

	CItemMoney::CItemMoney();

	void Spawn( void );
	void Precache( void );
	bool MyTouch( CBasePlayer *pPlayer );

	int				m_iAmount;
	CBasePlayer *	m_taker;
};

void DropMoney( const Vector &vecOrigin, int amount, CBasePlayer * pTaker );