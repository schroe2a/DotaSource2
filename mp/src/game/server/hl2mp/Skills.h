#pragma once
#include "baseentity.h"
#include "hl2mp_player.h"
#include "props.h"
#include "items.h"

class CBaseSkill :	public CBaseEntity
{
public:
	DECLARE_CLASS( CBaseSkill, CBaseEntity );
	DECLARE_SERVERCLASS();

	CBaseSkill();

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
	
	const char		*GetName() { return HL2MPRules()->GetSkillName( this->GetClassname() ); }

	virtual void	SetPlayer( CHL2MP_Player * player ) 
	{ 
		m_hPlayer = player; 
		this->SetOwnerEntity( player );
	}
	CHL2MP_Player * GetPlayer()	{ return dynamic_cast< CHL2MP_Player* >( m_hPlayer.Get() ); }

	virtual void	LevelUp();

	virtual void	Use();
	virtual void 	SkillThink();

	virtual void	DeActivate() {}
	virtual bool 	CanSkipRespawnPenilty() { return false; }
	virtual void 	SkipRespawnPenilty() {}
	virtual Disposition_t	IRelationType( CAI_BaseNPC * source ) { return D_NU; }
	virtual void	OnDoingDamage( CTakeDamageInfo &inputInfo, CBaseCombatCharacter * victim ) {}
	virtual void	OnPlayerSpawn() {}
	virtual void	Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info ) {}
	virtual void	OnTakeDamage( CTakeDamageInfo &inputInfo ) {}
	virtual void	DeathNotice( CBaseEntity *pVictim );

protected:
	CAI_BaseNPC *	SpawnNPC(const char * className, int health, bool front );
	virtual void	PreNPCSpawn( CAI_BaseNPC *pNPC ) { }
	bool			PlaceNPCInFront( CAI_BaseNPC *pNPC, CBaseEntity * target, bool behind = false );
	bool			PlaceNPCInRadius( CAI_BaseNPC *pNPC, CBaseEntity * target );

	CPhysicsProp *	SpawnPhysicsProp( const char * pModelName, Vector vecSrc );
	virtual void	PrePysicsPropSpawn( CPhysicsProp *pProp ) { }

	
	bool UseSkill();
	
	CNetworkHandle( CBaseEntity, m_hPlayer );
	
	CNetworkVar(int, m_iLevel);

	CNetworkVar(float, m_iCooldown);
	float		m_fCooldownduration[4];

	float		m_bActiveEnds;
	float		m_fActiveduration[4];

	int			m_iMaxLiveChildren;
	int			m_nLiveChildren;
};
//Freman
class CSkillStun : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillStun, CBaseSkill );

	virtual void	OnDoingDamage( CTakeDamageInfo &inputInfo, CBaseCombatCharacter * victim );
};
class CFreemanHealthVial : public CItem
{
public:
	DECLARE_CLASS( CFreemanHealthVial, CItem );

	CFreemanHealthVial::CFreemanHealthVial() 
	{
		m_healthAmount = 5.0f;
		m_taker = NULL;
		m_excludeTaker = false;
	}

	float			m_healthAmount;
	CBasePlayer *	m_taker;
	bool			m_excludeTaker;

	void Spawn( void )
	{
		Precache();
		SetModel( "models/items/healthkit.mdl" );

		BaseClass::Spawn();
	}

	void Precache( void )
	{
		PrecacheModel("models/items/healthkit.mdl");

		PrecacheScriptSound( "HealthVial.Touch" );
	}

	bool MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->GetTeamNumber() != this->GetTeamNumber() )
			return false;

		if ( m_excludeTaker )
		{
			if ( pPlayer == m_taker )
				return false;
		}
		else if ( m_taker && pPlayer != m_taker )
			return false;

		if ( pPlayer->TakeHealth( m_healthAmount, DMG_GENERIC ) )
		{
			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();

			UserMessageBegin( user, "ItemPickup" );
				WRITE_STRING( GetClassname() );
			MessageEnd();

			CPASAttenuationFilter filter( pPlayer, "HealthVial.Touch" );
			EmitSound( filter, pPlayer->entindex(), "HealthVial.Touch" );

			return true;
		}

		return false;
	}
};

class CSkillFindHealth : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillFindHealth, CBaseSkill );

	virtual void	Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );
};

class CFreemanBattery : public CItem
{
public:
	DECLARE_CLASS( CFreemanBattery, CItem );

	float			m_armorAmount;
	CBasePlayer *	m_taker;

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/battery.mdl" );
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		PrecacheModel ("models/items/battery.mdl");

		PrecacheScriptSound( "ItemBattery.Touch" );

	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->GetTeamNumber() != this->GetTeamNumber() )
			return false;

		if ( m_taker && pPlayer != m_taker )
			return false;

		if ( pPlayer->ArmorValue() < pPlayer->GetMaxHealth() )
		{
			pPlayer->IncrementArmorValue( m_armorAmount, pPlayer->GetMaxHealth() );

			CPASAttenuationFilter filter( pPlayer, "ItemBattery.Touch" );
			EmitSound( filter, pPlayer->entindex(), "ItemBattery.Touch" );

			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();

			UserMessageBegin( user, "ItemPickup" );
				WRITE_STRING( "item_battery" );
			MessageEnd();
			
			return true;		
		}
		return false;
	}
};
class CSkillFindArmor : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillFindArmor, CBaseSkill );
	
	virtual void	Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );
};
class CSkillReload : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillReload, CBaseSkill );

	virtual bool CanSkipRespawnPenilty();
	virtual void SkipRespawnPenilty();
};

