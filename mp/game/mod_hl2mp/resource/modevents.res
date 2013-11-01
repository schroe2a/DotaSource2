//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//=============================================================================

// No spaces in event names, max length 32
// All strings are case sensitive
//
// valid data key types are:
//   string : a zero terminated string
//   bool   : unsigned int, 1 bit
//   byte   : unsigned int, 8 bit
//   short  : signed int, 16 bit
//   long   : signed int, 32 bit
//   float  : float, 32 bit
//   local  : any data, but not networked to clients
//
// following key names are reserved:
//   local      : if set to 1, event is not networked to clients
//   unreliable : networked, but unreliable
//   suppress   : never fire this event
//   time	: firing server time
//   eventid	: holds the event ID

"modevents"
{
	"player_death"				// a game event, name may be 32 charaters long
	{
		"userid"	"short"   	// user ID who died				
		"attacker"	"short"	 	// user ID who killed
		"weapon"	"string" 	// weapon name killed used 
	}
	
	"teamplay_round_start"			// round restart
	{
		"full_reset"	"bool"		// is this a full reset of the map
	}
	
	"spec_target_updated"
	{
	}
	
	"achievement_earned"
	{
		"player"	"byte"		// entindex of the player
		"achievement"	"short"		// achievement ID
	}
	
	// Issue#7: JSM - 2013-10-06 - new mod events to indicate creep spawn/death
	"creep_spawn"
	{
		"entindex"	"short"		// entindex of the newly spawned creep entity
	}
	
	"creep_death"
	{
		"entindex"	"short"		// entindex of the creep entity that just died
	}
	
	// Issue#28: JSM - 2013-10-12 - new mod events to indicate antlionguard, ammobox spawn/death
	"antlionguard_spawn"
	{
		"entindex"	"short"		// entindex of the newly spawned antlionguard entity
	}
	
	"antlionguard_death"
	{
		"entindex"	"short"		// entindex of the antlionguard entity that just died
	}
	"ammobox_spawn"
	{
		"entindex"	"short"		// entindex of the newly spawned ammobox entity
	}
	
	"ammobox_death"
	{
		"entindex"	"short"		// entindex of the ammobox entity that just died
	}

	// Issue #35: JMS - 2013-10-27 - new mod events to indicate the spawn of objective gates in Dota Source
	"objectivegate_open"
	{
		"lane"		"string"	// "LEFT", "CENTER", "RIGHT"
		"team"		"short"		// BaseEntity's team number
	}
	"objectivegate_close"
	{
		"lane"		"string"	// "LEFT", "CENTER", "RIGHT"
		"team"		"short"		// BaseEntity's team number
	}
	"objectivegate_attacked"
	{
		"lane"		"string"	// "LEFT", "CENTER", "RIGHT"
		"team"		"short"		// BaseEntity's team number
		"health"	"float"		// Amount of health left for the attached gate
	}
}
