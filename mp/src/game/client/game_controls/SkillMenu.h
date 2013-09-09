#pragma once

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <game/client/iviewport.h>

namespace vgui
{
	class RichText;
}

class CSkillMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CSkillMenu, vgui::Frame );

public:
	CSkillMenu(IViewPort *pViewPort);
	virtual ~CSkillMenu(void);

	virtual const char *GetName( void ) { return PANEL_SKILL; }
	virtual void SetData(KeyValues *data);
	virtual void Reset() {};
	virtual void Update() {};
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

protected:

	// helper functions
	void SetLabelText(const char *textEntryName, const char *text);

	virtual void OnCommand( const char *command );

	virtual void SetControlText();

	IViewPort	*m_pViewPort;

	int			m_HeroType;
	int			m_iLevel;
	int			m_iSkillPoints;

	int			m_iSkillLevel[4];

	int			m_iStatLevel;
};
