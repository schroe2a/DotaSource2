#include "cbase.h"
#include <stdio.h>

#include "MouseoverPanelButton.h"

CUtlVector<CClassImagePanel*> g_ClassImagePanels;

CClassImagePanel::CClassImagePanel( vgui::Panel *pParent, const char *pName )
    : vgui::ImagePanel( pParent, pName )
{
    g_ClassImagePanels.AddToTail( this );
    m_ModelName[0] = 0;
}

CClassImagePanel::~CClassImagePanel()
{
    g_ClassImagePanels.FindAndRemove( this );
}

void CClassImagePanel::ApplySettings( KeyValues *inResourceData )
{
    const char *pName = inResourceData->GetString( "3DModel" );
    if ( pName )
    {
        Q_strncpy( m_ModelName, pName, sizeof( m_ModelName ) );
    }
    BaseClass::ApplySettings( inResourceData );
}

void CClassImagePanel::Paint()
{
    BaseClass::Paint();
}