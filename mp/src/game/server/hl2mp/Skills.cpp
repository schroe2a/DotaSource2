#include "cbase.h"
#include "Skills.h"
#include "util.h"
#include "ai_basenpc.h"
#include "ai_behavior_follow.h"
#include "datacache/imdlcache.h"
#include "ai_moveprobe.h"
#include "npc_citizen17.h"
#include "ammodef.h"
#include "npc_combines.h"
#include "npc_metropolice.h"
#include "HeroDef.h"
#include "te_effect_dispatch.h"
#include "ItemMoney.h"
#include "in_buttons.h"

LINK_ENTITY_TO_CLASS( dota_baseskill, CBaseSkill );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CBaseSkill, DT_BaseSkill )
	SendPropFloat( SENDINFO( m_iCooldown ) ),
	SendPropInt( SENDINFO( m_iLevel ) ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
END_SEND_TABLE()

static void DispatchActivate( CBaseEntity *pEntity )
{
	bool bAsyncAnims = mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, false );
	pEntity->Activate();
	mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, bAsyncAnims );
}

				CBaseSkill::CBaseSkill()
{
	m_iLevel = 0;
	
	m_iCooldown = 0.0f;
	m_bActiveEnds = 0.0f;
	
	m_iMaxLiveChildren = 0;
	m_nLiveChildren = 0;
	
	SetThink( &CBaseSkill::SkillThink );
	SetNextThink( gpGlobals->curtime );
}

void			CBaseSkill::Use()
{	
	ClientPrint( this->GetPlayer(), HUD_PRINTTALK, UTIL_VarArgs("%s is a passive skill\n", this->GetName() ) ); // write it on their screen
}
void			CBaseSkill::SkillThink( void )
{
	if ( m_iLevel && m_iCooldown != 0.0f && m_iCooldown < gpGlobals->curtime )
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "BaseCombatCharacter.AmmoPickup" );
	}

	if ( m_iLevel && m_bActiveEnds != 0.0f && m_bActiveEnds < gpGlobals->curtime )
	{
		m_bActiveEnds = 0.0f;
		ClientPrint( this->GetPlayer(), HUD_PRINTTALK, UTIL_VarArgs("%s Exhausted\n", this->GetName() ) ); // write it on their screen
		this->GetPlayer()->EmitSound( "Weapon_StunStick.Deactivate" );
		DeActivate();
	}
	
	SetNextThink( gpGlobals->curtime + 1.0f );
}
void			CBaseSkill::LevelUp()
{
	if( m_iLevel == 0 )
	{
		for( int i=0; i<=4; i++ )
			m_fCooldownduration[i] = HL2MPRules()->GetSkillCooldown( this->GetClassname(), i+1 );

		for( int i=0; i<=4; i++ )
			m_fActiveduration[i] = HL2MPRules()->GetSkillDuration( this->GetClassname(), i+1 );		
	}

	m_iLevel++;
}
CAI_BaseNPC *	CBaseSkill::SpawnNPC( const char * className, int health, bool front )
{
	CAI_BaseNPC	*pent = (CAI_BaseNPC*)CreateEntityByName( className );
	if ( !pent )
	{
		Warning("NULL Ent in player summon!\n" );
		return NULL;
	}
	
	bool placed = false;
	if ( front )
		placed = PlaceNPCInFront( pent, this->GetPlayer() );
	else
		placed = PlaceNPCInRadius( pent, this->GetPlayer() );

	if ( !placed )
	{
		// Failed to place the NPC. Abort
		UTIL_RemoveImmediate( (CBaseEntity *)pent );
		return NULL;
	}

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
	pent->AddSpawnFlags( SF_NPC_FADE_CORPSE );	

	pent->SetSquadName( AllocPooledString( "player_squad" ) );
	pent->ChangeTeam( this->GetPlayer()->GetTeamNumber() );

	CAI_FollowBehavior *pBehaviorFollow;
	if ( pent->GetBehavior( &pBehaviorFollow ) )
	{
		AI_FollowParams_t params; 			
		pBehaviorFollow->SetParameters(  params );
		pBehaviorFollow->SetFollowTarget( this->GetPlayer() );
	}	

	this->PreNPCSpawn( pent );
	
	DispatchSpawn( pent );

	pent->CapabilitiesAdd( bits_CAP_NO_HIT_SQUADMATES | bits_CAP_FRIENDLY_DMG_IMMUNE );

	pent->SetOwnerEntity( this );
	
	if ( health > 0 )
		pent->SetHealth( health );
	
	DispatchActivate( pent );

	m_nLiveChildren++;

	return pent;
}

bool			CBaseSkill::PlaceNPCInFront( CAI_BaseNPC *pNPC, CBaseEntity * target, bool behind )
{
	Vector vPos;

	Vector forward;
	AngleVectors( target->GetAbsAngles(), &forward );
	Vector vStart = target->GetAbsOrigin();

	forward *= 50;
	if ( behind )
		forward *= -1;	

	vPos = vStart + forward;	

	trace_t tr;

	UTIL_TraceLine( vPos, vPos - Vector( 0, 0, 8192 ), MASK_SHOT, pNPC, COLLISION_GROUP_NONE, &tr );
	if( tr.fraction != 1.0 )
	{
		UTIL_TraceHull( tr.endpos,
						tr.endpos + Vector( 0, 0, 10 ),
						pNPC->GetHullMins(),
						pNPC->GetHullMaxs(),
						MASK_NPCSOLID,
						pNPC,
						COLLISION_GROUP_NONE,
						&tr );

		if( tr.fraction == 1.0 && pNPC->GetMoveProbe()->CheckStandPosition( tr.endpos, MASK_NPCSOLID ) )
		{
			pNPC->SetAbsOrigin( tr.endpos );
			pNPC->SetAbsAngles( target->GetAbsAngles() );
			return true;
		}
	}

	DevMsg("**Failed to place NPC in front!\n");
	return false;
}
bool			CBaseSkill::PlaceNPCInRadius( CAI_BaseNPC *pNPC, CBaseEntity * target )
{
	Vector vPos;

	if ( CAI_BaseNPC::FindSpotForNPCInRadius( &vPos, target->GetAbsOrigin(), pNPC, 100 ) )
	{
		pNPC->SetAbsOrigin( vPos );
		return true;
	}

	DevMsg("**Failed to place NPC in radius!\n");
	return false;
}

