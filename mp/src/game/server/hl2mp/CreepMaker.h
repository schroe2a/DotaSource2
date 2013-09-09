#pragma once

class CreepMaker : public CBaseEntity
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CreepMaker, CBaseEntity );

public:	

	CreepMaker(void);

	void Precache( void );
	void Spawn( void );
	void MakerThink( void );
	bool HumanHullFits( const Vector &vecLocation );
	bool CanMakeNPC( bool bIgnoreSolidEntities = false );

	virtual void DeathNotice( CBaseEntity *pChild );// NPC maker children use this to tell the NPC maker that they have died.

	virtual void MakeMultipleNPCS( int nNPCs );
	void MakeNPCInRadius( void );

	string_t m_WaypointName;

	float		m_flSpawnFrequency;		// delay (in secs) between spawns 

	int			m_nLiveChildren;	// how many NPCs made by this NPC maker that are currently alive
	int			m_nMaxLiveChildren;	// max number of NPCs that this maker may have out at one time.
	int			m_nGroupSize;

	string_t	m_spawnEquipment;

	string_t	m_AssaultPointName;

protected:
	
	bool PlaceNPCInRadius( CAI_BaseNPC *pNPC );

	float	m_flRadius;
	int		m_GroupCount;
};
