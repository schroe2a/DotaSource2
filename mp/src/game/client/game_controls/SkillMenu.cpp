#include "cbase.h"

#include <cdll_client_int.h>

#include "SkillMenu.h"
#include "HeroDef.h"
#include "hl2mp_gamerules.h"
#include <vgui_controls/RichText.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CSkillMenu::CSkillMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_SKILL)
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
	
	m_HeroType = -1;
	m_iLevel = 0;
	m_iSkillPoints = 0;

	for( int i = 0; i<4; i++ ) m_iSkillLevel[i] = 0;

	m_iStatLevel = 0;

	LoadControlSettings("Resource/UI/SkillMenu.res");
	InvalidateLayout();
}

CSkillMenu::~CSkillMenu(void)
{
}

void CSkillMenu::ShowPanel(bool bShow)
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

void CSkillMenu::OnCommand( const char *command )
{
	if ( Q_stricmp( command, "vguicancel" ) )
	{
		engine->ClientCmd( const_cast<char *>( command ) );

		if ( Q_stricmp( command, "upgradeskill 1" ) == 0 )
		{
			m_iSkillLevel[0]++;
		}
		else if ( Q_stricmp( command, "upgradeskill 2" ) == 0 )
		{
			m_iSkillLevel[1]++;
		}
		else if ( Q_stricmp( command, "upgradeskill 3" ) == 0 )
		{
			m_iSkillLevel[2]++;
		}
		else if ( Q_stricmp( command, "upgradeskill 4" ) == 0 )
		{
			m_iSkillLevel[3]++;
		}
		else if ( Q_stricmp( command, "upgradeskill 5" ) == 0 )
		{
			m_iStatLevel++;
		}
		m_iSkillPoints--;
	}

	if ( m_iSkillPoints <= 0 || Q_stricmp( command, "vguicancel" ) == 0 )
	{
		Close();
		gViewPortInterface->ShowBackGround( false );
	}
	else
	{
		SetControlText();
	}
	
	BaseClass::OnCommand(command);	
}

void CSkillMenu::SetData(KeyValues *data)
{
	m_HeroType = data->GetInt( "m_HeroType" );
	m_iLevel = data->GetInt( "m_iLevel" );
	m_iSkillPoints = data->GetInt( "m_iSkillPoints" );

	m_iSkillLevel[0] = data->GetInt( "m_iSkill1Level" );
	m_iSkillLevel[1] = data->GetInt( "m_iSkill2Level" );
	m_iSkillLevel[2] = data->GetInt( "m_iSkill3Level" );
	m_iSkillLevel[3] = data->GetInt( "m_iSkill4Level" );

	m_iStatLevel = data->GetInt( "m_iStatLevel" );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CSkillMenu::SetLabelText(const char *textEntryName, const char *text)
{
	vgui::Label *entry = dynamic_cast<vgui::Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

void CSkillMenu::SetControlText()
{
	char text[500];
	char controlName[64];
	const char * skillClass;
	int cooldown;
	int duration;
	float formula;
	const char * label;
	vgui::RichText * entry;

	Q_snprintf( text, sizeof(text), "Choose Your Skills, %i points remaining.", m_iSkillPoints ); 
	SetLabelText( "chooseSkill", text );

	for( int iSkill = 0; iSkill<4; iSkill++ )
	{
		skillClass = HL2MPRules()->GetSkillClassForHero( m_HeroType, iSkill+1 );
		
		if ( (iSkill < 3 && m_iSkillLevel[iSkill] < 4) ||
			 (iSkill == 3 && m_iSkillLevel[iSkill] < 3) )
			Q_snprintf( text, sizeof(text), "Upgrade %s to Level %i", HL2MPRules()->GetSkillName( skillClass ), m_iSkillLevel[iSkill] + 1 );
		else
			Q_snprintf( text, sizeof(text), "%s Upgrade Complete", HL2MPRules()->GetSkillName( skillClass ) );
		Q_snprintf( controlName, sizeof(controlName), "skill%i", iSkill+1 );
		SetLabelText( controlName, text );		
		
		Q_snprintf( text, sizeof(text), "%s: %s", HL2MPRules()->GetSkillName( skillClass ), HL2MPRules()->GetSkillDesc( skillClass ) ); 

		for( int iLevel = 1; iLevel<=4; iLevel++ )
		{
			if( iSkill == 3 && iLevel == 4 ) continue;

			cooldown = HL2MPRules()->GetSkillCooldown( skillClass, iLevel );
			duration = HL2MPRules()->GetSkillDuration( skillClass, iLevel );
			formula = HL2MPRules()->GetSkillFormula( skillClass, iLevel );
			label = HL2MPRules()->GetSkillFormulaLabel( skillClass, iLevel );

			Q_snprintf( text, sizeof(text), "%s\nLevel %i", text, iLevel );
			if( cooldown != 0 )
				Q_snprintf( text, sizeof(text), "%s, Cooldown:%i", text, cooldown );

			if( duration != 0 )
				Q_snprintf( text, sizeof(text), "%s, Duration:%i", text, duration );

			if( formula != 0 )
				Q_snprintf( text, sizeof(text), "%s, %.2f", text, formula );

			if( label != NULL )
				Q_snprintf( text, sizeof(text), "%s %s", text, label );
		}
		
		Q_snprintf( controlName, sizeof(controlName), "Skill%iText", iSkill+1 );
		entry = dynamic_cast<vgui::RichText *>(FindChildByName(controlName));
		if (entry)
		{
			entry->SetText(text);
		}
	}

	if ( m_iStatLevel < 10 )
	{
		Q_snprintf( text, sizeof(text), "Upgrade stats to Level %i", m_iStatLevel + 1 );
		SetLabelText( "skill5", text );
	}
	else
	{
		SetLabelText( "skill5", "Stats Upgrade Complete" );
	}

	FindChildByName( "skill1" )->SetEnabled( m_iSkillPoints > 0 && m_iSkillLevel[0] < 4 && (((m_iSkillLevel[0]+1) * 2) <= m_iLevel+1) ); 
	FindChildByName( "skill2" )->SetEnabled( m_iSkillPoints > 0 && m_iSkillLevel[1] < 4 && (((m_iSkillLevel[1]+1) * 2) <= m_iLevel+1) ); 
	FindChildByName( "skill3" )->SetEnabled( m_iSkillPoints > 0 && m_iSkillLevel[2] < 4 && (((m_iSkillLevel[2]+1) * 2) <= m_iLevel+1) ); 
	FindChildByName( "skill4" )->SetEnabled( m_iSkillPoints > 0 && 
		((m_iSkillLevel[3] < 1 && m_iLevel >= 6) || 
		 (m_iSkillLevel[3] < 2 && m_iLevel >= 10) || 
		 (m_iSkillLevel[3] < 3 && m_iLevel >= 16) ) );
	FindChildByName( "skill5" )->SetEnabled( m_iSkillPoints > 0 && m_iStatLevel < 10 );
}