#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "c_hl2mp_player.h"
#include "hl2mp_gamerules.h"

#include "c_baseskill.h"

#include "vgui_controls/AnimationController.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INIT_BAT	-1

//-----------------------------------------------------------------------------
// Purpose: Displays suit power (armor) on hud
//-----------------------------------------------------------------------------
class CHudSkill : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudSkill, CHudNumericDisplay );

public:
	CHudSkill( const char *pName, const char *pElementName ) : BaseClass(NULL, pName), CHudElement( pElementName )
	{
		SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_NEEDSUIT );
	}
	void Init( void ) { Reset(); }
	void Reset( void )
	{
		C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
		if ( pPlayer && pPlayer->m_HeroType != -1 )
		{
			wchar_t pwchDest[MAX_PLACE_NAME_LENGTH * 2];			
			V_UTF8ToUnicode( HL2MPRules()->GetSkillName( HL2MPRules()->GetSkillClassForHero( pPlayer->m_HeroType, m_skillNumber )), pwchDest, MAX_PLACE_NAME_LENGTH * 2 );
			SetLabelText( pwchDest );
		}
	}
	void VidInit( void ) { Reset(); }
	void OnThink( void ) 
	{
		C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
		if ( pPlayer && pPlayer->GetSkill(m_skillNumber) )
		{
			if ( pPlayer->GetSkill(m_skillNumber)->GetLevel() < 1 )
			{
				this->SetVisible(false);
			}
			else
			{
				this->SetVisible(true);
				float cooldown = pPlayer->GetSkill(m_skillNumber)->GetCooldown();
				cooldown -= gpGlobals->curtime;
				int icooldown = int(cooldown);
				SetDisplayValue( max(0, icooldown ) );
			}
		}
	}
protected:
	int m_skillNumber;
};

class CHudSkill1 : public CHudSkill
{
	DECLARE_CLASS_SIMPLE( CHudSkill1, CHudSkill );
public:
	CHudSkill1( const char *pElementName ) : BaseClass( "HudSkill1", pElementName) { m_skillNumber = 1; }
};
DECLARE_HUDELEMENT( CHudSkill1 );

class CHudSkill2 : public CHudSkill
{
	DECLARE_CLASS_SIMPLE( CHudSkill2, CHudSkill );
public:
	CHudSkill2( const char *pElementName ) : BaseClass( "HudSkill2", pElementName) { m_skillNumber = 2; }
};
DECLARE_HUDELEMENT( CHudSkill2 );

class CHudSkill3 : public CHudSkill
{
	DECLARE_CLASS_SIMPLE( CHudSkill3, CHudSkill );
public:
	CHudSkill3( const char *pElementName ) : BaseClass( "HudSkill3", pElementName) { m_skillNumber = 3; }
};
DECLARE_HUDELEMENT( CHudSkill3 );

class CHudSkill4 : public CHudSkill
{
	DECLARE_CLASS_SIMPLE( CHudSkill4, CHudSkill );
public:
	CHudSkill4( const char *pElementName ) : BaseClass( "HudSkill4", pElementName) { m_skillNumber = 4; }
};
DECLARE_HUDELEMENT( CHudSkill4 );