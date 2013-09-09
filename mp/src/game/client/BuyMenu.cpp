#include "cbase.h"

#include <cdll_client_int.h>

#include "BuyMenu.h"
#include "HeroDef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CBuyMenu::CBuyMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_BUY)
{
	m_pViewPort = pViewPort;

	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(true);
	
	m_iHasPistol = 0;
	m_iHasSMG = 0;
	m_iHasAR2 = 0;
	m_iHasBuckshot = 0;
	m_iHas357 = 0;
	m_iHasXBow = 0;
	m_iHasPhysCannon = 0;

	m_iMoney  = 0;

	LoadControlSettings("Resource/UI/BuyMenu.res");
	InvalidateLayout();
}

CBuyMenu::~CBuyMenu(void)
{
}

void CBuyMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();

		SetControlText();

		SetMouseInputEnabled( true );		
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}

	m_pViewPort->ShowBackGround( bShow );
}

void CBuyMenu::OnCommand( const char *command )
{
	if ( Q_stricmp( command, "vguicancel" ) )
	{
		engine->ClientCmd( const_cast<char *>( command ) );
	}

	Close();
	gViewPortInterface->ShowBackGround( false );
	
	BaseClass::OnCommand(command);	
}

void CBuyMenu::SetData(KeyValues *data)
{
	m_iHasPistol = data->GetInt( "m_iHasPistol" );
	m_iHasSMG = data->GetInt( "m_iHasSMG" );
	m_iHasAR2 = data->GetInt( "m_iHasAR2" );	
	m_iHasBuckshot = data->GetInt( "m_iHasBuckshot" );
	m_iHas357 = data->GetInt( "m_iHas357" );
	m_iHasXBow = data->GetInt( "m_iHasXBow" );
	m_iHasPhysCannon = data->GetInt( "m_iHasPhysCannon" );
	m_iMoney = data->GetInt( "m_iMoney" );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CBuyMenu::SetLabelText(const char *textEntryName, const char *text)
{
	vgui::Label *entry = dynamic_cast<vgui::Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

void CBuyMenu::SetControlText()
{
	char buttonText[64];
	char buttonCommand[64];

	Q_snprintf( buttonText, sizeof(buttonText), "Buy something! You have $%i to spend.", m_iMoney ); 
	SetLabelText( "buySomething", buttonText );

	for ( int i = 1; i < MAX_AMMO_TYPES; i++ )
	{
		Item_t * item = GetItemDef()->GetItemOfIndex(i);
		if ( item )
		{
			vgui::Button *entry = dynamic_cast<vgui::Button *>(FindChildByName(item->pName));
			if (entry)
			{	
				int currentLevel = this->ItemLevel( item->pName );
				int totalCost = item->cost + ( currentLevel * (item->cost / 2) );

				Q_snprintf( buttonText, sizeof(buttonText), "%s(%i) - $%i", item->pName, currentLevel+1, totalCost );
				Q_snprintf( buttonCommand, sizeof(buttonCommand), "buy %s", item->pName );
				entry->SetText( buttonText );
				entry->SetCommand( buttonCommand );
				entry->SetEnabled( ItemAvailable(item) );
			}
		}
	}
}

bool CBuyMenu::ItemAvailable( Item_t * item )
{
	if ( item->cost > m_iMoney )
		return false;

	if ( Q_stristr(item->pName, "AR2AltFire" ) )
		return ItemLevel( "weapon_ar2" ) > 0;

	if ( Q_stristr(item->pName, "smg1_grenade" ) )
		return ItemLevel( "weapon_357" ) > 0;

	if ( Q_stristr(item->pName, "weapon_physcannon" ) )
		return ItemLevel( "weapon_physcannon" ) == 0;

	return ItemLevel( item->pName ) <= 3;
}

int CBuyMenu::ItemLevel( char * itemName )
{
	if ( Q_stristr(itemName, "weapon_pistol" ) )
		return m_iHasPistol;
	else if ( Q_stristr(itemName, "weapon_357" ) )
		return m_iHas357;
	else if ( Q_stristr(itemName, "weapon_smg1" ) )
		return m_iHasSMG;
	else if ( Q_stristr(itemName, "weapon_ar2" ) )
		return m_iHasAR2;
	else if ( Q_stristr(itemName, "weapon_shotgun" ) )
		return m_iHasBuckshot;
	else if ( Q_stristr(itemName, "weapon_crossbow" ) )
		return m_iHasXBow;
	else if ( Q_stristr(itemName, "weapon_physcannon" ) )
		return m_iHasPhysCannon; 

	return 0;
}