CPhysicsProp *	CBaseSkill::SpawnPhysicsProp( const char * pModelName, Vector vecSrc )
{
	CPhysicsProp *pProp = NULL;

	MDLCACHE_CRITICAL_SECTION();

	MDLHandle_t h = mdlcache->FindMDL( pModelName );
	if ( h == MDLHANDLE_INVALID )
		return NULL;

	// Must have vphysics to place as a physics prop
	studiohdr_t *pStudioHdr = mdlcache->GetStudioHdr( h );
	if ( !pStudioHdr )
		return NULL;

	// Must have vphysics to place as a physics prop
	if ( !mdlcache->GetVCollide( h ) )
		return NULL;

	Vector vecSweepMins = pStudioHdr->hull_min;
	Vector vecSweepMaxs = pStudioHdr->hull_max;

	Vector	vecEye = this->GetPlayer()->EyePosition();

	trace_t tr;
	UTIL_TraceHull( vecEye, vecSrc, vecSweepMins, vecSweepMaxs, this->GetPlayer()->PhysicsSolidMaskForEntity(), this->GetPlayer(), this->GetPlayer()->GetCollisionGroup(), &tr );	
	if ( tr.DidHit() )
	{
		vecSrc = tr.endpos;
	}

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	pProp = assert_cast<CPhysicsProp*>(CreateEntityByName( "prop_physics" ));	
	if ( pProp )
	{
		char buf[512];
		Q_snprintf( buf, sizeof(buf), "%.10f %.10f %.10f", vecSrc.x, vecSrc.y, vecSrc.z );
		pProp->KeyValue( "origin", buf );
		QAngle angles = this->GetPlayer()->GetAbsAngles();
		Q_snprintf( buf, sizeof(buf), "%.10f %.10f %.10f", angles.x, angles.y, angles.z );
		pProp->KeyValue( "angles", buf );
		pProp->SetModel( pModelName );	
		pProp->KeyValue( "inertiaScale", "1.0" );
		pProp->Precache();

		pProp->SetOwnerEntity( this );

		PrePysicsPropSpawn( pProp );

		DispatchSpawn( pProp );								
		DispatchActivate( pProp );
	}

	CBaseEntity::SetAllowPrecache( bAllowPrecache );			

	m_nLiveChildren++;

	return pProp;
}
void			CBaseSkill::DeathNotice( CBaseEntity *pVictim )
{
	// ok, we've gotten the deathnotice from our child
	m_nLiveChildren--;

	// If we're here, we're getting erroneous death messages from children we haven't created
	AssertMsg( m_nLiveChildren >= 0, "player receiving child death notice but thinks has no children\n" );	
}