//Alyx
class CSkillSpeedBoost : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillSpeedBoost, CBaseSkill );

	virtual void Use();
	virtual void DeActivate();
};
class CSkillEvasion : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillEvasion, CBaseSkill );

	virtual void	OnTakeDamage( CTakeDamageInfo &inputInfo );
};
class CSkillSneeky : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillSneeky, CBaseSkill );

	virtual void	LevelUp();
	virtual void	OnPlayerSpawn();
protected:
	virtual void	SetAlpha();
};

class CSkillSummonDog :	public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillSummonDog, CBaseSkill );

	virtual void	Use();
	virtual void	DeActivate();

	CHandle< CAI_BaseNPC > dog;
};

//Barney
class CSkillThrowExplosive : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillThrowExplosive, CBaseSkill );

	virtual void Use();
};

class CSkillTakeBribe : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillTakeBribe, CBaseSkill );

	virtual void	Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );
};
class CSkillSMGAmmo : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillSMGAmmo, CBaseSkill );

	virtual void Use();
};


class CSkillInfiltrate : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillInfiltrate, CBaseSkill );

	virtual void	Use();

	virtual Disposition_t	IRelationType( CAI_BaseNPC * source ); 
};

//Eli
class CSkillHeal : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillHeal, CBaseSkill );

	virtual void	Use();
};
class CSkillAR2Ammo : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillAR2Ammo, CBaseSkill );

	virtual void Use();
};

class CSkillFakeDeath : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillFakeDeath, CBaseSkill );

	virtual void Use();
	virtual void DeActivate();
};

class CSkillSummonRebs : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillSummonRebs, CBaseSkill );

	CSkillSummonRebs() : spawnFlags ( 0 ) {}

	virtual void Use();
protected:
	virtual void PreNPCSpawn( CAI_BaseNPC *pNPC );
	int spawnFlags;
};

//Monk
class CSkillShotgunAmmo : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillShotgunAmmo, CBaseSkill );

	virtual void Use();
};

class CSkillSummonHeadcrab : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillSummonHeadcrab, CBaseSkill );

	virtual void Use();
};

class CSkillZombieInvasion : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillZombieInvasion, CBaseSkill );

	virtual void	Use();
protected:
	virtual void PreNPCSpawn( CAI_BaseNPC *pNPC );
	CHL2MP_Player * m_currentPlayer;
};
class CSkillSummonZombies : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillSummonZombies, CBaseSkill );

	virtual void Use();
};

//Gman
class CSkillPhoneHome : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillPhoneHome, CBaseSkill );

	virtual void Use();
};

class CSkillSwap : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillSwap, CBaseSkill );

	virtual void	Use();
};

class CSkillBlink : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillBlink, CBaseSkill );

	virtual void Use();
};

struct healthPos_t
{
	int		health;
	int		armor;
	Vector	origin;
	QAngle  orintation;
};
class CSkillTimeLapse : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillTimeLapse, CBaseSkill );

	virtual void	LevelUp();

	virtual void	Use();
	virtual void 	SkillThink();
protected:
	healthPos_t		past[5];	
};

//Mossman
class CSkillBackstab : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillBackstab, CBaseSkill );

	virtual void	OnDoingDamage( CTakeDamageInfo &inputInfo, CBaseCombatCharacter * victim );
};

class CSkillPoisonArrow : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillPoisonArrow, CBaseSkill );

	virtual void	Use();
	virtual void	OnDoingDamage( CTakeDamageInfo &inputInfo, CBaseCombatCharacter * victim );
};
class CSkillTrick : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillTrick, CBaseSkill );

	virtual void	Use();
};

//Elite
class CSkillDropWall : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillDropWall, CBaseSkill );

	virtual void	Use();
	virtual void	SkillThink();
	virtual void	DeathNotice( CBaseEntity *pVictim );
		
private:
	float m_fSpawnTime;
	CHandle< CPhysicsProp > wall;
};
class CSkillDropTurret : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillDropTurret, CBaseSkill );

	virtual void	Use();
	virtual void	DeathNotice( CBaseEntity *pVictim );
		
private:
	float m_fSpawnTime;
};

//class CSkillAR2Ammo
class CSkillSummonElites : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillSummonElites, CBaseSkill );

	virtual void Use();
protected:
	virtual void PreNPCSpawn( CAI_BaseNPC *pNPC );
};

//Metropolice
class CSkillSummonManhacks : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillSummonManhacks, CBaseSkill );

	virtual void Use();
};

class CSkillInfect : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillInfect, CBaseSkill );

	virtual void	Use();
	virtual void 	DeActivate();
protected:
	CHandle< CHL2MP_Player > m_victim;
	float			m_oldSprint;
	float			m_oldNormal;
};



class CSkillSummonPolice : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillSummonPolice, CBaseSkill );

	virtual void Use();
	virtual void OnDoingDamage( CTakeDamageInfo &inputInfo, CBaseCombatCharacter * victim );
protected:
	virtual void PreNPCSpawn( CAI_BaseNPC *pNPC );
};

//Breen
class CSkillMassHeal : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillMassHeal, CBaseSkill );

	virtual void	Use();
};
class CSkillPersonalGuard : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillPersonalGuard, CBaseSkill );

	virtual void Use();
protected:
	virtual void PreNPCSpawn( CAI_BaseNPC *pNPC );
};
class CSkillColtAmmo : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillColtAmmo, CBaseSkill );

	virtual void Use();
};

class CSkillRallyTroops : public CBaseSkill
{
public:
	DECLARE_CLASS( CSkillRallyTroops, CBaseSkill );

	virtual void Use();
};
