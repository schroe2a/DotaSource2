#include "cbase.h"
#include "ItemDef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CItemDef::CItemDef(void)
{
	m_nItemIndex = 1;
	memset( m_ItemType, 0, sizeof( m_ItemType ) );
}

CItemDef::~CItemDef(void)
{
	for ( int i = 1; i < MAX_AMMO_TYPES; i++ )
	{
		delete[] m_ItemType[ i ].pName;
		delete[] m_ItemType[ i ].pWeaponNeeded;
	}
}

Item_t *CItemDef::GetItemOfIndex(int nItemIndex)
{
	if ( nItemIndex >= m_nItemIndex )
		return NULL;

	return &m_ItemType[ nItemIndex ];
}
int CItemDef::Index(const char *psz)
{
	int i;

	if (!psz)
		return -1;

	for (i = 1; i < m_nItemIndex; i++)
	{
		if (stricmp( psz, m_ItemType[i].pName ) == 0)
			return i;
	}

	return -1;
}
int	CItemDef::GetCost(int nItemIndex)
{
	if ( nItemIndex < 1 || nItemIndex >= m_nItemIndex )
		return 0;

	return m_ItemType[nItemIndex].cost;
}
char *CItemDef::GetWeaponNeeded(int nItemIndex)
{
	if ( nItemIndex < 1 || nItemIndex >= m_nItemIndex )
		return 0;

	return m_ItemType[nItemIndex].pWeaponNeeded;
}
void CItemDef::AddItemType(const char *name, int cost, char const* weaponNeeded)
{
	if (m_nItemIndex == MAX_AMMO_TYPES)
		return;

	int len = strlen(name);
	m_ItemType[m_nItemIndex].pName = new char[len+1];
	Q_strncpy(m_ItemType[m_nItemIndex].pName, name,len+1);
	m_ItemType[m_nItemIndex].cost = cost;

	if (weaponNeeded)
	{
		len = strlen(weaponNeeded);
		m_ItemType[m_nItemIndex].pWeaponNeeded = new char[len+1];
		Q_strncpy(m_ItemType[m_nItemIndex].pWeaponNeeded, weaponNeeded, len+1);
	}
	else
		m_ItemType[m_nItemIndex].pWeaponNeeded = NULL;

	m_nItemIndex++;
}