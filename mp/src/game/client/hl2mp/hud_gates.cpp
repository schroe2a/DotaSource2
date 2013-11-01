#include "hud.h"
#include "cbase.h"
#include "hud_gates.h"
#include "iclientmode.h"
#include "hud_macros.h"
#include "c_team.h"
#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"
 
#include "tier0/memdbgon.h"
 
using namespace vgui;
 
DECLARE_HUDELEMENT( CHudGates );

static ConVar show_hudgates( "cl_show_hudgates", "1", 0, "Toggles the gates state HUD panel" );
ConVar closed_gate_alpha( "cl_closed_hudgate_alpha", "100", 0, "The alpha value for the closed gate in the HUD" );
ConVar gate_icon_grad_size( "cl_gate_icon_grad_size", "20", 0, "The gradient size for drawing hurt gates in the HUD" );
ConVar gate_icon_dmg_grad_size( "cl_gate_icon_dmg_grad_size", "10", 0, "The gradient size for drawing highlights for hurt gates in the HUD" );
ConVar gate_icon_dmg_upd_time( "cl_gate_icon_dmg_upd_time", "0.4", 0, "The amount of time for the gradient for highlighting hurt gates in the HUD lasts" );

CHudGates::CHudGates( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudGates" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetVisible( false );
	SetAlpha( 255 );

	// Create Texture here (if using textures?)

	// Set hidden states, if necessary
	//SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_nInset = 4;
	m_nIconWidth = 32;
	m_nIconHeight = 6;
	m_nIconSpacing = 2;
}

void CHudGates::Update( void )
{
	m_fWorldTime = gpGlobals->curtime;

}

void CHudGates::SetTime( float time )
{
	m_fWorldTime = time;
}

void CHudGates::Paint()
{
	int closedGateAlpha = closed_gate_alpha.GetInt();
	int alpha = 160;
	int gradSize = gate_icon_grad_size.GetInt();
	int dmgGradSize = gate_icon_dmg_grad_size.GetInt();
	float dmgGradTime = gate_icon_dmg_upd_time.GetFloat();

	m_fWorldTime = gpGlobals->curtime;
	Color openEnemyColor   ( 255,               0,               0, alpha );
	Color closedEnemyColor ( closedGateAlpha,   0,               0, alpha );
	Color openFriendColor  ( 0,                 255,             0, alpha );
	Color closedFriendColor( 0,                 closedGateAlpha, 0, alpha );

	for ( int i = 0; i < m_GateStates.Count(); i++ )
	{
		GateState_t *gate = &m_GateStates[i];

		int boundX1 = m_nInset + (gate->gatePos * m_nIconWidth ) + (gate->gatePos * m_nIconSpacing);
		int boundY1 = m_nInset + (gate->team    * m_nIconHeight) + (gate->team    * m_nIconSpacing);
		int boundX2 = boundX1 + m_nIconWidth;
		int boundY2 = boundY1 + m_nIconHeight;

		if ( gate->state ) {

			for (int j = 0; j < gradSize; j++)
			{
				int xOff = (j * m_nIconWidth  / 2 / gradSize);
				int yOff = (j * m_nIconHeight / 2 / gradSize);
				int x1 = boundX1 + xOff;
				int y1 = boundY1 + yOff;
				int x2 = boundX2 - xOff;
				int y2 = boundY2 - yOff;

				int colorRange = 255 - closedGateAlpha;
				int colorMin  = (1.0*closedGateAlpha) + (gate->health * colorRange);
				float colorIncr = (255.0 - colorMin) / gradSize;
				Color enemyClr ( (255 - (colorIncr * (gradSize - j))), 0, 0, alpha );
				Color friendClr( 0, (255 - (colorIncr * (gradSize - j))), 0, alpha );

				Color *c = NULL;
				if ( gate->team == 0 ) {
					c = &enemyClr;
				} else {
					c = &friendClr;
				}

				surface()->DrawSetColor( *c );
				surface()->DrawFilledRect(
					scheme()->GetProportionalScaledValue( x1 ),
					scheme()->GetProportionalScaledValue( y1 ),
					scheme()->GetProportionalScaledValue( x2 ),
					scheme()->GetProportionalScaledValue( y2 )
					);
			}

			if ( gate->dmgState )
			{
				Color dmgColor( 255, 255, 255, (int)(255.0 * (1.0*gate->dmgState / dmgGradSize)) );
				surface()->DrawSetColor( dmgColor );
				surface()->DrawOutlinedRect( 
					scheme()->GetProportionalScaledValue( boundX1-1 ), 
					scheme()->GetProportionalScaledValue( boundY1-1 ), 
					scheme()->GetProportionalScaledValue( boundX2+1 ), 
					scheme()->GetProportionalScaledValue( boundY2+1 ) 
					);

				if ( m_fWorldTime >= gate->nextUpdateTime )
				{
					--gate->dmgState;
					gate->nextUpdateTime = m_fWorldTime + (dmgGradTime / dmgGradSize);
				}
			}
		} else {
			Color *c = (gate->team == 0 ? &closedEnemyColor : &closedFriendColor);

			surface()->DrawSetColor( *c );
			surface()->DrawFilledRect(
				scheme()->GetProportionalScaledValue( boundX1 ),
				scheme()->GetProportionalScaledValue( boundY1 ),
				scheme()->GetProportionalScaledValue( boundX2 ),
				scheme()->GetProportionalScaledValue( boundY2 )
				);
		}
	}
}

