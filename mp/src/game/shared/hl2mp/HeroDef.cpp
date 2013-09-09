#include "cbase.h"
#include "HeroDef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Return a pointer to the Hero at the Index passed in
//-----------------------------------------------------------------------------
Hero_t *CHeroDef::GetHeroOfIndex(int nHeroIndex)
{
	if ( nHeroIndex >= m_nHeroIndex )
		return NULL;

	return &m_HeroType[ nHeroIndex ];
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CHeroDef::Index(const char *psz)
{
	int i;

	if (!psz)
		return -1;

	for (i = 1; i < m_nHeroIndex; i++)
	{
		if (stricmp( psz, m_HeroType[i].pName ) == 0)
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CHeroDef::BaseHealth(int nHeroIndex)
{
	if ( nHeroIndex < 1 || nHeroIndex >= m_nHeroIndex )
		return 0;

	return m_HeroType[nHeroIndex].baseHealth;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
float	CHeroDef::AmmoBonus(int nHeroIndex)
{
	return m_HeroType[nHeroIndex].ammoBonus;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
float	CHeroDef::SpeedBonus(int nHeroIndex)
{
	return m_HeroType[nHeroIndex].speedBonus;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
char *CHeroDef::MainWeapon(int nHeroIndex)
{
	if (nHeroIndex < 1 || nHeroIndex >= m_nHeroIndex)
		return 0;

	return m_HeroType[nHeroIndex].pMainWeapon;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
char *CHeroDef::AltWeapon(int nHeroIndex)
{
	if (nHeroIndex < 1 || nHeroIndex >= m_nHeroIndex)
		return 0;

	return m_HeroType[nHeroIndex].pAltWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Add an Hero type with it's damage & carrying capability specified via integers
//-----------------------------------------------------------------------------
void CHeroDef::AddHeroType(char const* name, int baseHealth, float ammoBonus, float speedBonus, char const* pMainWeapon, char const* pAltWeapon )
{
	if (m_nHeroIndex == MAX_AMMO_TYPES)
		return;

	int len = strlen(name);
	m_HeroType[m_nHeroIndex].pName = new char[len+1];
	Q_strncpy(m_HeroType[m_nHeroIndex].pName, name,len+1);
	m_HeroType[m_nHeroIndex].baseHealth	= baseHealth;
	m_HeroType[m_nHeroIndex].ammoBonus	= ammoBonus;
	m_HeroType[m_nHeroIndex].speedBonus	= speedBonus;

	len = strlen(pMainWeapon);
	m_HeroType[m_nHeroIndex].pMainWeapon = new char[len+1];
	Q_strncpy(m_HeroType[m_nHeroIndex].pMainWeapon, pMainWeapon, len+1);

	len = strlen(pAltWeapon);
	m_HeroType[m_nHeroIndex].pAltWeapon = new char[len+1];
	Q_strncpy(m_HeroType[m_nHeroIndex].pAltWeapon, pAltWeapon, len+1);

	m_HeroType[m_nHeroIndex].pTakenBy = NULL;
	
	m_nHeroIndex++;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CHeroDef::CHeroDef(void)
{
	// Start with an index of 1.  Client assumes 0 is an invalid Hero type
	m_nHeroIndex = 1;
	memset( m_HeroType, 0, sizeof( m_HeroType ) );
}

CHeroDef::~CHeroDef( void )
{
	for ( int i = 1; i < MAX_AMMO_TYPES; i++ )
	{
		delete[] m_HeroType[ i ].pName;
		delete[] m_HeroType[ i ].pMainWeapon;
		delete[] m_HeroType[ i ].pAltWeapon;
	}
}