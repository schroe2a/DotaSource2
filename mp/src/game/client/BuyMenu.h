#pragma once

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <game/client/iviewport.h>
#include "ItemDef.h"

class CBuyMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CBuyMenu, vgui::Frame );

public:
	CBuyMenu(IViewPort *pViewPort);
	~CBuyMenu(void);

	virtual const char *GetName( void ) { return PANEL_BUY; }
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

	bool		ItemAvailable( Item_t * item );
	int			ItemLevel( char * itemName );

	IViewPort	*m_pViewPort;

	int 		m_iHasPistol;
	int 		m_iHasSMG;
	int 		m_iHasAR2;
	int 		m_iHasBuckshot;
	int 		m_iHas357;
	int 		m_iHasXBow;
	int 		m_iHasPhysCannon;

	int			m_iMoney;
};