void CHudGates::togglePrint()
{
	this->SetVisible( m_GateStates.Count() > 0 && show_hudgates.GetBool() );
}

void CHudGates::OnThink()
{
	togglePrint();

	BaseClass::OnThink();
}

void CHudGates::Init()
{
	// register for events as client listener
	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "round_start" );
	ListenForGameEvent( "objectivegate_open" );
	ListenForGameEvent( "objectivegate_close" );
	ListenForGameEvent( "objectivegate_attacked" );
}

void CHudGates::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if ( Q_strcmp(type, "game_newmap") == 0 )
	{
		ResetRound( false );
		m_fWorldTime = 0;
	}
	else if ( Q_strcmp(type, "round_start") == 0 )
	{
	}
	else if ( Q_strcmp(type, "objectivegate_open") == 0 )
	{
		const char* lane = event->GetString( "lane" );
		int gateTeam = event->GetInt( "team" );
		SetGateState( true, gateTeam, lane );
	}
	else if ( Q_strcmp(type, "objectivegate_close") == 0 )
	{
		const char* lane = event->GetString( "lane" );
		int gateTeam = event->GetInt( "team" );
		SetGateState( false, gateTeam, lane );
	}
	else if ( Q_strcmp(type, "objectivegate_attacked") == 0 )
	{
		const char* lane = event->GetString( "lane" );
		int gateTeam = event->GetInt( "team" );
		float health = event->GetFloat( "health" );
		SetGateHealth( health, gateTeam, lane );
	}
}

void CHudGates::SetGateState( bool isOpen, int gateTeam, const char *lane )
{
		int laneIdx = GetLaneIndex( lane );
		C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();

		if ( localPlayer )
		{
			bool isEnemyGate = localPlayer->GetTeamNumber() != gateTeam;

			GateState_t* gate = FindGateByTeamLane( !isEnemyGate, laneIdx );

			if ( gate ) {
				gate->state = isOpen;
				gate->health = isOpen ? 1.0 : 0.0;
			}
		}
}

void CHudGates::SetGateHealth( float health, int gateTeam, const char *lane )
{
		int laneIdx = GetLaneIndex( lane );
		int dmgGradSize = gate_icon_dmg_grad_size.GetInt();
		float dmgGradTime = gate_icon_dmg_upd_time.GetFloat();
		C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();

		if ( localPlayer )
		{
			bool isEnemyGate = localPlayer->GetTeamNumber() != gateTeam;

			GateState_t* gate = FindGateByTeamLane( !isEnemyGate, laneIdx );

			if ( gate ) {
				gate->health = health;
				gate->dmgState = dmgGradSize;
				gate->nextUpdateTime = m_fWorldTime + (dmgGradTime / dmgGradSize);
			}
		}
}

void CHudGates::ResetRound( bool keepGates )
{
	if (!keepGates)
	{
		m_GateStates.RemoveAll();

		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < MAX_GATES; j++)
			{
				GateState_t gate;
				Q_memset( &gate, 0, sizeof(gate) );
				m_GateStates.AddToTail( gate );
			}
		}
	}

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < MAX_GATES; j++)
		{
			int idx = (i*MAX_GATES)+j;

			if (m_GateStates.Count() >= (idx+1))
			{
				GateState_t *gate = & m_GateStates[idx];

				gate->gatePos = j;
				gate->state = true;
				gate->team = i;
				gate->health = 1.0;
				gate->dmgState = 0;
			}
			else
				break;
		}
	}
}

int CHudGates::GetLaneIndex( const char *lane )
{
	int retVal = -1;
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
	
	if ( localPlayer )
	{
		C_Team *team = localPlayer->GetTeam();

		if ( team )
		{
			const char *teamName = team->Get_Name();  //Rebels
			bool isRebelTeam = Q_strcmp( teamName, "Rebels" ) == 0;
			
			if ( Q_strcmp( lane, "LEFT" ) == 0 )
				retVal = GATEPOS_L;
			if ( Q_strcmp( lane, "CENTER" ) == 0 )
				retVal = GATEPOS_C;
			if ( Q_strcmp( lane, "RIGHT" ) == 0 )
				retVal = GATEPOS_R;

			// If Rebel team, then we need to invert L and R
			if ( isRebelTeam && retVal != GATEPOS_C )
				retVal = (retVal + -Sign(retVal - 1) * 2);
		}
	}

	return retVal;
}

CHudGates::GateState_t* CHudGates::FindGateByTeamLane( bool isPlayerTeam, int laneIndex )
{
	int teamIdx = isPlayerTeam ? 1 : 0;

	for (int i = 0; i < m_GateStates.Count(); i++)
	{
		GateState_t *gate = &m_GateStates[i];

		if (gate->team == teamIdx && gate->gatePos == laneIndex)
			return gate;
	}

	return NULL;
}
