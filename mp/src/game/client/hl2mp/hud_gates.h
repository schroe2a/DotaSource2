#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

#define MAX_GATES 3

using namespace vgui;

class CHudGates : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE( CHudGates, Panel );

public:
	enum
	{
		GATEPOS_L = 0,
		GATEPOS_C,
		GATEPOS_R
	};

	CHudGates( const char *pElementName );
	void togglePrint();

	// Virtual functions
	virtual void OnThink();
	virtual void Init( void );
	virtual void FireGameEvent( IGameEvent *event);
	virtual void Update();
	virtual void SetTime( float time );

protected:
	typedef struct GateState_s {
		int gatePos;	// Refer to the GATEPOS_* enum values, they are relative to the what the player sees in the map overview
		int team;		// 0 = Enemy, 1 = Friend
		bool state;		// false = closed, true = open
		float health;
		int dmgState;
		float nextUpdateTime;
	} GateState_t;

	int m_nInset;
	int m_nIconWidth;
	int m_nIconHeight;
	int m_nIconSpacing;
	CUtlVector<GateState_t> m_GateStates;

	int GetLaneIndex( const char* lane );
	GateState_t* FindGateByTeamLane( bool isPlayerTeam, int laneIndex );
	void SetGateState( bool isOpen, int gateTeam, const char *lane );
	void SetGateHealth( float health, int gateTeam, const char *lane );
	float	m_fWorldTime;	// current world time


	// Virtual functions
	virtual void Paint();
	virtual void ResetRound( bool keepGates = true );
};
