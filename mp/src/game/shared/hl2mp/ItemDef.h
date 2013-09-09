#pragma once

struct Item_t
{
	char	*pName;
	int		cost;
	char	*pWeaponNeeded;
};

class CItemDef
{
public:
	CItemDef(void);
	~CItemDef(void);

	int		m_nItemIndex;

	Item_t	m_ItemType[MAX_AMMO_TYPES];

	Item_t	*GetItemOfIndex(int nItemIndex);
	int		Index(const char *psz);
	int		GetCost(int nItemIndex);
	char	*GetWeaponNeeded(int nItemIndex);

	void	AddItemType(char const* name, int cost, char const* weaponNeeded);
};

CItemDef* GetItemDef();