bool			CBaseSkill::UseSkill()
{
	if ( this->m_iLevel <= 0 || this->m_iCooldown > 0.0f )
	{
		if (this->GetPlayer()->IsAlive()) { // Issue #18: AMP - 2013-10-04 - Fix repeating UseDeny sound after you die
			this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
		}
		return false;
	}

	m_iCooldown = gpGlobals->curtime + m_fCooldownduration[m_iLevel-1];

	if ( m_fActiveduration[m_iLevel-1] > 0.0f )
	{
		this->GetPlayer()->EmitSound( "Weapon_StunStick.Activate" );
		ClientPrint( this->GetPlayer(), HUD_PRINTTALK, UTIL_VarArgs("%s Activated\n", this->GetName() ) ); // write it on their screen
		m_bActiveEnds = gpGlobals->curtime + m_fActiveduration[m_iLevel-1];
	}

	return true;
}
//Freman
LINK_ENTITY_TO_CLASS( dota_skill_stun, CSkillStun );
void CSkillStun::OnDoingDamage( CTakeDamageInfo &inputInfo, CBaseCombatCharacter * victim )
{
	if ( inputInfo.GetDamageType() & DMG_CLUB ) 
	{
		CHL2MP_Player * playerVictim = ToHL2MPPlayer( victim );
		if ( playerVictim )
		{
			int rand = random->RandomInt( 1, 4 );
			if ( rand <= m_iLevel ) 
			{
				QAngle vecDeltaAngles;
				Vector vecPushDir;
				Vector vecPush;
				QAngle a;

				switch ( rand )
				{
				case 1:
					vecDeltaAngles.x = -30;
					vecDeltaAngles.y = 30;
					vecDeltaAngles.z = 0.0f;

					playerVictim->SnapEyeAngles( GetLocalAngles() + vecDeltaAngles );
					playerVictim->ViewPunch( vecDeltaAngles );
					break;

				case 2:
					vecPushDir = ( playerVictim->BodyTarget( this->GetPlayer()->GetAbsOrigin(), false ) - this->GetPlayer()->GetAbsOrigin() );
						
					vecPush = (vecPushDir * 100);
					vecPush.z += 10;
					if ( playerVictim->GetFlags() & FL_BASEVELOCITY )
					{
						vecPush = vecPush + playerVictim->GetBaseVelocity();
					}
					if ( vecPush.z > 0 && (playerVictim->GetFlags() & FL_ONGROUND) )
					{
						playerVictim->SetGroundEntity( NULL );
						Vector origin = playerVictim->GetAbsOrigin();
						origin.z += 1.0f;
						playerVictim->SetAbsOrigin( origin );
					}

					playerVictim->SetBaseVelocity( vecPush );
					playerVictim->AddFlag( FL_BASEVELOCITY );
					break;

				case 3:
					playerVictim->SetLocalAngles( this->GetPlayer()->GetLocalAngles() );
					playerVictim->m_Local.m_vecPunchAngle = vec3_angle;
					playerVictim->m_Local.m_vecPunchAngleVel = vec3_angle;
					playerVictim->SnapEyeAngles( this->GetPlayer()->GetLocalAngles() );					
					break;
				case 4:
					a = this->GetPlayer()->GetLocalAngles();
					a.x -= 89;
					playerVictim->SetLocalAngles( a );
					playerVictim->m_Local.m_vecPunchAngle = vec3_angle;
					playerVictim->m_Local.m_vecPunchAngleVel = vec3_angle;
					playerVictim->SnapEyeAngles( a );					
					break;
				}
			}
		}
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_find_health, CSkillFindHealth );
LINK_ENTITY_TO_CLASS( item_freeman_healthvial, CFreemanHealthVial );
PRECACHE_REGISTER( item_freeman_healthvial );
void CSkillFindHealth::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	if ( m_iLevel && pVictim->ClassMatches( "npc_creep" ) )
	{
		CFreemanHealthVial * vial = (CFreemanHealthVial*)CBaseEntity::Create( "item_freeman_healthvial", pVictim->GetAbsOrigin(), pVictim->GetAbsAngles() );
		if ( vial )
		{
			vial->ChangeTeam( this->GetPlayer()->GetTeamNumber() );
			vial->m_healthAmount = HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel );
			if( m_iLevel <= 3 )
				vial->m_taker = this->GetPlayer();
		}
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_find_armor, CSkillFindArmor );
LINK_ENTITY_TO_CLASS(item_freeman_battery, CFreemanBattery);
PRECACHE_REGISTER(item_freeman_battery);
void CSkillFindArmor::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	if ( m_iLevel && pVictim->ClassMatches( "npc_creep" ) )
	{
		CFreemanBattery * battery = (CFreemanBattery*)CBaseEntity::Create( "item_freeman_battery", pVictim->GetAbsOrigin(), pVictim->GetAbsAngles() );
		if ( battery )
		{
			battery->ChangeTeam( this->GetPlayer()->GetTeamNumber() );
			battery->m_armorAmount = HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel );
			if( m_iLevel <= 3 )
				battery->m_taker = this->GetPlayer();
		}
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_reload, CSkillReload );
bool CSkillReload::CanSkipRespawnPenilty() 
{ 
	if ( UseSkill() )
	{
		m_iCooldown = 0.0f;
		return true;
	}
	return false; 
}
void CSkillReload::SkipRespawnPenilty() 
{  
	UseSkill();
}
//Alyx
LINK_ENTITY_TO_CLASS( dota_skill_speed_boost, CSkillSpeedBoost );
void CSkillSpeedBoost::Use()
{
	if ( !this->UseSkill() )
	{
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
		return;
	}

	this->GetPlayer()->maxNormalSpeed *= HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel );
	this->GetPlayer()->maxSprintSpeed *= HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel );
}
void CSkillSpeedBoost::DeActivate()
{
	this->GetPlayer()->maxNormalSpeed = 190;
	this->GetPlayer()->maxSprintSpeed = 320;

	this->GetPlayer()->m_nButtons &= ~IN_SPEED;
}
LINK_ENTITY_TO_CLASS( dota_skill_evasion, CSkillEvasion );
void CSkillEvasion::OnTakeDamage( CTakeDamageInfo &inputInfo )
{
	if ( inputInfo.GetDamageType() & DMG_BULLET ) 
	{
		if ( random->RandomInt( 1, 100 ) <= HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ) )
		{
			inputInfo.SetDamage( 0.0f );
			ClientPrint( this->GetPlayer(), HUD_PRINTTALK, UTIL_VarArgs("Dodge!\n", (int)( m_iCooldown - gpGlobals->curtime ) ) ); // write it on their screen

			CHL2MP_Player * attacker = ToHL2MPPlayer( inputInfo.GetAttacker() );
			if ( attacker )
				ClientPrint( attacker, HUD_PRINTTALK, UTIL_VarArgs("MISS!\n", (int)( m_iCooldown - gpGlobals->curtime ) ) ); // write it on their screen
		}
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_sneeky, CSkillSneeky );
void CSkillSneeky::LevelUp()
{
	SetAlpha();
	BaseClass::LevelUp();
}
void CSkillSneeky::OnPlayerSpawn()
{
	SetAlpha();
}
void CSkillSneeky::SetAlpha()
{
	this->GetPlayer()->SetRenderMode( kRenderTransColor );
	this->GetPlayer()->SetRenderColorA( 255 - HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ) );
}

