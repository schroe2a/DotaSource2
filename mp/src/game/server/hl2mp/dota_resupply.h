#pragma once
#include "baseanimating.h"
#include "hl2mp_player.h"

class Dota_Resupply;

class Dota_Resupply : public CBaseAnimating
{
public:
	DECLARE_CLASS( Dota_Resupply, CBaseAnimating );

	void	Spawn( void );
	void	Precache( void );
	bool	CreateVPhysics( void );

	virtual void HandleAnimEvent( animevent_t *pEvent );
	
	//FIXME: May not want to have this used in a radius
	int		ObjectCaps( void ) { return (BaseClass::ObjectCaps() | (FCAP_IMPULSE_USE|FCAP_USE_IN_RADIUS)); };
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void	CrateThink( void );
	
	virtual int OnTakeDamage( const CTakeDamageInfo &info );

	static bool ReSupplyPlayer( CHL2MP_Player * pPlayer );
	// Issue #28: JMS - 2013-10-12 - Make antlion guards always update to the client
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

protected:

	float	m_flCloseTime;
	CHandle< CHL2MP_Player > m_hActivator;

	DECLARE_DATADESC();
};