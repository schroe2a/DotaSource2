#include "cbase.h"

#include "npc_creep.h"

#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	dota_creep_grenades( "dota_creep_grenades","5");
ConVar	sv_dotaCreepUpdateRate( "sv_dotaCreepUpdateRate", "1.0");

static const char *g_ppszRandomHeads[] = 
{
	"male_01.mdl",
	"male_02.mdl",
	"female_01.mdl",
	"male_03.mdl",
	"female_02.mdl",
	"male_04.mdl",
	"female_03.mdl",
	"male_05.mdl",
	"female_04.mdl",
	"male_06.mdl",
	"female_06.mdl",
	"male_07.mdl",
	"female_07.mdl",
	"male_08.mdl",
	"male_09.mdl",
};

struct HeadCandidate_t
{
	int iHead;
	int nHeads;

	static int __cdecl Sort( const HeadCandidate_t *pLeft, const HeadCandidate_t *pRight )
	{
		return ( pLeft->nHeads - pRight->nHeads );
	}
};

LINK_ENTITY_TO_CLASS( npc_creep, CNPC_Creep );

BEGIN_DATADESC( CNPC_Creep )

	//DEFINE_KEYFIELD( m_iTeamNum,				FIELD_INTEGER,	"Team" ),	

END_DATADESC()

CNPC_Creep::CNPC_Creep()
{
	m_iNumGrenades = dota_creep_grenades.GetInt();
}

bool CNPC_Creep::CreateBehaviors()
{
	AddBehavior( &m_CreepBehavior );

	return BaseClass::CreateBehaviors();
}

void CNPC_Creep::SelectRebelModel()
{
	const char *pszModelName = NULL;

	// Count the heads
	int headCounts[ARRAYSIZE(g_ppszRandomHeads)] = { 0 };
	int i;

	for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
	{
		CNPC_Creep *pCitizen = dynamic_cast<CNPC_Creep *>(g_AI_Manager.AccessAIs()[i]);
		if ( pCitizen && pCitizen != this && pCitizen->m_iHead >= 0 && pCitizen->m_iHead < ARRAYSIZE(g_ppszRandomHeads) )
		{
			headCounts[pCitizen->m_iHead]++;
		}
	}

	// Find all candidates
	CUtlVectorFixed<HeadCandidate_t, ARRAYSIZE(g_ppszRandomHeads)> candidates;

	for ( i = 0; i < ARRAYSIZE(g_ppszRandomHeads); i++ )
	{
		HeadCandidate_t candidate = { i, headCounts[i] };
		candidates.AddToTail( candidate );		
	}

	Assert( candidates.Count() );
	candidates.Sort( &HeadCandidate_t::Sort );

	int iSmallestCount = candidates[0].nHeads;
	int iLimit;

	for ( iLimit = 0; iLimit < candidates.Count(); iLimit++ )
	{
		if ( candidates[iLimit].nHeads > iSmallestCount )
			break;
	}

	m_iHead = candidates[random->RandomInt( 0, iLimit - 1 )].iHead;
	pszModelName = g_ppszRandomHeads[m_iHead];
	SetModelName( AllocPooledString( CFmtStr( "models/Humans/%s/%s", (const char *)(CFmtStr("Group03%s", ( IsMedic() ) ? "m" : "" )), pszModelName ) ) );
}

// Determine if creep that is out of range of the player transmit its position/etc. Based on svar sv_dotaCreepUpdateRate (default 1.0 -- once per second)
int CNPC_Creep::ShouldTransmit(const CCheckTransmitInfo *pInfo) { // Issue #33: AMP - 2013-10-18 - Set creep update rate
	if (gpGlobals->curtime>=m_fNextFullUpdate) {
		m_fNextFullUpdate = gpGlobals->curtime + sv_dotaCreepUpdateRate.GetFloat();
		return FL_EDICT_ALWAYS;
	} else 
		return FL_EDICT_DONTSEND;
}

void CNPC_Creep::Spawn()
{
	if (sv_dotaCreepUpdateRate.GetFloat()<-0.001f)
		m_fNextFullUpdate= INT_MAX; // Never update
	else 
		m_fNextFullUpdate= 0.0f;
	CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );

	if ( this->GetTeamNumber() != TEAM_COMBINE && this->GetTeamNumber() != TEAM_REBELS )
	{
		this->ChangeTeam( TEAM_COMBINE );
	}

	if ( this->GetTeamNumber() == TEAM_COMBINE )
	{
		SetModelName( MAKE_STRING( "models/combine_soldier.mdl" ) );
	}
	else if ( this->GetTeamNumber() == TEAM_REBELS )
	{
		SelectRebelModel();		
	}

	BaseClass::Spawn();
}