LINK_ENTITY_TO_CLASS( dota_skill_summondog, CSkillSummonDog );
void CSkillSummonDog::Use()
{
	if ( !this->UseSkill() )
		return;
	
	if ( (dog = this->SpawnNPC( "npc_attack_dog", HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ), true )) == NULL )
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
	}
}
void CSkillSummonDog::DeActivate()
{
	if( dog )
	{
		CTakeDamageInfo info;
		info.SetDamage( 1 );
		info.SetDamageType( DMG_CRUSH );
		dog->Event_Killed( info );
	}
}
//Barney
LINK_ENTITY_TO_CLASS( dota_skill_throw_explosive, CSkillThrowExplosive );
void CSkillThrowExplosive::Use()
{
	if ( !this->UseSkill() )
		return;

	Vector	vecEye = this->GetPlayer()->EyePosition();
	Vector	vForward, vRight;
	this->GetPlayer()->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;

	CPhysicsProp *pProp = this->SpawnPhysicsProp( "models/props_c17/canister01a.mdl", vecSrc );
	if ( pProp )
	{
		vForward[2] += 0.1f; 
		Vector vecThrow;
		this->GetPlayer()->GetVelocity( &vecThrow, NULL );
		vecThrow += vForward * 600;

		Vector vecSpin;
		vecSpin.x = random->RandomFloat( -500.0, 500.0 );
		vecSpin.y = random->RandomFloat( -500.0, 500.0 );
		vecSpin.z = random->RandomFloat( -500.0, 500.0 );

		IPhysicsObject *pPhysicsObject = pProp->VPhysicsGetObject();
		if ( pPhysicsObject )
		{
			pPhysicsObject->AddVelocity( &vecThrow, &vecSpin );
		}
		pProp->SetHealth( 20 );
		pProp->SetExplosiveDamage( 120.0f );
		pProp->SetExplosiveRadius( 256.0f );
		pProp->m_takedamage = DAMAGE_YES;
	}
	else
	{
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
		m_iCooldown = 0.0f;
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_take_bribe, CSkillTakeBribe );
void CSkillTakeBribe::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	if ( (info.GetDamageType() & DMG_CLUB) || (info.GetDamageType() & DMG_SLASH) ) 
	{
		if ( pVictim->ClassMatches( "npc_creep" ) )
		{
			DropMoney( pVictim->GetAbsOrigin(), HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ), this->GetPlayer() );
		}
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_smg_ammo, CSkillSMGAmmo );
void CSkillSMGAmmo::Use()
{
	if ( !this->UseSkill() )
		return;

	int iAmmoType = GetAmmoDef()->Index("SMG1");	
	this->GetPlayer()->GiveAmmo( HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ), iAmmoType, false );
	this->GetPlayer()->GiveNamedItem( "weapon_smg1" );
}
LINK_ENTITY_TO_CLASS( dota_skill_infiltrate, CSkillInfiltrate );
void CSkillInfiltrate::Use()
{
	if ( !this->UseSkill() )
		return;
}
Disposition_t CSkillInfiltrate::IRelationType( CAI_BaseNPC * source )
{
	if ( m_bActiveEnds > gpGlobals->curtime )
		return D_LI;
	
	return BaseClass::IRelationType( source );
}
//Eli
LINK_ENTITY_TO_CLASS( dota_skill_heal, CSkillHeal );
void CSkillHeal::Use()
{
	if ( !this->UseSkill() )
		return;

	Vector	vecEye = this->GetPlayer()->EyePosition();
	Vector	vForward, vRight;
	this->GetPlayer()->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;		

	CFreemanHealthVial * vial = (CFreemanHealthVial*)CBaseEntity::Create( "item_freeman_healthvial", vecSrc, vec3_angle );
	if ( vial )
	{
		vial->ChangeTeam( this->GetPlayer()->GetTeamNumber() );
		vial->m_healthAmount = HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel );
		vial->m_taker = this->GetPlayer();
		vial->m_excludeTaker = true;
		
		vForward[2] += 0.1f;
		Vector vecThrow;
		this->GetPlayer()->GetVelocity( &vecThrow, NULL );
		vecThrow += vForward * 120;

		Vector vecSpin;
		vecSpin.x = random->RandomFloat( -1000.0, 1000.0 );
		vecSpin.y = random->RandomFloat( -1000.0, 1000.0 );
		vecSpin.z = random->RandomFloat( -1000.0, 1000.0 );

		IPhysicsObject *pPhysicsObject = vial->VPhysicsGetObject();
		if ( pPhysicsObject )
		{
			pPhysicsObject->AddVelocity( &vecThrow, &vecSpin );
		}		
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_ar2_ammo, CSkillAR2Ammo );
void CSkillAR2Ammo::Use()
{
	if ( !this->UseSkill() )
		return;

	int iAmmoType = GetAmmoDef()->Index("AR2");	
	this->GetPlayer()->GiveAmmo( HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ), iAmmoType, false );
	this->GetPlayer()->GiveNamedItem( "weapon_ar2" );
}
LINK_ENTITY_TO_CLASS( dota_skill_fake_death, CSkillFakeDeath );
void CSkillFakeDeath::Use()
{
	if ( !this->UseSkill() )
		return;

	this->GetPlayer()->CreateRagdollEntity();

	CTakeDamageInfo info;
	this->GetPlayer()->DeathSound( info );

	this->GetPlayer()->SetRenderMode( kRenderTransColor );
	this->GetPlayer()->SetRenderColorA( 0 );
}
void CSkillFakeDeath::DeActivate()
{
	this->GetPlayer()->SetRenderMode( kRenderNormal );
	this->GetPlayer()->SetRenderColorA( 255 );
}
LINK_ENTITY_TO_CLASS( dota_skill_summonrebs, CSkillSummonRebs );
void CSkillSummonRebs::Use()
{
	if ( !this->UseSkill() )
		return;

	CAI_BaseNPC * reb = this->SpawnNPC("npc_citizen", 100, true );
	if ( reb )
	{
		reb = this->SpawnNPC("npc_citizen", 100, false);
		reb = this->SpawnNPC("npc_citizen", 100, true );

		if ( m_iLevel >= 2 )
		{
			spawnFlags = SF_CITIZEN_MEDIC; 
			reb = this->SpawnNPC("npc_citizen", 100, false);
			reb = this->SpawnNPC("npc_citizen", 100, false);
			reb = this->SpawnNPC("npc_citizen", 100, false);
		}
		if ( m_iLevel >= 3 )
		{
			spawnFlags = SF_CITIZEN_AMMORESUPPLIER;
			reb = this->SpawnNPC("npc_citizen", 100, false );
			reb = this->SpawnNPC("npc_citizen", 100, false );
			reb = this->SpawnNPC("npc_citizen", 100, false );
		}
	}
	else
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
	}
}
void CSkillSummonRebs::PreNPCSpawn( CAI_BaseNPC *pNPC ) 
{
	if ( spawnFlags )
	{
		pNPC->AddSpawnFlags( spawnFlags );
	}

	pNPC->m_spawnEquipment = MAKE_STRING("weapon_smg1");
}
//Monk
LINK_ENTITY_TO_CLASS( dota_skill_shotgun_ammo, CSkillShotgunAmmo );
void CSkillShotgunAmmo::Use()
{
	if ( !this->UseSkill() )
		return;

	int iAmmoType = GetAmmoDef()->Index("Buckshot");	
	this->GetPlayer()->GiveAmmo( HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ), iAmmoType, false );
	this->GetPlayer()->GiveNamedItem( "weapon_shotgun" );
}

