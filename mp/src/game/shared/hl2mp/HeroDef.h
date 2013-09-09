#pragma once

class CHL2MP_Player;

struct Hero_t
{
	char			*pName;
	int				baseHealth;
	float			ammoBonus;
	float			speedBonus;
	char			*pMainWeapon;
	char			*pAltWeapon;
	CHandle< CHL2MP_Player > pTakenBy;
};

class CHeroDef
{
public:
	int					m_nHeroIndex;

	Hero_t				m_HeroType[MAX_AMMO_TYPES];

	Hero_t				*GetHeroOfIndex(int nHeroIndex);
	int					Index(const char *psz);
	int					BaseHealth(int nHeroIndex);
	float				AmmoBonus(int nHeroIndex);
	float				SpeedBonus(int nHeroIndex);
	char				*MainWeapon(int nHeroIndex);
	char				*AltWeapon(int nHeroIndex);

	void				AddHeroType(char const* name, int baseHealth, float ammoBonus, float speedBonus, char const* pMainWeapon, char const* pAltWeapon );
	
	CHeroDef(void);
	virtual ~CHeroDef( void );	
};


// Get the global Herodef object. This is usually implemented in each mod's game rules file somewhere,
// so the mod can setup custom Hero types.
CHeroDef* GetHeroDef();
