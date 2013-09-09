#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "c_hl2mp_player.h"

#include "vgui_controls/AnimationController.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INIT_BAT	-1

//-----------------------------------------------------------------------------
// Purpose: Displays suit power (armor) on hud
//-----------------------------------------------------------------------------
class CHudLevel : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudLevel, CHudNumericDisplay );

public:
	CHudLevel( const char *pElementName );
	void Init( void );
	void Reset( void );
	void VidInit( void );
	void OnThink( void );	
};

DECLARE_HUDELEMENT( CHudLevel );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudLevel::CHudLevel( const char *pElementName ) : BaseClass(NULL, "HudLevel"), CHudElement( pElementName )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLevel::Init( void )
{
	Reset();	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLevel::Reset( void )
{
	SetLabelText(L"LEVEL");
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLevel::VidInit( void )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLevel::OnThink( void )
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if ( pPlayer )
		SetDisplayValue(pPlayer->GetLevel());
}