LINK_ENTITY_TO_CLASS( dota_skill_summonheadcrab, CSkillSummonHeadcrab );
void CSkillSummonHeadcrab::Use()
{
	if ( !this->UseSkill() )
		return;

	char type[20];
	if ( m_iLevel < 3 )
		Q_strcpy(type, "npc_headcrab");
	else
		Q_strcpy(type, "npc_headcrab_fast");

	CAI_BaseNPC *  spawnWorked = this->SpawnNPC(type, 0, true);
	if ( spawnWorked )
	{
		for( int i=0; i<5; i++ )
		{
			this->SpawnNPC(type, 50, false);
		}

		if ( m_iLevel >= 2 )
		{
			for( int i=0; i<6; i++ )
			{
				this->SpawnNPC(type, 50, false);
			}
		}
		if ( m_iLevel >= 4 )
		{
			for( int i=0; i<6; i++ )
			{
				this->SpawnNPC(type, 50, false);
			}
		}
	}
	else
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_zombie_invasion, CSkillZombieInvasion );
void CSkillZombieInvasion::Use()
{
	if ( !this->UseSkill() )
		return;

	for ( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt(pEntity) )
	{
		CHL2MP_Player * player = ToHL2MPPlayer( pEntity );
		if ( player )
		{
			if ( player->IsAlive() && player->GetTeamNumber() != this->GetPlayer()->GetTeamNumber() )
			{
				this->m_currentPlayer = player;
				this->SpawnNPC("npc_zombie", HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ), false);
			}
		}
	}
}
void CSkillZombieInvasion::PreNPCSpawn( CAI_BaseNPC *pNPC ) 
{
	if ( m_currentPlayer )
	{
		if ( !this->PlaceNPCInFront( pNPC, m_currentPlayer, true ) )
			this->PlaceNPCInRadius( pNPC, m_currentPlayer );
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_summonzombies, CSkillSummonZombies );
void CSkillSummonZombies::Use()
{
	if ( !this->UseSkill() )
		return;

	CAI_BaseNPC * spawnWorked = false;
	if ( m_iLevel == 1 )
		spawnWorked = this->SpawnNPC("npc_zombie", 100, true);
	else if ( m_iLevel == 2 )
		spawnWorked = this->SpawnNPC("npc_poisonzombie", 200, true);
	else if ( m_iLevel == 3 )
		spawnWorked = this->SpawnNPC("npc_fastzombie", 300, true);				

	if ( spawnWorked )
	{
		for ( int i = 0; i < 20; i++ )
		{
			if ( m_nLiveChildren >= 4 )
				break;

			if ( m_iLevel == 1 )				
				this->SpawnNPC("npc_zombie", 100, false);
			else if ( m_iLevel == 2 )
				this->SpawnNPC("npc_poisonzombie", 200, false);
			else if ( m_iLevel == 3 )
				this->SpawnNPC("npc_fastzombie", 300, false);
		}
	}
	else
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
	}
}
//Gman
LINK_ENTITY_TO_CLASS( dota_skill_phonehome, CSkillPhoneHome );
void CSkillPhoneHome::Use()
{
	if ( !this->UseSkill() )
		return;
		
	HL2MPRules()->GetPlayerSpawnSpot( this->GetPlayer() );
}
LINK_ENTITY_TO_CLASS( dota_skill_swap, CSkillSwap );
void CSkillSwap::Use()
{
	if ( !this->UseSkill() )
		return;

	Vector forward;
	this->GetPlayer()->EyeVectors( &forward );

	//int distance = (5 + ( m_iLevel * 5)) * 36;

	trace_t tr;
	AI_TraceLine(this->GetPlayer()->EyePosition(), this->GetPlayer()->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_SHOT, this->GetPlayer(), COLLISION_GROUP_NONE, &tr);

	if ( tr.m_pEnt && (tr.m_pEnt->IsPlayer() || tr.m_pEnt->ClassMatches( "npc_creep" ) ) )
	{
		Vector	origin = this->GetPlayer()->GetAbsOrigin();
		QAngle  orintation = this->GetPlayer()->GetAbsAngles();

		this->GetPlayer()->SetAbsOrigin( tr.m_pEnt->GetAbsOrigin() );
		this->GetPlayer()->SetAbsAngles( tr.m_pEnt->GetAbsAngles() );

		tr.m_pEnt->SetAbsOrigin(  origin );
		tr.m_pEnt->SetAbsAngles( orintation );
	}
	else
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_blink, CSkillBlink );
void CSkillBlink::Use()
{
	if ( !this->UseSkill() )
		return;

	Vector forward;
	this->GetPlayer()->EyeVectors( &forward );

	int distance = HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel );

	trace_t tr;
	UTIL_TraceLine( this->GetPlayer()->EyePosition(), this->GetPlayer()->EyePosition() + forward * distance, MASK_NPCSOLID, this->GetPlayer(), COLLISION_GROUP_NONE, &tr );

	if ( tr.allsolid )
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
		return;
	}		

	this->GetPlayer()->SetLocalOrigin( tr.endpos );
	this->GetPlayer()->SetAbsVelocity( vec3_origin );	
}
LINK_ENTITY_TO_CLASS( dota_skill_time_lapse, CSkillTimeLapse );
void CSkillTimeLapse::LevelUp()
{
	if ( m_iLevel == 0 )
	{
		for ( int i = 0; i<5; i++ )
		{
			past[i].health = this->GetPlayer()->GetHealth();
			past[i].armor = this->GetPlayer()->ArmorValue();
			past[i].origin = this->GetPlayer()->GetLocalOrigin();
			past[i].orintation = this->GetPlayer()->GetLocalAngles();
		}
	}

	BaseClass::LevelUp();
}
void CSkillTimeLapse::Use()
{
	if ( !this->UseSkill() )
		return;

	this->GetPlayer()->SetHealth(past[4].health);
	this->GetPlayer()->SetArmorValue(past[4].armor);
	this->GetPlayer()->SetLocalOrigin(past[4].origin);
	this->GetPlayer()->SetLocalAngles(past[4].orintation);
	
}
void CSkillTimeLapse::SkillThink()
{
	if ( this->GetPlayer()->IsAlive() )
	{
		for ( int i = 4; i>0; i-- )
		{
			past[i] = past[i-1];
		}

		past[0].health = this->GetPlayer()->GetHealth();
		past[0].armor = this->GetPlayer()->ArmorValue();
		past[0].origin = this->GetPlayer()->GetAbsOrigin();
		past[0].orintation = this->GetPlayer()->GetAbsAngles();
	}

	BaseClass::SkillThink();
}
//Mossman
LINK_ENTITY_TO_CLASS( dota_skill_backstab, CSkillBackstab );
void CSkillBackstab::OnDoingDamage( CTakeDamageInfo &inputInfo, CBaseCombatCharacter * victim )
{	
	if ( !victim->FInViewCone( this->GetPlayer() ) ) 
	{
		inputInfo.AddDamage( inputInfo.GetDamage() * HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ) );
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_poison, CSkillPoisonArrow );
void CSkillPoisonArrow::Use()
{
	if ( !this->UseSkill() )
		return;
}
void CSkillPoisonArrow::OnDoingDamage( CTakeDamageInfo &inputInfo, CBaseCombatCharacter * victim )
{
	if ( inputInfo.GetDamageType() & DMG_NEVERGIB ) 
	{
		CHL2MP_Player * playerVictim = ToHL2MPPlayer( victim );
		if ( playerVictim )
		{
			if ( m_bActiveEnds > gpGlobals->curtime )
			{
				m_bActiveEnds = 0.0f;				
				
				int victimRemainingHealthTarget = playerVictim->GetMaxHealth() * HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel );
				int damageToDo = max(1, (playerVictim->m_iHealth - inputInfo.GetDamage()) - victimRemainingHealthTarget);
								
				playerVictim->TakeDamage( CTakeDamageInfo( this, this, damageToDo, DMG_POISON ) );

				ClientPrint( this->GetPlayer(), HUD_PRINTTALK, UTIL_VarArgs("Poison Applied!\n", (int)( m_iCooldown - gpGlobals->curtime ) ) ); // write it on their screen
				ClientPrint( playerVictim, HUD_PRINTTALK, UTIL_VarArgs("You were Poisoned!\n", (int)( m_iCooldown - gpGlobals->curtime ) ) ); // write it on their screen
			}
		}
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_trick, CSkillTrick );
void CSkillTrick::Use()
{
	if ( !this->UseSkill() )
		return;
	
	Vector forward;
	this->GetPlayer()->EyeVectors( &forward );

	trace_t tr;
	AI_TraceLine(this->GetPlayer()->EyePosition(), this->GetPlayer()->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_SHOT, this->GetPlayer(), COLLISION_GROUP_NONE, &tr);

	if ( tr.m_pEnt && tr.m_pEnt->ClassMatches( "npc_creep" ) )
	{
		tr.m_pEnt->ChangeTeam( this->GetPlayer()->GetTeamNumber() );
	}
	else
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
	}
}

//Elite
LINK_ENTITY_TO_CLASS( dota_skill_drop_wall, CSkillDropWall );
void CSkillDropWall::Use()
{
	if ( !this->UseSkill() )
		return;

	Vector forward;
	AngleVectors( this->GetPlayer()->GetAbsAngles(), &forward );
	Vector vStart = this->GetPlayer()->GetAbsOrigin();
	
	wall = this->SpawnPhysicsProp( "models/props_combine/combine_barricade_short01a.mdl", vStart + forward * 50 );
	if ( wall )
	{
		wall->SetHealth( HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ) );
		wall->m_takedamage = DAMAGE_YES;
		m_fSpawnTime = gpGlobals->curtime;
		wall->AddEFlags( EFL_NO_DAMAGE_FORCES );
	}
	else
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
	}
}
void CSkillDropWall::SkillThink()
{
	if( wall )
	{
		Vector	up;
		wall->GetVectors( NULL, NULL, &up );

		if( DotProduct( up, Vector(0,0,1) ) < 0.5f )
		{
			CTakeDamageInfo	info;
			info.SetDamage( 1 );
			info.SetDamageType( DMG_CRUSH );
			wall->Event_Killed( info );
		}
	}

	BaseClass::SkillThink();
}
void CSkillDropWall::DeathNotice( CBaseEntity *pVictim )
{
	if ( (gpGlobals->curtime - m_fSpawnTime) < 10 )
	{
		m_fSpawnTime = 0.0f;
		m_iCooldown = 0.0f;
	}

	BaseClass::DeathNotice( pVictim );
}
LINK_ENTITY_TO_CLASS( dota_skill_drop_turret, CSkillDropTurret );
void CSkillDropTurret::Use()
{
	if ( !this->UseSkill() )
		return;
	
	if ( this->SpawnNPC("npc_turret_floor", 0, true) )
	{
		m_fSpawnTime = gpGlobals->curtime;
	}
	else
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
	}
}
void CSkillDropTurret::DeathNotice( CBaseEntity *pVictim )
{
	if ( (gpGlobals->curtime - m_fSpawnTime) < 10 )
	{
		m_fSpawnTime = 0.0f;
		m_iCooldown = 0.0f;
	}

	BaseClass::DeathNotice( pVictim );
}
LINK_ENTITY_TO_CLASS( dota_skill_summon_elites, CSkillSummonElites );
void CSkillSummonElites::Use()
{
	if ( !this->UseSkill() )
		return;

	CAI_BaseNPC * elite = this->SpawnNPC( "npc_combine_s", m_iLevel * 100, false );
	if ( elite )
	{
		for ( int i = 1; i <= 3; i++ )
		{
			this->SpawnNPC( "npc_combine_s", m_iLevel * 100, false );
		}			
	}
	else
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
	}
}
void CSkillSummonElites::PreNPCSpawn( CAI_BaseNPC *pNPC ) 
{
	pNPC->SetModelName( AllocPooledString("models/combine_super_soldier.mdl") );
	pNPC->m_spawnEquipment = MAKE_STRING("weapon_ar2");

	CBaseEntity *ent = NULL;

	if ( m_iLevel == 1 )
	{
		bool valid = false;
		CBaseEntity * pSpot = NULL;
		while ( ( pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_combine" )) != NULL )
		{
			for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 128 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
			{
				// if ent is a client, don't spawn on 'em
				if ( ent->IsPlayer() && ent != pNPC )
					continue;
			}
			valid = true;
			break;
		}	
		if ( valid )
		{
			pNPC->SetAbsOrigin( pSpot->GetAbsOrigin() + Vector(0,0,1) );
		}
	}
	else if ( m_iLevel == 3 )
	{
		Vector forward;
		this->GetPlayer()->EyeVectors( &forward );

		trace_t tr;
		UTIL_TraceHull( this->GetPlayer()->EyePosition(), this->GetPlayer()->EyePosition() + forward * MAX_TRACE_LENGTH, 
			pNPC->GetHullMins(), pNPC->GetHullMaxs(), MASK_NPCSOLID, this->GetPlayer(), COLLISION_GROUP_NONE, &tr );
		
		if ( tr.fraction != 1.0 && !tr.allsolid )
		{
			pNPC->SetAbsOrigin( tr.endpos + Vector(0,0,1) );
		}
	}
}
//Metropolice
LINK_ENTITY_TO_CLASS( dota_skill_summonmanhacks, CSkillSummonManhacks );
void CSkillSummonManhacks::Use()
{
	if ( !this->UseSkill() )
		return;

	if ( this->SpawnNPC("npc_manhack", 75, true) )
	{
		for ( int i = 0; i < 20; i++ )
		{
			if ( m_nLiveChildren >= HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ))
				break;

			this->SpawnNPC("npc_manhack", 75, false);
		}
	}
	else
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_infect, CSkillInfect );
void CSkillInfect::Use()
{
	if ( !this->UseSkill() )
		return;
	
	Vector forward;
	this->GetPlayer()->EyeVectors( &forward );

	trace_t tr;
	AI_TraceLine(this->GetPlayer()->EyePosition(), this->GetPlayer()->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_SHOT, this->GetPlayer(), COLLISION_GROUP_NONE, &tr);

	CHL2MP_Player *player = ToHL2MPPlayer(tr.m_pEnt);
	if ( player )
	{
		m_victim = player;
		m_oldSprint = player->maxSprintSpeed;
		m_oldNormal = player->maxNormalSpeed;
		player->maxSprintSpeed = 100;
		player->maxNormalSpeed = 100;		

		ClientPrint( m_victim, HUD_PRINTTALK, UTIL_VarArgs("Infected!\n", (int)( m_iCooldown - gpGlobals->curtime ) ) ); // write it on their screen
	}
	else
	{
		m_iCooldown = 0.0f;
		this->GetPlayer()->EmitSound( "HL2Player.UseDeny" );
	}
}
void CSkillInfect::DeActivate()
{
	if ( m_victim )
	{
		ClientPrint( m_victim, HUD_PRINTTALK, UTIL_VarArgs("Infection cleared\n", (int)( m_iCooldown - gpGlobals->curtime ) ) ); // write it on their screen

		m_victim->maxSprintSpeed = m_oldSprint;
		m_victim->maxNormalSpeed = m_oldNormal;
		m_victim = NULL;
	}
}

LINK_ENTITY_TO_CLASS( dota_skill_summon_police, CSkillSummonPolice );
void CSkillSummonPolice::Use()
{
	if ( !this->UseSkill() )
		return;

	for ( int i = 1; i <= 4; i++ )
	{
		SpawnNPC( "npc_metropolice", HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ), false );
	}
}
void CSkillSummonPolice::PreNPCSpawn( CAI_BaseNPC *pNPC ) 
{
	CNPC_MetroPolice * police = (CNPC_MetroPolice*)pNPC;
	police->m_spawnEquipment = MAKE_STRING("weapon_stunstick");
	police->m_iManhacks = m_iLevel;
}
void CSkillSummonPolice::OnDoingDamage( CTakeDamageInfo &inputInfo, CBaseCombatCharacter * victim )
{	
	if ( inputInfo.GetDamageType() & DMG_CLUB ) 
	{
		if ( random->RandomInt( 1, 100 ) <= (m_iLevel * 10) )
		{
			if( victim->GetActiveWeapon() )
				victim->GetActiveWeapon()->Holster();
		}
	}
}
//Breen
LINK_ENTITY_TO_CLASS( dota_skill_mass_heal, CSkillMassHeal );
void CSkillMassHeal::Use()
{
	if ( !this->UseSkill() )
		return;

	CBaseEntity *pTarget = NULL;
	while ( ( pTarget = gEntList.FindEntityInSphere( pTarget, this->GetPlayer()->GetAbsOrigin(), 500.0f ) ) != NULL )
	{
		if ( pTarget->GetTeamNumber() == this->GetPlayer()->GetTeamNumber() )
		{
			pTarget->m_iHealth = min( pTarget->GetMaxHealth(), pTarget->GetMaxHealth() + (pTarget->GetMaxHealth() * (0.5f * m_iLevel)));

			if ( m_iLevel > 2 )
			{
				CHL2MP_Player *player = ToHL2MPPlayer(pTarget);
				if ( player )
				{
					player->SetArmorValue( min( pTarget->GetMaxHealth(), player->ArmorValue() + (pTarget->GetMaxHealth() * (0.5f * m_iLevel-2))));
				}
			}

			CEffectData data;
			data.m_fFlags = 0;
			data.m_vOrigin = pTarget->GetAbsOrigin();
			data.m_vNormal = Vector(0,0,1);
			data.m_vAngles = QAngle( 0, 0, 0 );			
			data.m_flScale = random->RandomFloat( 6, 8 );
			DispatchEffect( "watersplash", data );			
		}
	}
}
LINK_ENTITY_TO_CLASS( dota_skill_personal_guard, CSkillPersonalGuard );
void CSkillPersonalGuard::Use()
{
	if ( !this->UseSkill() )
		return;

	for ( int i = 1; i <= m_iLevel; i++ )
	{
		this->SpawnNPC( "npc_combine_s", HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ), false );
	}
}
void CSkillPersonalGuard::PreNPCSpawn( CAI_BaseNPC *pNPC ) 
{	
	pNPC->m_spawnEquipment = MAKE_STRING("weapon_smg1");
}	

LINK_ENTITY_TO_CLASS( dota_skill_colt_ammo, CSkillColtAmmo );
void CSkillColtAmmo::Use()
{
	if ( !this->UseSkill() )
		return;

	int iAmmoType = GetAmmoDef()->Index("357");	
	this->GetPlayer()->GiveAmmo( HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ), iAmmoType, false );
	this->GetPlayer()->GiveNamedItem( "weapon_357" );
}
LINK_ENTITY_TO_CLASS( dota_skill_rallytroops, CSkillRallyTroops );
void CSkillRallyTroops::Use()
{
	if ( !this->UseSkill() )
		return;

	this->GetPlayer()->EmitSound("breencast.br_overwatch07");

	CBaseEntity *pEntity = NULL;
	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "npc_creep" )) != NULL)
	{
		if ( pEntity->GetTeamNumber() == this->GetPlayer()->GetTeamNumber() )
		{			
			pEntity->SetHealth( pEntity->GetMaxHealth() + ( pEntity->GetMaxHealth() * HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ) ) );
		}
	}
	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "npc_combine_s" )) != NULL)
	{
		if ( pEntity->GetTeamNumber() == this->GetPlayer()->GetTeamNumber() )
		{			
			pEntity->SetHealth( pEntity->GetMaxHealth() + ( pEntity->GetMaxHealth() * HL2MPRules()->GetSkillFormula( this->GetClassname(), m_iLevel ) ) );
		}
	}
}