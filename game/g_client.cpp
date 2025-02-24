// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "Ghoul2/G2.h"
#include "bg_saga.h"
#include "bg_luaevent.h"
#include "JAPP/jp_csflags.h"
#include "bg_lua.h"
#include <unordered_map>

// g_client.c -- client functions that don't happen every frame

static vector3	playerMins = { -15, -15, DEFAULT_MINS_2 };
static vector3	playerMaxs = { 15, 15, DEFAULT_MAXS_2 };

extern int g_siegeRespawnCheck;

void WP_SaberAddG2Model( gentity_t *saberent, const char *saberModel, qhandle_t saberSkin );
void WP_SaberRemoveG2Model( gentity_t *saberent );
qboolean WP_SaberStyleValidForSaber( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int saberAnimLevel );
qboolean WP_UseFirstValidSaberStyle( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int *saberAnimLevel );

/*QUAKED info_player_duel (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for duelists in duel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_duel( gentity_t *ent ) {
	int i;

	G_SpawnInt( "nobots", "0", &i );
	if ( i )
		ent->flags |= FL_NO_BOTS;

	G_SpawnInt( "nohumans", "0", &i );
	if ( i )
		ent->flags |= FL_NO_HUMANS;
}

/*QUAKED info_player_duel1 (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for lone duelists in powerduel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_duel1( gentity_t *ent ) {
	int i;

	G_SpawnInt( "nobots", "0", &i );
	if ( i )
		ent->flags |= FL_NO_BOTS;

	G_SpawnInt( "nohumans", "0", &i );
	if ( i )
		ent->flags |= FL_NO_HUMANS;
}

/*QUAKED info_player_duel2 (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for paired duelists in powerduel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_duel2( gentity_t *ent ) {
	int i;

	G_SpawnInt( "nobots", "0", &i );
	if ( i )
		ent->flags |= FL_NO_BOTS;

	G_SpawnInt( "nohumans", "0", &i );
	if ( i )
		ent->flags |= FL_NO_HUMANS;
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int i;

	G_SpawnInt( "nobots", "0", &i );
	if ( i )
		ent->flags |= FL_NO_BOTS;

	G_SpawnInt( "nohumans", "0", &i );
	if ( i )
		ent->flags |= FL_NO_HUMANS;
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
Targets will be fired when someone spawns in on them.
equivelant to info_player_deathmatch
*/
void SP_info_player_start( gentity_t *ent ) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_start_red (1 0 0) (-16 -16 -24) (16 16 32) INITIAL
For Red Team DM starts

Targets will be fired when someone spawns in on them.
equivalent to info_player_deathmatch

INITIAL - The first time a player enters the game, they will be at an 'initial' spot.

"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_start_red( gentity_t *ent ) {
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_start_blue (1 0 0) (-16 -16 -24) (16 16 32) INITIAL
For Blue Team DM starts

Targets will be fired when someone spawns in on them.
equivalent to info_player_deathmatch

INITIAL - The first time a player enters the game, they will be at an 'initial' spot.

"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_start_blue( gentity_t *ent ) {
	SP_info_player_deathmatch( ent );
}

void SiegePointUse( gentity_t *self, gentity_t *other, gentity_t *activator ) {//Toggle the point on/off
	self->genericValue1 ^= 1;
}

/*QUAKED info_player_siegeteam1 (1 0 0) (-16 -16 -24) (16 16 32)
siege start point - team1
name and behavior of team1 depends on what is defined in the
.siege file for this level

startoff - if non-0 spawn point will be disabled until used
idealclass - if specified, this spawn point will be considered
"ideal" for players of this class name. Corresponds to the name
entry in the .scl (siege class) file.
Targets will be fired when someone spawns in on them.
*/
void SP_info_player_siegeteam1( gentity_t *ent ) {
	int startOff = 0;

	if ( level.gametype != GT_SIEGE ) { //turn into a DM spawn if not in siege game mode
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );
		return;
	}

	G_SpawnInt( "startoff", "0", &startOff );
	ent->genericValue1 = startOff ? 0 : 1;

	ent->use = SiegePointUse;
}

/*QUAKED info_player_siegeteam2 (0 0 1) (-16 -16 -24) (16 16 32)
siege start point - team2
name and behavior of team2 depends on what is defined in the
.siege file for this level

startoff - if non-0 spawn point will be disabled until used
idealclass - if specified, this spawn point will be considered
"ideal" for players of this class name. Corresponds to the name
entry in the .scl (siege class) file.
Targets will be fired when someone spawns in on them.
*/
void SP_info_player_siegeteam2( gentity_t *ent ) {
	int startOff = 0;

	if ( level.gametype != GT_SIEGE ) { //turn into a DM spawn if not in siege game mode
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );
		return;
	}

	G_SpawnInt( "startoff", "0", &startOff );

	ent->genericValue1 = startOff ? 0 : 1;

	ent->use = SiegePointUse;
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32) RED BLUE
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
RED - In a Siege game, the intermission will happen here if the Red (attacking) team wins
BLUE - In a Siege game, the intermission will happen here if the Blue (defending) team wins
*/
void SP_info_player_intermission( gentity_t *ent ) {}

/*QUAKED info_player_intermission_red (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.

In a Siege game, the intermission will happen here if the Red (attacking) team wins
target - ent to look at
target2 - ents to use when this intermission point is chosen
*/
void SP_info_player_intermission_red( gentity_t *ent ) {}

/*QUAKED info_player_intermission_blue (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.

In a Siege game, the intermission will happen here if the Blue (defending) team wins
target - ent to look at
target2 - ents to use when this intermission point is chosen
*/
void SP_info_player_intermission_blue( gentity_t *ent ) {}

#define JMSABER_RESPAWN_TIME 20000 //in case it gets stuck somewhere no one can reach

void ThrowSaberToAttacker( gentity_t *self, gentity_t *attacker ) {
	gentity_t	*ent = &g_entities[self->client->ps.saberIndex];
	vector3		a;
	int			altVelocity = 0;

	if ( !ent || ent->enemy != self ) { //something has gone very wrong (this should never happen)
		//but in case it does.. find the saber manually
#ifdef _DEBUG
		Com_Printf( "Lost the saber! Attempting to use global pointer..\n" );
#endif
		ent = gJMSaberEnt;

		if ( !ent ) {
#ifdef _DEBUG
			Com_Printf( "The global pointer was NULL. This is a bad thing.\n" );
#endif
			return;
		}

#ifdef _DEBUG
		Com_Printf( "Got it (%i). Setting enemy to client %i.\n", ent->s.number, self->s.number );
#endif

		ent->enemy = self;
		self->client->ps.saberIndex = ent->s.number;
	}

	trap->SetConfigstring( CS_CLIENT_JEDIMASTER, "-1" );

	if ( attacker && attacker->client && self->client->ps.saberInFlight ) {//someone killed us and we had the saber thrown, so actually move this saber to the saber location
		//	if we killed ourselves with saber thrown, however, same suicide rules of respawning at spawn spot still apply.
		gentity_t *flyingsaber = &g_entities[self->client->ps.saberEntityNum];

		if ( flyingsaber && flyingsaber->inuse ) {
			VectorCopy( &flyingsaber->s.pos.trBase, &ent->s.pos.trBase );
			VectorCopy( &flyingsaber->s.pos.trDelta, &ent->s.pos.trDelta );
			VectorCopy( &flyingsaber->s.apos.trBase, &ent->s.apos.trBase );
			VectorCopy( &flyingsaber->s.apos.trDelta, &ent->s.apos.trDelta );
			VectorCopy( &flyingsaber->r.currentOrigin, &ent->r.currentOrigin );
			VectorCopy( &flyingsaber->r.currentAngles, &ent->r.currentAngles );
			altVelocity = 1;
		}
	}

	self->client->ps.saberInFlight = qtrue; //say he threw it anyway in order to properly remove from dead body

	WP_SaberAddG2Model( ent, self->client->saber[0].model, self->client->saber[0].skin );

	ent->s.eFlags &= ~(EF_NODRAW);
	ent->s.modelGhoul2 = 1;
	ent->s.eType = ET_MISSILE;
	ent->enemy = NULL;

	if ( !attacker || !attacker->client ) {
		VectorCopy( &ent->s.origin2, &ent->s.pos.trBase );
		VectorCopy( &ent->s.origin2, &ent->s.origin );
		VectorCopy( &ent->s.origin2, &ent->r.currentOrigin );
		ent->pos2.x = 0;
		trap->LinkEntity( (sharedEntity_t *)ent );
		return;
	}

	if ( !altVelocity ) {
		VectorCopy( &self->s.pos.trBase, &ent->s.pos.trBase );
		VectorCopy( &self->s.pos.trBase, &ent->s.origin );
		VectorCopy( &self->s.pos.trBase, &ent->r.currentOrigin );

		VectorSubtract( &attacker->client->ps.origin, &ent->s.pos.trBase, &a );

		VectorNormalize( &a );

		ent->s.pos.trDelta.x = a.x * 256;
		ent->s.pos.trDelta.y = a.y * 256;
		ent->s.pos.trDelta.z = 256;
	}

	trap->LinkEntity( (sharedEntity_t *)ent );
}

void JMSaberThink( gentity_t *ent ) {
	gJMSaberEnt = ent;

	if ( ent->enemy ) {
		if ( !ent->enemy->client || !ent->enemy->inuse ) { //disconnected?
			VectorCopy( &ent->enemy->s.pos.trBase, &ent->s.pos.trBase );
			VectorCopy( &ent->enemy->s.pos.trBase, &ent->s.origin );
			VectorCopy( &ent->enemy->s.pos.trBase, &ent->r.currentOrigin );

			ent->s.modelindex = G_ModelIndex( "models/weapons2/saber/saber_w.glm" );
			ent->s.eFlags &= ~(EF_NODRAW);
			ent->s.modelGhoul2 = 1;
			ent->s.eType = ET_MISSILE;
			ent->enemy = NULL;

			ent->pos2.x = 1;
			ent->pos2.y = 0; //respawn next think
			trap->LinkEntity( (sharedEntity_t *)ent );
		}
		else
			ent->pos2.y = level.time + JMSABER_RESPAWN_TIME;
	}
	else if ( ent->pos2.x && ent->pos2.y < level.time ) {
		VectorCopy( &ent->s.origin2, &ent->s.pos.trBase );
		VectorCopy( &ent->s.origin2, &ent->s.origin );
		VectorCopy( &ent->s.origin2, &ent->r.currentOrigin );
		ent->pos2.x = 0;
		trap->LinkEntity( (sharedEntity_t *)ent );
	}

	ent->nextthink = level.time + 50;
	G_RunObject( ent );
}

void JMSaberTouch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	int i;

	if ( !other || !other->client || other->health < 1 )
		return;

	if ( self->enemy )
		return;

	if ( !self->s.modelindex )
		return;

	if ( other->client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER) )
		return;

	if ( other->client->ps.isJediMaster )
		return;

	self->enemy = other;
	other->client->ps.stats[STAT_WEAPONS] = (1 << WP_SABER);
	other->client->ps.weapon = WP_SABER;
	other->s.weapon = WP_SABER;
	other->client->ps.zoomMode = 0;
	G_AddEvent( other, EV_BECOME_JEDIMASTER, 0 );

	// Track the jedi master
	trap->SetConfigstring( CS_CLIENT_JEDIMASTER, va( "%i", other->s.number ) );

	if ( g_spawnInvulnerability.integer ) {
		other->client->ps.eFlags |= EF_INVULNERABLE;
		other->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
	}

	trap->SendServerCommand( -1, va( "cp \"%s %s\n\"", other->client->pers.netname, G_GetStringEdString( "MP_SVGAME", "BECOMEJM" ) ) );

	other->client->ps.isJediMaster = qtrue;
	other->client->ps.saberIndex = self->s.number;

	//full health when you become the Jedi Master
	other->health = other->client->ps.stats[STAT_HEALTH] = 200;
	other->client->ps.fd.forcePower = 100;

	for ( i = 0; i < NUM_FORCE_POWERS; i++ ) {
		other->client->ps.fd.forcePowersKnown |= (1 << i);
		other->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;
	}

	self->pos2.x = 1;
	self->pos2.y = level.time + JMSABER_RESPAWN_TIME;

	self->s.modelindex = 0;
	self->s.eFlags |= EF_NODRAW;
	self->s.modelGhoul2 = 0;
	self->s.eType = ET_GENERAL;

	/*
	te = G_TempEntity( vec3_origin, EV_DESTROY_GHOUL2_INSTANCE );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = self->s.number;
	*/
	G_KillG2Queue( self->s.number );

	return;
}

gentity_t *gJMSaberEnt = NULL;

/*QUAKED info_jedimaster_start (1 0 0) (-16 -16 -24) (16 16 32)
"jedi master" saber spawn point
*/
void SP_info_jedimaster_start( gentity_t *ent ) {
	if ( level.gametype != GT_JEDIMASTER ) {
		gJMSaberEnt = NULL;
		G_FreeEntity( ent );
		return;
	}

	ent->enemy = NULL;

	ent->flags = FL_BOUNCE_HALF;

	ent->s.modelindex = G_ModelIndex( "models/weapons2/saber/saber_w.glm" );
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 20;
	//	ent->s.eType		= ET_GENERAL;
	ent->s.eType = ET_MISSILE;
	ent->s.weapon = WP_SABER;
	ent->s.pos.trType = TR_GRAVITY;
	ent->s.pos.trTime = level.time;
	VectorSet( &ent->r.maxs, 3, 3, 3 );
	VectorSet( &ent->r.mins, -3, -3, -3 );
	ent->r.contents = CONTENTS_TRIGGER;
	ent->clipmask = MASK_SOLID;
	ent->isSaberEntity = qtrue;
	ent->bounceCount = -5;
	ent->physicsObject = qtrue;
	VectorCopy( &ent->s.pos.trBase, &ent->s.origin2 ); //remember the spawn spot
	ent->touch = JMSaberTouch;
	ent->think = JMSaberThink;
	ent->nextthink = level.time + 500;
	trap->LinkEntity( (sharedEntity_t *)ent );
}

qboolean SpotWouldTelefrag( gentity_t *spot ) {
	int			i;
	int			num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vector3		mins, maxs;

	VectorAdd( &spot->s.origin, &playerMins, &mins );
	VectorAdd( &spot->s.origin, &playerMaxs, &maxs );

	num = trap->EntitiesInBox( &mins, &maxs, touch, MAX_GENTITIES );

	for ( i = 0; i < num; i++ ) {
		hit = &g_entities[touch[i]];

		if ( hit->client )
			return qtrue;
	}

	return qfalse;
}

qboolean SpotWouldTelefrag2( gentity_t *mover, vector3 *dest ) {
	int			i;
	int			num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vector3		mins, maxs;

	VectorAdd( dest, &mover->r.mins, &mins );
	VectorAdd( dest, &mover->r.maxs, &maxs );

	num = trap->EntitiesInBox( &mins, &maxs, touch, MAX_GENTITIES );

	for ( i = 0; i < num; i++ ) {
		hit = &g_entities[touch[i]];

		if ( hit == mover )
			continue;
		if ( hit->r.contents & mover->r.contents )
			return qtrue;
	}

	return qfalse;
}

qboolean SpotWouldTelefrag3( vector3 *spot ) {
	int i, num, touch[MAX_GENTITIES];
	gentity_t *hit;
	vector3 mins, maxs;

	VectorAdd( spot, &playerMins, &mins );
	VectorAdd( spot, &playerMaxs, &maxs );

	num = trap->EntitiesInBox( &mins, &maxs, touch, MAX_GENTITIES );

	for ( i = 0; i < num; i++ ) {
		hit = &g_entities[touch[i]];

		if ( hit->client )
			return qtrue;
	}

	return qfalse;
}

#define	MAX_SPAWN_POINTS	128

// Find the spot that we DON'T want to use
gentity_t *SelectNearestDeathmatchSpawnPoint( vector3 *from ) {
	gentity_t	*spot = NULL;
	vector3		delta;
	float		dist;
	float		nearestDist = 999999.0f;
	gentity_t	*nearestSpot = NULL;

	while ( (spot = G_Find( spot, FOFS( classname ), "info_player_deathmatch" )) != NULL ) {

		VectorSubtract( &spot->s.origin, from, &delta );
		dist = VectorLength( &delta );

		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}

// go to a random point that doesn't telefrag
gentity_t *SelectRandomDeathmatchSpawnPoint( void ) {
	gentity_t	*spot = NULL;
	int			count = 0;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	while ( (spot = G_Find( spot, FOFS( classname ), "info_player_deathmatch" )) != NULL ) {
		if ( SpotWouldTelefrag( spot ) )
			continue;
		spots[count] = spot;
		count++;
	}

	// no spots that won't telefrag, find first spawnpoint
	if ( !count )
		return G_Find( NULL, FOFS( classname ), "info_player_deathmatch" );

	selection = rand() % count;
	return spots[selection];
}

// Chooses a player start, deathmatch start, etc
gentity_t *SelectRandomFurthestSpawnPoint( vector3 *avoidPoint, vector3 *origin, vector3 *angles, team_t team ) {
	gentity_t	*spot = NULL;
	vector3		delta;
	float		dist;
	float		list_dist[64];
	gentity_t	*list_spot[64];
	int			numSpots = 0;
	int			rnd;
	int			i;
	int			j;

	//in Team DM, look for a team start spot first, if any
	if ( level.gametype == GT_TEAM && team != TEAM_FREE && team != TEAM_SPECTATOR ) {//team-game, either red or blue
		const char *classname = (team == TEAM_RED) ? "info_player_start_red" : "info_player_start_blue";

		while ( (spot = G_Find( spot, FOFS( classname ), classname )) != NULL ) {
			if ( SpotWouldTelefrag( spot ) )
				continue;
			VectorSubtract( &spot->s.origin, avoidPoint, &delta );
			dist = VectorLength( &delta );
			for ( i = 0; i < numSpots; i++ ) {
				if ( dist > list_dist[i] ) {
					if ( numSpots >= 64 )
						numSpots = 64 - 1;

					for ( j = numSpots; j > i; j-- ) {
						list_dist[j] = list_dist[j - 1];
						list_spot[j] = list_spot[j - 1];
					}
					list_dist[i] = dist;
					list_spot[i] = spot;
					numSpots++;
					if ( numSpots > 64 )
						numSpots = 64;
					break;
				}
			}
			if ( i >= numSpots && numSpots < 64 ) {
				list_dist[numSpots] = dist;
				list_spot[numSpots] = spot;
				numSpots++;
			}
		}
	}

	if ( !numSpots ) {//couldn't find any of the above
		while ( (spot = G_Find( spot, FOFS( classname ), "info_player_deathmatch" )) != NULL ) {
			if ( SpotWouldTelefrag( spot ) )
				continue;

			VectorSubtract( &spot->s.origin, avoidPoint, &delta );
			dist = VectorLength( &delta );

			for ( i = 0; i<numSpots; i++ ) {
				if ( dist > list_dist[i] ) {
					if ( numSpots >= 64 )
						numSpots = 64 - 1;

					for ( j = numSpots; j > i; j-- ) {
						list_dist[j] = list_dist[j - 1];
						list_spot[j] = list_spot[j - 1];
					}

					list_dist[i] = dist;
					list_spot[i] = spot;
					numSpots++;

					if ( numSpots > 64 )
						numSpots = 64;
					break;
				}
			}
			if ( i >= numSpots && numSpots < 64 ) {
				list_dist[numSpots] = dist;
				list_spot[numSpots] = spot;
				numSpots++;
			}
		}
		if ( !numSpots ) {
			spot = G_Find( NULL, FOFS( classname ), "info_player_deathmatch" );
			if ( !spot ) {
				gentity_t *newSpot = G_Spawn();

				Com_Printf( "Couldn't find a spawn point, attempting to spawn at map center" );
				newSpot->classname = "info_player_deathmatch";
				VectorSet( &newSpot->s.origin, 0.0f, 0.0f, 64.0f );
				spot = newSpot;
				return spot;
			}
			VectorCopy( &spot->s.origin, origin );
			origin->z += 9;
			VectorCopy( &spot->s.angles, angles );
			return spot;
		}
	}

	// select a random spot from the spawn points furthest away
	//rnd = random() * (numSpots / 2);
	//rnd = Q_irand( 0, QRAND_MAX - 1 ) / numSpots
	rnd = Q_irand( 0, numSpots );

	VectorCopy( &list_spot[rnd]->s.origin, origin );
	origin->z += 9;
	VectorCopy( &list_spot[rnd]->s.angles, angles );

	return list_spot[rnd];
}

gentity_t *SelectDuelSpawnPoint( int team, vector3 *avoidPoint, vector3 *origin, vector3 *angles ) {
	gentity_t	*spot, *list_spot[64];
	vector3		delta;
	float		dist, list_dist[64];
	int			numSpots, rnd, i, j;
	const char *spotName;

	if ( team == DUELTEAM_LONE )	spotName = "info_player_duel1";
	else if ( team == DUELTEAM_DOUBLE )	spotName = "info_player_duel2";
	else if ( team == DUELTEAM_SINGLE )	spotName = "info_player_duel";
	else								spotName = "info_player_deathmatch";

tryAgain:
	numSpots = 0;
	spot = NULL;

	while ( (spot = G_Find( spot, FOFS( classname ), spotName )) != NULL ) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		VectorSubtract( &spot->s.origin, avoidPoint, &delta );
		dist = VectorLength( &delta );
		for ( i = 0; i < numSpots; i++ ) {
			if ( dist > list_dist[i] ) {
				if ( numSpots >= 64 )
					numSpots = 64 - 1;
				for ( j = numSpots; j > i; j-- ) {
					list_dist[j] = list_dist[j - 1];
					list_spot[j] = list_spot[j - 1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				if ( numSpots > 64 )
					numSpots = 64;
				break;
			}
		}
		if ( i >= numSpots && numSpots < 64 ) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if ( !numSpots ) {
		if ( Q_stricmp( spotName, "info_player_deathmatch" ) ) { //try the loop again with info_player_deathmatch as the target if we couldn't find a duel spot
			spotName = "info_player_deathmatch";
			goto tryAgain;
		}

		//If we got here we found no free duel or DM spots, just try the first DM spot
		spot = G_Find( NULL, FOFS( classname ), "info_player_deathmatch" );
		if ( !spot )
			trap->Error( ERR_DROP, "Couldn't find a spawn point" );
		VectorCopy( &spot->s.origin, origin );
		origin->z += 9;
		VectorCopy( &spot->s.angles, angles );
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy( &list_spot[rnd]->s.origin, origin );
	origin->z += 9;
	VectorCopy( &list_spot[rnd]->s.angles, angles );

	return list_spot[rnd];
}

// Chooses a player start, deathmatch start, etc
gentity_t *SelectSpawnPoint( vector3 *avoidPoint, vector3 *origin, vector3 *angles, team_t team ) {
	return SelectRandomFurthestSpawnPoint( avoidPoint, origin, angles, team );

	/*
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
	// roll again if it would be real close to point of death
	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
	// last try
	spot = SelectRandomDeathmatchSpawnPoint ( );
	}
	}

	// find a single player start spot
	if (!spot) {
	trap->Error( ERR_DROP, "Couldn't find a spawn point" );
	}

	VectorCopy (spot->s.origin, origin);
	origin.z += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
	*/
}

// Try to find a spawn point marked 'initial', otherwise use normal spawn selection.
gentity_t *SelectInitialSpawnPoint( vector3 *origin, vector3 *angles, team_t team ) {
	gentity_t	*spot;

	spot = NULL;
	while ( (spot = G_Find( spot, FOFS( classname ), "info_player_deathmatch" )) != NULL ) {
		if ( spot->spawnflags & 1 ) {
			break;
		}
	}

	if ( !spot || SpotWouldTelefrag( spot ) ) {
		return SelectSpawnPoint( &vec3_origin, origin, angles, team );
	}

	VectorCopy( &spot->s.origin, origin );
	origin->z += 9;
	VectorCopy( &spot->s.angles, angles );

	return spot;
}

gentity_t *SelectSpectatorSpawnPoint( vector3 *origin, vector3 *angles ) {
	FindIntermissionPoint();

	VectorCopy( &level.intermission_origin, origin );
	VectorCopy( &level.intermission_angle, angles );

	return NULL;
}

#define BODY_SINK_TIME		japp_corpseRemovalTime.value * 1000//30000//45000

void InitBodyQue( void ) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for ( i = 0; i < BODY_QUEUE_SIZE; i++ ) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

// After sitting around for five seconds, fall into the ground and dissapear
void BodySink( gentity_t *ent ) {
	if ( level.time - ent->timestamp > BODY_SINK_TIME + 2500 ) {
		// the body ques are never actually freed, they are just unlinked
		trap->UnlinkEntity( (sharedEntity_t *)ent );
		ent->physicsObject = qfalse;
		return;
	}
	//	ent->nextthink = level.time + 100;
	//	ent->s.pos.trBase[2] -= 1;

	G_AddEvent( ent, EV_BODYFADE, 0 );
	ent->nextthink = level.time + 18000;
	ent->takedamage = qfalse;
}

// A player is respawning, so make an entity that looks just like the existing corpse to leave behind.
static qboolean CopyToBodyQue( gentity_t *ent ) {
	gentity_t	*body;
	uint32_t	contents;
	int			islight = 0;

	if ( level.intermissiontime ) {
		return qfalse;
	}

	trap->UnlinkEntity( (sharedEntity_t *)ent );

	// if client is in a nodrop area, don't leave the body
	contents = trap->PointContents( &ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return qfalse;
	}

	if ( ent->client && (ent->client->ps.eFlags & EF_DISINTEGRATION) ) { //for now, just don't spawn a body if you got disint'd
		return qfalse;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[level.bodyQueIndex];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	trap->UnlinkEntity( (sharedEntity_t *)body );
	body->s = ent->s;

	//avoid oddly angled corpses floating around
	body->s.angles.pitch = body->s.angles.roll = body->s.apos.trBase.pitch = body->s.apos.trBase.roll = 0;

	body->s.g2radius = 100;

	body->s.eType = ET_BODY;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc

	if ( ent->client && (ent->client->ps.eFlags & EF_DISINTEGRATION) ) {
		body->s.eFlags |= EF_DISINTEGRATION;
	}

	VectorCopy( &ent->client->ps.lastHitLoc, &body->s.origin2 );

	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.loopIsSoundset = qfalse;
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( &ent->client->ps.velocity, &body->s.pos.trDelta );
	}
	else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	body->s.weapon = ent->s.bolt2;

	if ( body->s.weapon == WP_SABER && ent->client->ps.saberInFlight ) {
		body->s.weapon = WP_BLASTER; //lie to keep from putting a saber on the corpse, because it was thrown at death
	}

	//G_AddEvent(body, EV_BODY_QUEUE_COPY, ent->s.clientNum);
	//Now doing this through a modified version of the rcg reliable command.
	if ( ent->client && ent->client->ps.fd.forceSide == FORCESIDE_LIGHT ) {
		islight = 1;
	}
	trap->SendServerCommand( -1, va( "ircg %i %i %i %i", ent->s.number, body->s.number, body->s.weapon, islight ) );

	body->r.svFlags = ent->r.svFlags | SVF_BROADCAST;
	VectorCopy( &ent->r.mins, &body->r.mins );
	VectorCopy( &ent->r.maxs, &body->r.maxs );
	VectorCopy( &ent->r.absmin, &body->r.absmin );
	VectorCopy( &ent->r.absmax, &body->r.absmax );

	body->s.torsoAnim = body->s.legsAnim = ent->client->ps.legsAnim;

	body->s.customRGBA[0] = ent->client->ps.customRGBA[0];
	body->s.customRGBA[1] = ent->client->ps.customRGBA[1];
	body->s.customRGBA[2] = ent->client->ps.customRGBA[2];
	body->s.customRGBA[3] = ent->client->ps.customRGBA[3];

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + BODY_SINK_TIME;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	}
	else {
		body->takedamage = qtrue;
	}

	VectorCopy( &body->s.pos.trBase, &body->r.currentOrigin );
	trap->LinkEntity( (sharedEntity_t *)body );

	return qtrue;
}

void SetClientViewAngle( gentity_t *ent, vector3 *angle ) {
	int			i;

	// set the delta angle
	for ( i = 0; i < 3; i++ ) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT( angle->raw[i] );
		ent->client->ps.delta_angles.raw[i] = cmdAngle - ent->client->pers.cmd.angles.raw[i];
	}
	VectorCopy( angle, &ent->s.angles );
	VectorCopy( &ent->s.angles, &ent->client->ps.viewangles );
}

void MaintainBodyQueue( gentity_t *ent ) { //do whatever should be done taking ragdoll and dismemberment states into account.
	qboolean doRCG = qfalse;

	assert( ent && ent->client );
	if ( ent->client->tempSpectate >= level.time ||
		(ent->client->ps.eFlags2 & EF2_SHIP_DEATH) ) {
		ent->client->noCorpse = qtrue;
	}

	if ( !ent->client->noCorpse && !ent->client->ps.fallingToDeath ) {
		if ( !CopyToBodyQue( ent ) ) {
			doRCG = qtrue;
		}
	}
	else {
		ent->client->noCorpse = qfalse; //clear it for next time
		ent->client->ps.fallingToDeath = qfalse;
		doRCG = qtrue;
	}

	if ( doRCG ) { //bodyque func didn't manage to call ircg so call this to assure our limbs and ragdoll states are proper on the client.
		trap->SendServerCommand( -1, va( "rcg %i", ent->s.clientNum ) );
	}
}

void SiegeRespawn( gentity_t *ent );

void respawn( gentity_t *ent ) {
	MaintainBodyQueue( ent );

	if ( ent->client->hook ) {
		// make sure hook is removed, SHOULD BE DONE BEFORE THIS
		assert( !"respawn(): grapple hook was not removed" );
		Weapon_HookFree( ent->client->hook );
	}

	if ( gEscaping || level.gametype == GT_POWERDUEL ) {
		ent->client->sess.sessionTeam = TEAM_SPECTATOR;
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->sess.spectatorClient = 0;

		ent->client->pers.teamState.state = TEAM_BEGIN;
		ent->client->sess.spectatorTime = level.time;
		ClientSpawn( ent );
		ent->client->iAmALoser = qtrue;
		return;
	}

	trap->UnlinkEntity( (sharedEntity_t *)ent );

	if ( level.gametype == GT_SIEGE ) {
		if ( g_siegeRespawn.integer ) {
			if ( ent->client->tempSpectate < level.time ) {
				int minDel = g_siegeRespawn.integer * 2000;
				if ( minDel < 20000 ) {
					minDel = 20000;
				}
				ent->client->tempSpectate = level.time + minDel;
				ent->health = ent->client->ps.stats[STAT_HEALTH] = 1;
				ent->client->ps.weapon = WP_NONE;
				ent->client->ps.stats[STAT_WEAPONS] = 0;
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
				ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
				ent->takedamage = qfalse;
				trap->LinkEntity( (sharedEntity_t *)ent );

				// Respawn time.
				if ( ent->s.number < MAX_CLIENTS ) {
					gentity_t *te = G_TempEntity( &ent->client->ps.origin, EV_SIEGESPEC );
					te->s.time = g_siegeRespawnCheck;
					te->s.owner = ent->s.number;
				}

				return;
			}
		}
		SiegeRespawn( ent );
	}
	else {
		gentity_t	*tent;

		ClientSpawn( ent );

		// add a teleportation effect
		tent = G_TempEntity( &ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = ent->s.clientNum;
	}
}

// Returns number of players on a team
int TeamCount( int ignoreClientNum, team_t team ) {
	int		i;
	int		count = 0;

	for ( i = 0; i < level.maxclients; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			count++;
		}
		else if ( level.gametype == GT_SIEGE &&
			level.clients[i].sess.siegeDesiredTeam == team ) {
			count++;
		}
	}

	return count;
}

team_t PickTeam( int ignoreClientNum ) {
	int		counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount( ignoreClientNum, TEAM_BLUE );
	counts[TEAM_RED] = TeamCount( ignoreClientNum, TEAM_RED );

	if ( counts[TEAM_BLUE] > counts[TEAM_RED] ) {
		return TEAM_RED;
	}
	if ( counts[TEAM_RED] > counts[TEAM_BLUE] ) {
		return TEAM_BLUE;
	}
	// equal team count, so join the team with the lowest score
	if ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] ) {
		return TEAM_RED;
	}
	return TEAM_BLUE;
}

static qboolean IsSuffixed( const char *cleanName, int32_t *suffixNum ) {
	char tmp[MAX_NETNAME], *s;
	const char *bracketL, *bracketR;
	size_t len;

	len = strlen( cleanName );

	// no left bracket at all, can't be suffixed
	if ( !(bracketL = strrchr( cleanName, '(' )) ) {
		return qfalse;
	}

	//	0 1 2 3 4 5 6 7 8 9
	//	h,e,l,l,o, ,(,0,2,)
	//	            ^
	//	strlen = 10
	// the bracket is coming too early, it can't be a clientNum suffix
	if ( cleanName - bracketL < (ptrdiff_t)(len - 3) ) {
#ifdef _DEBUG
		trap->Print( "DuplicateNames: bracket came too early, not removing suffix. [cleanName - bracketL](%d) < "
			"[len - 3]()\n", cleanName - bracketL, len - 3 );
#endif
		return qfalse;
	}

	// no right bracket, can't be a clientNum suffix
	if ( !(bracketR = strchr( bracketL, ')' )) ) {
		return qfalse;
	}

	// found the left bracket, let's get the number between here and the next bracket
	for ( s = (char *)bracketL; s < bracketR; s++ ) {
		// if it's not a number, discard it - the name is not suffixed
		if ( !isdigit( *s ) ) {
			return qfalse;
		}

		tmp[s - bracketL] = *s;
	}

	if ( suffixNum ) {
		*suffixNum = atoi( tmp );
	}

	return qtrue;
}

// add the "(02)" suffix, truncating if needed
static void AddSuffix( char *name, size_t nameLen, int clientNum ) {
	size_t len = strlen( name );
	const char *suffix = va( " ^7(^5%02i^7)", clientNum );
	const size_t suffixLen = strlen( suffix );
	if ( len + suffixLen < nameLen ) {
		Q_strcat( name, nameLen, suffix );
	}
	else {
		// not enough space, we'll have to truncate
		char *s = &name[nameLen - suffixLen - 1];
		Q_strncpyz( s, suffix, suffixLen+1 );
	}
}

// check if the specified client is trying to use a name that is already in use and add a clientNum suffix if so
qboolean CheckDuplicateName( int clientNum ) {
	char *name = g_entities[clientNum].client->pers.netname;
	const char *cleanName = g_entities[clientNum].client->pers.netnameClean;
	gentity_t *ent;
	int i;
	int32_t suffixNum;

	// remove any potential suffix if it's not ours
	while ( IsSuffixed( cleanName, &suffixNum ) ) {
		char *s = strrchr( name, '(' );
		// if the last suffix on our name is ours, everything is in order
		if ( clientNum == suffixNum ) {
			return qfalse;
		}

		// remove this suffix and continue
		trap->Print( "Removing invalid name suffix on \"%s\"\n", G_PrintClient( clientNum ) );
		if ( s-1 > name ) {
			s--;
		}
		*s = '\0';
#ifdef _DEBUG
		trap->Print( "New name is \"%s\"\n", s );
#endif
	}

	// empty name? fall back to "Padawan"
	if ( strlen( name ) == 0u ) {
		trap->Print( "Falling back to default name for \"%s\"\n", G_PrintClient( clientNum ) );
		Q_strncpyz( name, DEFAULT_NAME, MAX_NETNAME );
	}

	// now check if someone else is using this name
	for ( i = 0, ent = g_entities; i<level.maxclients; i++, ent++ ) {
		if ( i == clientNum || !ent->inuse || ent->client->pers.connected == CON_DISCONNECTED ) {
			continue;
		}

		//TODO: perhaps do an advanced check replacing chars such as 'O' for '0'
		//	we are, after all, checking for impersonation
		if ( !Q_stricmp( ent->client->pers.netnameClean, cleanName ) ) {
			// we attempted to use this person's name, so add a suffix to ours
			trap->Print( "Adding suffix on name for \"%s\" because \"%s\" == \"%s\"\n", G_PrintClient( clientNum ),
				ent->client->pers.netnameClean, cleanName );
			AddSuffix( name, MAX_NETNAME, clientNum );
			return qtrue;
		}
	}

	return qfalse;
}

void ClientCleanName( const char *in, char *out, int outSize ) {
	int outpos = 0, colorlessLen = 0, spaces = 0, ats = 0;
	size_t len;

	// discard leading spaces
	for ( ; *in == ' '; in++ );

	for ( ; *in && outpos < outSize - 1; in++ ) {
		out[outpos] = *in;

		if ( *in == ' ' ) {// don't allow too many consecutive spaces
			if ( spaces > 2 ) {
				continue;
			}

			spaces++;
		}
		else if ( *in == '@' ) {// don't allow too many consecutive at signs
			if ( ++ats > 2 ) {
				outpos -= 2;
				ats = 0;
				continue;
			}
		}
		else if ( (byte)*in < 0x20
			|| (byte)*in == 0x81 || (byte)*in == 0x8D || (byte)*in == 0x8F || (byte)*in == 0x90 || (byte)*in == 0x9D
			|| (byte)*in == 0xA0 || (byte)*in == 0xAD )
		{
			continue;
		}
		else if ( outpos > 0 && out[outpos - 1] == Q_COLOR_ESCAPE ) {
			if ( Q_IsColorString( &out[outpos - 1] ) ) {
				colorlessLen--;
			}
			else {
				spaces = ats = 0;
				colorlessLen++;
			}
		}
		else {
			spaces = ats = 0;
			colorlessLen++;
		}

		outpos++;
	}

	out[outpos] = '\0';

	// strip trailing space
	len = strlen( out );
	while ( out[len-1] == ' ' ) {
		out[--len] = '\0';
	}

	// don't allow empty names
	if ( *out == '\0' || colorlessLen == 0 ) {
		Q_strncpyz( out, DEFAULT_NAME, outSize );
	}
}

#ifdef _DEBUG
void G_DebugWrite( const char *path, const char *text ) {
	fileHandle_t f;

	trap->FS_Open( path, &f, FS_APPEND );
	trap->FS_Write( text, strlen( text ), f );
	trap->FS_Close( f );
}
#endif

qboolean G_SaberModelSetup( gentity_t *ent ) {
	int i = 0;
	qboolean fallbackForSaber = qtrue;

	while ( i < MAX_SABERS ) {
		if ( ent->client->saber[i].model[0] ) {
			//first kill it off if we've already got it
			if ( ent->client->weaponGhoul2[i] ) {
				trap->G2API_CleanGhoul2Models( &(ent->client->weaponGhoul2[i]) );
			}
			trap->G2API_InitGhoul2Model( &ent->client->weaponGhoul2[i], ent->client->saber[i].model, 0, 0, -20, 0, 0 );

			if ( ent->client->weaponGhoul2[i] ) {
				int j = 0;
				const char *tagName;
				int tagBolt;

				if ( ent->client->saber[i].skin ) {
					trap->G2API_SetSkin( ent->client->weaponGhoul2[i], 0, ent->client->saber[i].skin, ent->client->saber[i].skin );
				}

				if ( ent->client->saber[i].saberFlags & SFL_BOLT_TO_WRIST ) {
					trap->G2API_SetBoltInfo( ent->client->weaponGhoul2[i], 0, 3 + i );
				}
				else { // bolt to right hand for 0, or left hand for 1
					trap->G2API_SetBoltInfo( ent->client->weaponGhoul2[i], 0, i );
				}

				//Add all the bolt points
				while ( j < ent->client->saber[i].numBlades ) {
					tagName = va( "*blade%i", j + 1 );
					tagBolt = trap->G2API_AddBolt( ent->client->weaponGhoul2[i], 0, tagName );

					if ( tagBolt == -1 ) {
						if ( j == 0 ) { //guess this is an 0ldsk3wl saber
							trap->G2API_AddBolt( ent->client->weaponGhoul2[i], 0, "*flash" );
							fallbackForSaber = qfalse;
							break;
						}

						if ( tagBolt == -1 ) {
							assert( 0 );
							break;

						}
					}
					j++;

					fallbackForSaber = qfalse; //got at least one custom saber so don't need default
				}

				//Copy it into the main instance
				trap->G2API_CopySpecificGhoul2Model( ent->client->weaponGhoul2[i], 0, ent->ghoul2, i + 1 );
			}
		}
		else {
			break;
		}

		i++;
	}

	return fallbackForSaber;
}

void *g2SaberInstance = NULL;

qboolean BG_IsValidCharacterModel( const char *modelName, const char *skinName );
qboolean BG_ValidateSkinForTeam( const char *modelName, char *skinName, int team, vector3 *colors );

// There are two ghoul2 model instances per player (actually three).
// One is on the clientinfo (the base for the client side player, and copied for player spawns and for corpses).
// One is attached to the centity itself, which is the model actually animated and rendered by the system.
// The final is the game ghoul2 model. This is animated by pmove on the server, and is used for determining where the
//	lightsaber should be, and for per-poly collision tests.
void SetupGameGhoul2Model( gentity_t *ent, char *modelname, char *skinName ) {
	int handle;
	char afilename[MAX_QPATH], GLAName[MAX_QPATH];
	vector3	tempVec = { 0, 0, 0 };

	// First things first.  If this is a ghoul2 model, then let's make sure we demolish this first.
	if ( ent->ghoul2 && trap->G2API_HaveWeGhoul2Models( ent->ghoul2 ) ) {
		trap->G2API_CleanGhoul2Models( &(ent->ghoul2) );
	}

	//rww - just load the "standard" model for the server"
	if ( !precachedKyle ) {
		int defSkin;

		Com_sprintf( afilename, sizeof(afilename), "models/players/kyle/model.glm" );
		handle = trap->G2API_InitGhoul2Model( &precachedKyle, afilename, 0, 0, -20, 0, 0 );

		if ( handle < 0 ) {
			return;
		}

		defSkin = trap->R_RegisterSkin( "models/players/kyle/model_default.skin" );
		trap->G2API_SetSkin( precachedKyle, 0, defSkin, defSkin );
	}

	if ( precachedKyle && trap->G2API_HaveWeGhoul2Models( precachedKyle ) ) {
		//rww - allow option for perplayer models on server for collision and bolt stuff.
		if ( d_perPlayerGhoul2.integer || ent->s.number >= MAX_CLIENTS ) {
			char modelFullPath[MAX_QPATH], truncModelName[MAX_QPATH], skin[MAX_QPATH], vehicleName[MAX_QPATH];
			int skinHandle = 0, i = 0;
			char *p;

			// If this is a vehicle, get it's model name.
			if ( ent->client->NPC_class == CLASS_VEHICLE ) {
				const char *vehModel = BG_GetVehicleModelName( modelname );
				Q_strncpyz( vehicleName, modelname, sizeof(vehicleName) );
				Q_strncpyz( modelname, vehModel, strlen( modelname ) ); // should only remove the $
				Q_strncpyz( truncModelName, vehModel, sizeof(truncModelName) );
				skin[0] = '\0';
				if ( ent->m_pVehicle && ent->m_pVehicle->m_pVehicleInfo && ent->m_pVehicle->m_pVehicleInfo->skin
					&& ent->m_pVehicle->m_pVehicleInfo->skin[0] ) {
					skinHandle = trap->R_RegisterSkin( va( "models/players/%s/model_%s.skin", vehModel,
						ent->m_pVehicle->m_pVehicleInfo->skin ) );
				}
				else
					skinHandle = trap->R_RegisterSkin( va( "models/players/%s/model_default.skin", vehModel ) );
			}
			else {
				if ( skinName && skinName[0] ) {
					Q_strncpyz( skin, skinName, sizeof(skin) );
					Q_strncpyz( truncModelName, modelname, sizeof(truncModelName) );
				}
				else {
					Q_strncpyz( skin, "default", sizeof(skin) );
					Q_strncpyz( truncModelName, modelname, sizeof(truncModelName) );
					p = Q_strrchr( truncModelName, '/' );

					if ( p ) {
						*p++ = '\0';

						while ( p && *p )
							skin[i++] = *p++;

						skin[i] = '\0';
					}

					if ( !BG_IsValidCharacterModel( truncModelName, skin ) ) {
						Q_strncpyz( truncModelName, "kyle", sizeof(truncModelName) );
						Q_strncpyz( skin, "default", sizeof(skin) );
					}

					if ( level.gametype >= GT_TEAM && level.gametype != GT_SIEGE && !g_jediVmerc.integer ) {
						//Also adjust customRGBA for team colors.
						vector3 colorOverride;

						colorOverride.r = colorOverride.g = colorOverride.b = 0.0f;

						BG_ValidateSkinForTeam( truncModelName, skin, ent->client->sess.sessionTeam, &colorOverride );
						if ( colorOverride.r != 0.0f || colorOverride.g != 0.0f || colorOverride.b != 0.0f ) {
							ent->client->ps.customRGBA[0] = colorOverride.r*255.0f;
							ent->client->ps.customRGBA[1] = colorOverride.g*255.0f;
							ent->client->ps.customRGBA[2] = colorOverride.b*255.0f;
						}

						//BG_ValidateSkinForTeam( truncModelName, skin, ent->client->sess.sessionTeam, NULL );
					}
					else if ( level.gametype == GT_SIEGE ) { //force skin for class if appropriate
						if ( ent->client->siegeClass != -1 ) {
							siegeClass_t *scl = &bgSiegeClasses[ent->client->siegeClass];
							if ( scl->forcedSkin[0] ) {
								strcpy( skin, scl->forcedSkin );
							}
						}
					}
				}
			}

			if ( skin[0] ) {
				const char *useSkinName;

				if ( strchr( skin, '|' ) )
					useSkinName = va( "models/players/%s/|%s", truncModelName, skin );
				else
					useSkinName = va( "models/players/%s/model_%s.skin", truncModelName, skin );

				skinHandle = trap->R_RegisterSkin( useSkinName );
			}

			Q_strncpyz( modelFullPath, va( "models/players/%s/model.glm", truncModelName ), sizeof(modelFullPath) );
			handle = trap->G2API_InitGhoul2Model( &ent->ghoul2, modelFullPath, 0, skinHandle, -20, 0, 0 );

			if ( handle < 0 ) { //Huh. Guess we don't have this model. Use the default.

				if ( ent->ghoul2 && trap->G2API_HaveWeGhoul2Models( ent->ghoul2 ) ) {
					trap->G2API_CleanGhoul2Models( &(ent->ghoul2) );
				}
				ent->ghoul2 = NULL;
				trap->G2API_DuplicateGhoul2Instance( precachedKyle, &ent->ghoul2 );
			}
			else {
				trap->G2API_SetSkin( ent->ghoul2, 0, skinHandle, skinHandle );

				GLAName[0] = 0;
				trap->G2API_GetGLAName( ent->ghoul2, 0, GLAName );

				if ( !GLAName[0] || (!strstr( GLAName, "players/_humanoid/" ) && ent->s.number < MAX_CLIENTS) ) {
					// a bad model
					trap->G2API_CleanGhoul2Models( &(ent->ghoul2) );
					ent->ghoul2 = NULL;
					trap->G2API_DuplicateGhoul2Instance( precachedKyle, &ent->ghoul2 );
				}

				if ( ent->s.number >= MAX_CLIENTS ) {
					ent->s.modelGhoul2 = 1; //so we know to free it on the client when we're removed.

					if ( skin[0] ) { //append it after a *
						strcat( modelFullPath, va( "*%s", skin ) );
					}

					if ( ent->client->NPC_class == CLASS_VEHICLE ) { //vehicles are tricky and send over their vehicle names as the model (the model is then retrieved based on the vehicle name)
						ent->s.modelindex = G_ModelIndex( vehicleName );
					}
					else {
						ent->s.modelindex = G_ModelIndex( modelFullPath );
					}
				}
			}
		}
		else {
			trap->G2API_DuplicateGhoul2Instance( precachedKyle, &ent->ghoul2 );
		}
	}
	else {
		return;
	}

	//Attach the instance to this entity num so we can make use of client-server
	//shared operations if possible.
	trap->G2API_AttachInstanceToEntNum( ent->ghoul2, ent->s.number, qtrue );

	// The model is now loaded.

	GLAName[0] = 0;

	if ( !BGPAFtextLoaded ) {
		if ( BG_ParseAnimationFile( "models/players/_humanoid/animation.cfg", bgHumanoidAnimations, qtrue ) == -1 ) {
			Com_Printf( "Failed to load humanoid animation file\n" );
			return;
		}
	}

	if ( ent->s.number >= MAX_CLIENTS ) {
		ent->localAnimIndex = -1;

		GLAName[0] = 0;
		trap->G2API_GetGLAName( ent->ghoul2, 0, GLAName );

		if ( GLAName[0] &&
			!strstr( GLAName, "players/_humanoid/" ) /*&&
			!strstr(GLAName, "players/rockettrooper/")*/ ) { //it doesn't use humanoid anims.
			char *slash = Q_strrchr( GLAName, '/' );
			if ( slash ) {
				strcpy( slash, "/animation.cfg" );

				ent->localAnimIndex = BG_ParseAnimationFile( GLAName, NULL, qfalse );
			}
		}
		else { //humanoid index.
			if ( strstr( GLAName, "players/rockettrooper/" ) ) {
				ent->localAnimIndex = 1;
			}
			else {
				ent->localAnimIndex = 0;
			}
		}

		if ( ent->localAnimIndex == -1 ) {
			Com_Error( ERR_DROP, "NPC had an invalid GLA\n" );
		}
	}
	else {
		GLAName[0] = 0;
		trap->G2API_GetGLAName( ent->ghoul2, 0, GLAName );

		if ( strstr( GLAName, "players/rockettrooper/" ) ) {
			//assert(!"Should not have gotten in here with rockettrooper skel");
			ent->localAnimIndex = 1;
		}
		else {
			ent->localAnimIndex = 0;
		}
	}

	if ( ent->s.NPC_class == CLASS_VEHICLE &&
		ent->m_pVehicle ) { //do special vehicle stuff
		char strTemp[128];
		int i;

		// Setup the default first bolt
		trap->G2API_AddBolt( ent->ghoul2, 0, "model_root" );

		// Setup the droid unit.
		ent->m_pVehicle->m_iDroidUnitTag = trap->G2API_AddBolt( ent->ghoul2, 0, "*droidunit" );

		// Setup the Exhausts.
		for ( i = 0; i < MAX_VEHICLE_EXHAUSTS; i++ ) {
			Com_sprintf( strTemp, 128, "*exhaust%i", i + 1 );
			ent->m_pVehicle->m_iExhaustTag[i] = trap->G2API_AddBolt( ent->ghoul2, 0, strTemp );
		}

		// Setup the Muzzles.
		for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ ) {
			Com_sprintf( strTemp, 128, "*muzzle%i", i + 1 );
			ent->m_pVehicle->m_iMuzzleTag[i] = trap->G2API_AddBolt( ent->ghoul2, 0, strTemp );
			if ( ent->m_pVehicle->m_iMuzzleTag[i] == -1 ) {//ergh, try *flash?
				Com_sprintf( strTemp, 128, "*flash%i", i + 1 );
				ent->m_pVehicle->m_iMuzzleTag[i] = trap->G2API_AddBolt( ent->ghoul2, 0, strTemp );
			}
		}

		// Setup the Turrets.
		for ( i = 0; i < MAX_VEHICLE_TURRET_MUZZLES; i++ ) {
			if ( ent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag ) {
				ent->m_pVehicle->m_iGunnerViewTag[i] = trap->G2API_AddBolt( ent->ghoul2, 0, ent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag );
			}
			else {
				ent->m_pVehicle->m_iGunnerViewTag[i] = -1;
			}
		}
	}

	if ( ent->client->ps.weapon == WP_SABER || ent->s.number < MAX_CLIENTS ) { //a player or NPC saber user
		trap->G2API_AddBolt( ent->ghoul2, 0, "*r_hand" );
		trap->G2API_AddBolt( ent->ghoul2, 0, "*l_hand" );

		//rhand must always be first bolt. lhand always second. Whichever you want the
		//jetpack bolted to must always be third.
		trap->G2API_AddBolt( ent->ghoul2, 0, "*chestg" );

		//claw bolts
		trap->G2API_AddBolt( ent->ghoul2, 0, "*r_hand_cap_r_arm" );
		trap->G2API_AddBolt( ent->ghoul2, 0, "*l_hand_cap_l_arm" );

		trap->G2API_SetBoneAnim( ent->ghoul2, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, level.time, -1, -1 );
		trap->G2API_SetBoneAngles( ent->ghoul2, 0, "upper_lumbar", &tempVec, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, level.time );
		trap->G2API_SetBoneAngles( ent->ghoul2, 0, "cranium", &tempVec, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, POSITIVE_X, NULL, 0, level.time );

		if ( !g2SaberInstance ) {
			trap->G2API_InitGhoul2Model( &g2SaberInstance, "models/weapons2/saber/saber_w.glm", 0, 0, -20, 0, 0 );

			if ( g2SaberInstance ) {
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap->G2API_SetBoltInfo( g2SaberInstance, 0, 0 );
				// now set up the gun bolt on it
				trap->G2API_AddBolt( g2SaberInstance, 0, "*blade1" );
			}
		}

		if ( G_SaberModelSetup( ent ) ) {
			if ( g2SaberInstance ) {
				trap->G2API_CopySpecificGhoul2Model( g2SaberInstance, 0, ent->ghoul2, 1 );
			}
		}
	}

	if ( ent->s.number >= MAX_CLIENTS ) { //some extra NPC stuff
		if ( trap->G2API_AddBolt( ent->ghoul2, 0, "lower_lumbar" ) == -1 ) { //check now to see if we have this bone for setting anims and such
			ent->noLumbar = qtrue;
		}
	}
}

void G_ValidateSiegeClassForTeam( gentity_t *ent, int team );

typedef struct userinfoValidate_s {
	const char		*field, *fieldClean;
	unsigned int	minCount, maxCount;
} userinfoValidate_t;

#define UIF( x, _min, _max ) { STRING(\\) #x STRING(\\), STRING( x ), _min, _max }
static userinfoValidate_t userinfoFields[] = {
	UIF( cl_guid, 0, 0 ), // not allowed, q3fill protection
	UIF( cl_punkbuster, 0, 0 ), // not allowed, q3fill protection
	UIF( ip, 0, 1 ), // engine adds this at the end
	UIF( cp_pluginDisable, 0, 1 ), // JA+
	UIF( name, 0, 1 ),
	UIF( rate, 1, 1 ),
	UIF( snaps, 1, 1 ),
	UIF( model, 1, 1 ),
	UIF( forcepowers, 1, 1 ),
	UIF( color1, 1, 1 ),
	UIF( color2, 1, 1 ),
	UIF( handicap, 1, 1 ),
	UIF( sex, 0, 1 ),
	UIF( cg_predictItems, 1, 1 ),
	UIF( saber1, 1, 1 ),
	UIF( saber2, 1, 1 ),
	UIF( char_color_red, 1, 1 ),
	UIF( char_color_green, 1, 1 ),
	UIF( char_color_blue, 1, 1 ),
	UIF( cp_sbRGB1, 0, 1 ), // JA+
	UIF( cp_sbRGB2, 0, 1 ), // JA+
	UIF( teamtask, 0, 1 ),
	UIF( password, 0, 1 ), // optional
	UIF( cjp_client, 0, 1 ), // JA+
	UIF( csf, 0, 1 ), // JA++
	UIF( teamoverlay, 0, 1 ), // only registered in cgame, not sent when connecting
};
static const size_t numUserinfoFields = ARRAY_LEN( userinfoFields );

static const char *userinfoValidateExtra[USERINFO_VALIDATION_MAX] = {
	"Size",					// USERINFO_VALIDATION_SIZE
	"# of slashes",			// USERINFO_VALIDATION_SLASH
	"Extended ascii",		// USERINFO_VALIDATION_EXTASCII
	"Control characters",	// USERINFO_VALIDATION_CONTROLCHARS
};

void SV_ToggleUserinfoValidation_f( void ) {
	if ( trap->Argc() == 1 ) {
		unsigned int i = 0;
		for ( i = 0; i < numUserinfoFields; i++ ) {
			if ( (japp_userinfoValidate.bits & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, userinfoFields[i].fieldClean );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, userinfoFields[i].fieldClean );
			}
		}
		for ( ; i < numUserinfoFields + USERINFO_VALIDATION_MAX; i++ ) {
			if ( (japp_userinfoValidate.bits & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, userinfoValidateExtra[i - numUserinfoFields] );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, userinfoValidateExtra[i - numUserinfoFields] );
			}
		}

		return;
	}
	else {
		char arg[8] = { 0 };
		uint32_t index;
		const uint32_t mask = (1 << (numUserinfoFields + USERINFO_VALIDATION_MAX)) - 1;

		trap->Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		if ( index > numUserinfoFields + USERINFO_VALIDATION_MAX - 1 ) {
			trap->Print( "ToggleUserinfoValidation: Invalid range: %i [0, %i]\n", index,
				numUserinfoFields + USERINFO_VALIDATION_MAX - 1 );
			return;
		}

		trap->Cvar_Set( "japp_userinfoValidate", va( "%i", (1 << index) ^ (japp_userinfoValidate.bits & mask) ) );
		trap->Cvar_Update( &japp_userinfoValidate );

		if ( index < numUserinfoFields ) {
			trap->Print( "%s %s\n", userinfoFields[index].fieldClean,
				((japp_userinfoValidate.bits & (1 << index)) ? "Validated" : "Ignored") );
		}
		else {
			trap->Print( "%s %s\n", userinfoValidateExtra[index - numUserinfoFields],
				((japp_userinfoValidate.bits & (1 << index)) ? "Validated" : "Ignored") );
		}
	}
}

static const char *G_ValidateUserinfo( const char *userinfo ) {
	unsigned int		fieldCount[ARRAY_LEN( userinfoFields )];

	memset( fieldCount, 0, sizeof(fieldCount) );

	// size checks
	const size_t length = strlen( userinfo );
	if ( japp_userinfoValidate.bits & (1 << (numUserinfoFields + USERINFO_VALIDATION_SIZE)) ) {
		if ( length < 1 ) {
			return "Userinfo too short";
		}
		else if ( length >= MAX_INFO_STRING ) {
			return "Userinfo too long";
		}
	}

	// slash checks
	if ( japp_userinfoValidate.bits & (1 << (numUserinfoFields + USERINFO_VALIDATION_SLASH)) ) {
		// there must be a leading slash
		if ( userinfo[0] != '\\' ) {
			return "Missing leading slash";
		}

		// no trailing slashes allowed, engine will append ip\\ip:port
		if ( userinfo[length - 1] == '\\' ) {
			return "Trailing slash";
		}

		// format for userinfo field is: \\key\\value
		// so there must be an even amount of slashes
		uint32_t count = 0;
		for ( int i = 0; i < length; i++ ) {
			if ( userinfo[i] == '\\' ) {
				count++;
			}
		}
		if ( (count & 1) ) {
			// odd
			return "Bad number of slashes";
		}
	}

	// extended characters are impossible to type, may want to disable
	if ( japp_userinfoValidate.bits & (1 << (numUserinfoFields + USERINFO_VALIDATION_EXTASCII)) ) {
		uint32_t count = 0u;
		for ( int i = 0; i < length; i++ ) {
			if ( userinfo[i] < 0 ) {
				count++;
			}
		}
		if ( count ) {
			return "Extended ASCII characters found";
		}
	}

	// disallow \n \r ; and \"
	if ( japp_userinfoValidate.bits & (1 << (numUserinfoFields + USERINFO_VALIDATION_CONTROLCHARS)) ) {
		if ( Q_strchrs( userinfo, "\n\r\t;\"" ) ) {
			return "Invalid characters found";
		}
	}

	infoPair_t ip;
	const char *s = userinfo;
	while ( s ) {
		if ( !Info_NextPair( &s, &ip ) ) {
			break;
		}

		for ( int i = 0; i < numUserinfoFields; i++ ) {
			if ( !Q_stricmp( ip.key, userinfoFields[i].fieldClean ) ) {
				fieldCount[i]++;
			}
		}
	}

	// count the number of fields
	userinfoValidate_t	*info = userinfoFields;
	for ( int i = 0; i < numUserinfoFields; i++, info++ ) {
		if ( japp_userinfoValidate.bits & (1 << i) ) {
			if ( info->minCount && !fieldCount[i] ) {
				return va( "%s field not found", info->fieldClean );
			}
			else if ( fieldCount[i] > info->maxCount ) {
				return va( "Too many %s fields (%i/%i)", info->fieldClean, fieldCount[i], info->maxCount );
			}
		}
	}

	return nullptr;
}

extern void G_ResetSaberStyle(gentity_t *ent);
// Called from ClientConnect when the player first connects and directly by the server system when the player updates a
//	userinfo variable.
// The game can override any of the settings and call trap->SetUserinfo if desired.
qboolean ClientUserinfoChanged( int clientNum ) {
	gentity_t *ent = g_entities + clientNum;
	gclient_t *client = ent->client;
	team_t team = TEAM_FREE;
	int health = 100, maxHealth = 100;
	const char *s = NULL, *value = NULL;
	char userinfo[MAX_INFO_STRING], buf[MAX_INFO_STRING], oldClientinfo[MAX_INFO_STRING], model[MAX_QPATH],
		forcePowers[DEFAULT_FORCEPOWERS_LEN], oldname[MAX_NETNAME], className[MAX_QPATH], color1[16], color2[16],
		cp_sbRGB1[MAX_INFO_STRING], cp_sbRGB2[MAX_INFO_STRING];
	qboolean modelChanged = qfalse;
	gender_t gender = GENDER_MALE;

	trap->GetUserinfo( clientNum, userinfo, sizeof(userinfo) );

	if ( JPLua::Event_ClientUserinfoChanged( clientNum, userinfo ) ) {
		trap->SetUserinfo( clientNum, userinfo );
	}

	//Raz: Scooper's code for userinfo spamming
	if ( japp_antiUserinfoFlood.integer && !(ent->r.svFlags & SVF_BOT) ) {
		ent->userinfoChanged = level.time;

		if ( ent->userinfoSpam > 12 ) {
			G_LogPrintf( level.log.security, "Client %s userinfo changing too fast. Ignored.\n", G_PrintClient( ent ) );
			trap->SendServerCommand( ent - g_entities, va( "print \"" S_COLOR_YELLOW "Userinfo changing too fast. Ignored.\n\"" ) );
			return qfalse;
		}
		ent->userinfoSpam++;
	}

	// check for malformed or illegal info strings
	s = G_ValidateUserinfo( userinfo );
	if ( s && *s ) {
		G_LogPrintf( level.log.security, "Client %s failed userinfo validation: %s\n", G_PrintClient( ent ), s );
		trap->DropClient( clientNum, va( "Failed userinfo validation: %s", s ) );
		G_LogPrintf( level.log.security, "Userinfo: %s\n", userinfo );
		return qfalse;
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	if ( !strcmp( s, "localhost" ) && !(ent->r.svFlags & SVF_BOT) ) {
		client->pers.localClient = qtrue;
	}

	//Raz: cp_pluginDisable
	s = Info_ValueForKey( userinfo, "cp_pluginDisable" );
	if ( s[0] && sscanf( s, "%u", &client->pers.CPD ) != 1 ) {
		G_LogPrintf( level.log.security, "ClientUserinfoChanged(): Client %i (%s) userinfo 'cp_pluginDisable' was found"
			", but invalid. [IP: %s]\n", clientNum, client->pers.netname, client->sess.IP );
	}

	s = Info_ValueForKey( userinfo, "cjp_client" );
	if ( s[0] ) {
		client->pers.CSF |= JAPLUS_CLIENT_FLAGS;
	}

	// check the item prediction
	client->pers.predictItemPickup = !!atoi( Info_ValueForKey( userinfo, "cg_predictItems" ) );

	// set name
	Q_strncpyz( oldname, client->pers.netname, sizeof(oldname) );
	s = Info_ValueForKey( userinfo, "name" );
	ClientCleanName( s, client->pers.netname, sizeof(client->pers.netname) );

	Q_strncpyz( client->pers.netnameClean, client->pers.netname, sizeof(client->pers.netnameClean) );
	Q_CleanString( client->pers.netnameClean, STRIP_COLOUR );

	if ( CheckDuplicateName( clientNum ) ) {
		Q_strncpyz( client->pers.netnameClean, client->pers.netname, sizeof(client->pers.netnameClean) );
		Q_CleanString( client->pers.netnameClean, STRIP_COLOUR );
	}

	if ( client->sess.sessionTeam == TEAM_SPECTATOR && client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		Q_strncpyz( client->pers.netname, "scoreboard", sizeof(client->pers.netname) );
		Q_strncpyz( client->pers.netnameClean, "scoreboard", sizeof(client->pers.netnameClean) );
	}

	if ( client->pers.connected == CON_CONNECTED && strcmp( oldname, client->pers.netname ) ) {
		if ( client->pers.netnameTime > level.time ) {
			trap->SendServerCommand( clientNum, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME",
				"NONAMECHANGE" ) ) );

			Info_SetValueForKey( userinfo, "name", oldname );
			trap->SetUserinfo( clientNum, userinfo );
			Q_strncpyz( client->pers.netname, oldname, sizeof(client->pers.netname) );
			Q_strncpyz( client->pers.netnameClean, oldname, sizeof(client->pers.netnameClean) );
			Q_CleanString( client->pers.netnameClean, STRIP_COLOUR );
		}
		else if ( client->pers.adminData.renamedTime != 0
			&& client->pers.adminData.renamedTime > level.time - (japp_amrenameTime.value * 60.0f) * 1000 )
		{
			float remaining = japp_amrenameTime.value * 60.0f;
			remaining -= (level.time - client->pers.adminData.renamedTime) / 1000.0f;
			trap->SendServerCommand( clientNum, va( "print \"You are not allowed to change name for another "
				S_COLOR_CYAN "%.1f " S_COLOR_WHITE "seconds\n\"", remaining ) );
		}
		else {
			trap->SendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " %s %s\n\"", oldname,
				G_GetStringEdString( "MP_SVGAME", "PLRENAME" ), client->pers.netname ) );
			G_LogPrintf( level.log.console, "ClientRename: %i [%s] \"%s^7\" -> \"%s^7\"\n", clientNum,
				ent->client->sess.IP, oldname, ent->client->pers.netname );
			client->pers.netnameTime = level.time + 5000;
		}
	}

	// set model
	Q_strncpyz( model, Info_ValueForKey( userinfo, "model" ), sizeof(model) );

	if ( d_perPlayerGhoul2.integer && Q_stricmp( model, client->modelname ) ) {
		Q_strncpyz( client->modelname, model, sizeof(client->modelname) );
		modelChanged = qtrue;
	}

	// custom skin RGB
	value = Info_ValueForKey( userinfo, "char_color_red" );
	client->ps.customRGBA[0] = value ? Q_clampi( 0, atoi( value ), 255 ) : 255;
	value = Info_ValueForKey( userinfo, "char_color_green" );
	client->ps.customRGBA[1] = value ? Q_clampi( 0, atoi( value ), 255 ) : 255;
	value = Info_ValueForKey( userinfo, "char_color_blue" );
	client->ps.customRGBA[2] = value ? Q_clampi( 0, atoi( value ), 255 ) : 255;
	client->ps.customRGBA[3] = 255;
	if ( japp_charRestrictRGB.integer
		&& ((client->ps.customRGBA[0] + client->ps.customRGBA[1] + client->ps.customRGBA[2]) < 100) )
	{
		client->ps.customRGBA[0] = client->ps.customRGBA[1] = client->ps.customRGBA[2] = 255;
	}

	Q_strncpyz( forcePowers, Info_ValueForKey( userinfo, "forcepowers" ), sizeof(forcePowers) );

	// update our customRGBA for team colors.
	if ( level.gametype >= GT_TEAM && level.gametype != GT_SIEGE && !g_jediVmerc.integer ) {
		char skin[MAX_QPATH] = { 0 };
		vector3 colorOverride;

		VectorClear( &colorOverride );

		BG_ValidateSkinForTeam( model, skin, client->sess.sessionTeam, &colorOverride );
		if ( colorOverride.r > 0.0f || colorOverride.g > 0.0f || colorOverride.b > 0.0f ) {
			client->ps.customRGBA[0] = colorOverride.r * 255.0f;
			client->ps.customRGBA[1] = colorOverride.g * 255.0f;
			client->ps.customRGBA[2] = colorOverride.b * 255.0f;
		}
	}

	// bots set their team a few frames later
	if ( level.gametype >= GT_TEAM && (g_entities[clientNum].r.svFlags & SVF_BOT) ) {
		s = Info_ValueForKey( userinfo, "team" );
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		}
		else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		}
		else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}
	}
	else {
		team = client->sess.sessionTeam;
	}

	//Set the siege class
	if ( level.gametype == GT_SIEGE ) {
		Q_strncpyz( className, client->sess.siegeClass, sizeof(className) );

		//Now that the team is legal for sure, we'll go ahead and get an index for it.
		client->siegeClass = BG_SiegeFindClassIndexByName( className );
		if ( client->siegeClass == -1 ) {
			// ok, get the first valid class for the team you're on then, I guess.
			BG_SiegeCheckClassLegality( team, className );
			Q_strncpyz( client->sess.siegeClass, className, sizeof(client->sess.siegeClass) );
			client->siegeClass = BG_SiegeFindClassIndexByName( className );
		}
		else {
			// otherwise, make sure the class we are using is legal.
			G_ValidateSiegeClassForTeam( ent, team );
			Q_strncpyz( className, client->sess.siegeClass, sizeof(className) );
		}

		if ( client->siegeClass != -1 ) {
			// Set the sabers if the class dictates
			siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];

			G_SetSaber( ent, 0, scl->saber1[0] ? scl->saber1 : DEFAULT_SABER, qtrue );
			G_SetSaber( ent, 1, scl->saber2[0] ? scl->saber2 : "none", qtrue );

			//make sure the saber models are updated
			G_SaberModelSetup(ent);

			if (japp_instantSaberSwitch.integer){
				const char *saber, *key, *value;
				trap->GetUserinfo(ent - g_entities, userinfo, sizeof(userinfo));
				for (int i = 0; i < MAX_SABERS; i++) {
					saber = (i & 1) ? ent->client->pers.saber2 : ent->client->pers.saber1;
					key = va("saber%d", i + 1);
					value = Info_ValueForKey(userinfo, key);
					if (Q_stricmp(value, saber)) {
						// they don't match up, force the user info
						Info_SetValueForKey(userinfo, key, saber);
						trap->SetUserinfo(ent - g_entities, userinfo);
					}
				}
				G_ResetSaberStyle(ent);
			}

			if ( scl->forcedModel[0] ) {
				// be sure to override the model we actually use
				Q_strncpyz( model, scl->forcedModel, sizeof(model) );
				if ( d_perPlayerGhoul2.integer && Q_stricmp( model, client->modelname ) ) {
					Q_strncpyz( client->modelname, model, sizeof(client->modelname) );
					modelChanged = qtrue;
				}
			}
		}
	}
	else {
		Q_strncpyz( className, "none", sizeof(className) );
	}

	// only set the saber name on the first connect
	// it will be read from userinfo on ClientSpawn and stored in client->pers.saber1/2
	//RAZTODO: instantly switch optionally?
	if ( !VALIDSTRING( client->pers.saber1 ) || !VALIDSTRING( client->pers.saber2 ) ) {
		G_SetSaber( ent, 0, Info_ValueForKey( userinfo, "saber1" ), qfalse );
		G_SetSaber( ent, 1, Info_ValueForKey( userinfo, "saber2" ), qfalse );
	}

	// set max health
	if ( level.gametype == GT_SIEGE && client->siegeClass != -1 ) {
		siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];

		if ( scl->maxhealth ) {
			maxHealth = scl->maxhealth;
		}

		health = maxHealth;
	}
	else {
		health = Q_clampi( 1, atoi( Info_ValueForKey( userinfo, "handicap" ) ), 100 );
	}

	client->pers.maxHealth = health;
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > maxHealth ) {
		client->pers.maxHealth = 100;
	}
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	if ( level.gametype >= GT_TEAM ) {
		client->pers.teamInfo = qtrue;
	}
	else {
		s = Info_ValueForKey( userinfo, "teamoverlay" );
		if ( !*s || atoi( s ) != 0 ) {
			client->pers.teamInfo = qtrue;
		}
		else {
			client->pers.teamInfo = qfalse;
		}
	}

	// colors
	Q_strncpyz( color1, Info_ValueForKey( userinfo, "color1" ), sizeof(color1) );
	Q_strncpyz( color2, Info_ValueForKey( userinfo, "color2" ), sizeof(color2) );
	Q_strncpyz( cp_sbRGB1, Info_ValueForKey( userinfo, "cp_sbRGB1" ), sizeof(cp_sbRGB1) );
	Q_strncpyz( cp_sbRGB2, Info_ValueForKey( userinfo, "cp_sbRGB2" ), sizeof(cp_sbRGB2) );

	//Raz: Gender hints
	s = Info_ValueForKey( userinfo, "sex" );
	if ( !Q_stricmp( s, "male" ) ) {
		gender = GENDER_MALE;
	}
	else if ( !Q_stricmp( s, "female" ) ) {
		gender = GENDER_FEMALE;
	}
	else {
		gender = GENDER_NEUTER;
	}

	s = Info_ValueForKey( userinfo, "snaps" );
	if ( atoi( s ) < sv_fps.integer ) {
		trap->SendServerCommand( clientNum, va( "print \"" S_COLOR_YELLOW "Recommend setting /snaps %d or higher to "
			"match this server's sv_fps\n\"", sv_fps.integer ) );
	}

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	buf[0] = '\0';
	Q_strcat( buf, sizeof(buf), va( "n\\%s\\", client->pers.netname ) );
	Q_strcat( buf, sizeof(buf), va( "t\\%i\\", client->sess.sessionTeam ) );
	Q_strcat( buf, sizeof(buf), va( "model\\%s\\", model ) );
	if ( gender == GENDER_MALE ) {
		Q_strcat( buf, sizeof(buf), va( "ds\\%c\\", 'm' ) );
	}
	else if ( gender == GENDER_FEMALE ) {
		Q_strcat( buf, sizeof(buf), va( "ds\\%c\\", 'f' ) );
	}
	else {
		Q_strcat( buf, sizeof(buf), va( "ds\\%c\\", 'n' ) );
	}
	Q_strcat( buf, sizeof(buf), va( "st\\%s\\", client->pers.saber1 ) );
	Q_strcat( buf, sizeof(buf), va( "st2\\%s\\", client->pers.saber2 ) );
	Q_strcat( buf, sizeof(buf), va( "c1\\%s\\", color1 ) );
	Q_strcat( buf, sizeof(buf), va( "c2\\%s\\", color2 ) );
	Q_strcat( buf, sizeof(buf), va( "c3\\%s\\", cp_sbRGB1 ) );
	Q_strcat( buf, sizeof(buf), va( "c4\\%s\\", cp_sbRGB2 ) );
	Q_strcat( buf, sizeof(buf), va( "hc\\%i\\", client->pers.maxHealth ) );
	if ( ent->r.svFlags & SVF_BOT ) {
		Q_strcat( buf, sizeof(buf), va( "skill\\%s\\", Info_ValueForKey( userinfo, "skill" ) ) );
	}
	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		Q_strcat( buf, sizeof(buf), va( "w\\%i\\", client->sess.wins ) );
		Q_strcat( buf, sizeof(buf), va( "l\\%i\\", client->sess.losses ) );
	}
	if ( level.gametype == GT_POWERDUEL ) {
		Q_strcat( buf, sizeof(buf), va( "dt\\%i\\", client->sess.duelTeam ) );
	}
	if ( level.gametype == GT_SIEGE ) {
		Q_strcat( buf, sizeof(buf), va( "siegeclass\\%s\\", className ) );
		Q_strcat( buf, sizeof(buf), va( "sdt\\%i\\", className ) );
	}

	trap->GetConfigstring( CS_PLAYERS + clientNum, oldClientinfo, sizeof(oldClientinfo) );
	trap->SetConfigstring( CS_PLAYERS + clientNum, buf );

	// only going to be true for allowable server-side custom skeleton cases
	if ( modelChanged ) {
		// update the server g2 instance if appropriate
		char modelname[MAX_QPATH];
		Q_strncpyz( modelname, Info_ValueForKey( userinfo, "model" ), sizeof(modelname) );
		SetupGameGhoul2Model( ent, modelname, NULL );
		Info_SetValueForKey( userinfo, "model", modelname );

		if ( ent->ghoul2 && ent->client ) {
			ent->client->renderInfo.lastG2 = NULL; //update the renderinfo bolts next update.
		}

		client->torsoAnimExecute = client->legsAnimExecute = -1;
		client->torsoLastFlip = client->legsLastFlip = qfalse;
	}

	if ( g_logClientInfo.integer ) {
		if ( strcmp( oldClientinfo, buf ) ) {
			G_LogPrintf( level.log.console, "ClientUserinfoChanged: %i %s\n", clientNum, buf );
		}
		else {
			G_LogPrintf( level.log.console, "ClientUserinfoChanged: %i <no change>\n", clientNum );
		}
	}

	return qtrue;
}

static qboolean CompareIPString( const char *ip1, const char *ip2 ) {
	while ( 1 ) {
		if ( *ip1 != *ip2 ) {
			return qfalse;
		}
		if ( !*ip1 || *ip1 == ':' ) {
			break;
		}
		ip1++, ip2++;
	}

	return qtrue;
}

// called when a player begins connecting to the server
// called again for every map change or tournament restart
// The session information will be valid after exit
// Return NULL if the client should be allowed, otherwise return a string with the reason for denial
// Otherwise, the client will be sent the current gamestate and will eventually get to ClientBegin
// firstTime will be qtrue the very first time a client connects to the server machine, but qfalse on map changes and
//	tournament restarts.
static std::unordered_map<int, GeoIPData*> pending_list;

const char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	gentity_t *ent = g_entities + clientNum, *te = NULL;
	gclient_t *client;
	uint32_t finalCSF = 0;
	char userinfo[MAX_INFO_STRING], tmpIP[NET_ADDRSTRMAXLEN], tmp[MAX_INFO_VALUE];
	const char *result = NULL, *value = NULL;

	ent->s.number = clientNum;
	ent->classname = "connecting";

	trap->GetUserinfo( clientNum, userinfo, sizeof(userinfo) );

	// ban checks
	value = Info_ValueForKey( userinfo, "ip" );
	Q_strncpyz( tmpIP, isBot ? "Bot" : value, sizeof(tmpIP) );

	if ( !isBot && firstTime && Q_stricmp( value, "localhost" ) ) {
		byteAlias_t *ba = BuildByteFromIP( value );
		const char *reason = JP_Bans_IsBanned( ba->b );
		if ( reason ) {
			return reason;
		}
	}

	// password checks
	if ( !isBot && g_needpass.integer ) {
		// check for a password
		value = Info_ValueForKey( userinfo, "password" );
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) && strcmp( g_password.string, value ) ) {
			return G_GetStringEdString( "MP_SVGAME", "INVALID_ESCAPE_TO_MAIN" );
		}
	}

	// disallow multiple connections from same IP
	if ( !isBot && firstTime ) {
		if ( japp_antiFakePlayer.integer ) {
			// check for > g_maxConnPerIP connections from same IP
			int count = 0, i = 0;
			for ( i = 0; i<sv_maxclients.integer; i++ ) {
				if ( CompareIPString( tmpIP, level.clients[i].sess.IP ) ) {
					count++;
				}
			}
			if ( count > japp_maxConnPerIP.integer ) {
				return "Too many connections from the same IP";
			}
		}
	}

	if ( ent->inuse ) {
		// if a player reconnects quickly after a disconnect, the client disconnect may never be called, thus flag can
		//	get lost in the ether, so lets just fix up anything that should happen on a disconnect
		G_LogPrintf( level.log.console, "Forcing disconnect on active client: %i\n", clientNum );
		ClientDisconnect( clientNum );
	}

	// preliminary userinfo validation, client support detection
	if ( !isBot ) {
		char msg[2048] = { 0 };

		// cjp_client
		Q_strncpyz( tmp, Info_ValueForKey( userinfo, "cjp_client" ), sizeof(tmp) );
		if ( Q_strchrs( tmp, "\n\r;\"" ) ) {
			// sent, but malicious
			G_LogPrintf( level.log.security, "ClientConnect(%d) Spoofed userinfo 'cjp_client'. [IP: %s]\n", clientNum, tmpIP );
			return "Invalid userinfo detected";
		}
		Q_strcat( msg, sizeof(msg), va( "cjp_client: %s...", tmp[0] ? tmp : "basejka" ) );
		if ( tmp[0] ) {
			// sent, they must at-least have JA+ support flags
			Q_strcat( msg, sizeof(msg), va( " assumed JA+ csf 0x%X...", JAPLUS_CLIENT_FLAGS ) );
			finalCSF = JAPLUS_CLIENT_FLAGS;
		}
		else {
			// not sent, assume base client
			Q_strcat( msg, sizeof(msg), va( " assumed basejka csf 0x0..." ) );
		}

		// client support hinting
		value = Info_ValueForKey( userinfo, "csf" );
		if ( Q_strchrs( value, "\n\r;\"" ) ) {
			// sent, but malicious
			G_LogPrintf( level.log.security, "ClientConnect(%d): Spoofed userinfo 'csf'. [IP: %s]\n", clientNum, tmpIP );
			return "Invalid userinfo detected";
		}

		if ( value[0] && sscanf( value, "%X", &finalCSF ) != 1 ) {
			// sent, but failed to parse. probably malicious
			G_LogPrintf( level.log.security, "ClientConnect(%d): userinfo 'csf' was found, but empty. [IP: %s]\n", clientNum, tmpIP );
			return "Invalid userinfo detected";
		}
		Q_strcat( msg, sizeof(msg), va( " final csf 0x%X\n", finalCSF ) );

		G_LogPrintf( level.log.console, msg );
	}

	// JPLua plugins can deny connections
	if ( (result = JPLua::Event_ClientConnect( clientNum, userinfo, tmpIP, firstTime )) ) {
		G_LogPrintf( level.log.console, "Connection from %s denied by plugin: %s\n", tmpIP, result );
		return result;
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

	// assign the pointer for bg entity access
	ent->playerState = &ent->client->ps;

	memset( client, 0, sizeof(*client) );

	client->pers.connected = CON_CONNECTING;
	client->pers.connectTime = level.time;
	client->pers.CSF = finalCSF;

	// read or initialize the session data
	if ( firstTime || level.newSession ) {
		G_InitClientSessionData( client, userinfo, isBot );
	}
	G_ReadClientSessionData( client );

	// if this is the first time then auto-assign a desired siege team and show briefing for that team
	if ( level.gametype == GT_SIEGE && (firstTime || level.newSession) ) {
		client->sess.siegeDesiredTeam = 0;
	}

	if ( level.gametype == GT_SIEGE && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		if ( firstTime || level.newSession ) {
			// start as spec
			client->sess.siegeDesiredTeam = client->sess.sessionTeam;
			client->sess.sessionTeam = TEAM_SPECTATOR;
		}
	}
	else if ( level.gametype == GT_POWERDUEL && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		client->sess.sessionTeam = TEAM_SPECTATOR;
	}

	if ( isBot ) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if ( !G_BotConnect( clientNum, !firstTime ) ) {
			return "BotConnectfailed";
		}
	}

	if ( !ClientUserinfoChanged( clientNum ) ) {
		return "Failed userinfo validation";
	}

	if ( !isBot && firstTime ) {
		if ( !tmpIP[0] ) {
			// No IP sent when connecting, probably an unban hack attempt
			client->pers.connected = CON_DISCONNECTED;
			G_LogPrintf( level.log.security, "Client %i (%s) sent no IP when connecting.\n", clientNum,
				client->pers.netname );
			return "Invalid userinfo detected";
		}
		Q_strncpyz( client->sess.IP, tmpIP, sizeof(client->sess.IP) );
	}

	// get GeoIP data
	if ( japp_detectCountry.integer && !isBot && firstTime && Q_stricmp( value, "localhost" ) ) {
		GeoIPData *gip = pending_list[clientNum];
		if ( !gip ) {
			gip = GeoIP::GetIPInfo( value );
			if ( gip ) {
				pending_list[clientNum] = gip;
				return "Please wait...";
			}
			// else GeoIP stuff isn't working
		}
		else if ( gip ) {
			if ( !gip->isReady() ) {
				return "Please wait...";
			}
			if ( gip->getStatus() ) {
				const char *sData = gip->getData()->c_str();
				trap->Print( "Got GeoIP data. IP: %s Data: %s",
					gip->getIp().c_str(), sData
				);
				Q_strncpyz( ent->client->sess.geoipData, sData, sizeof(ent->client->sess.geoipData) );
			}
			else {
				trap->Print( "Failed to Get GeoIP data. IP: %s Error: %s",
					gip->getIp().c_str(), gip->getData()->c_str()
				);
				Q_strncpyz( ent->client->sess.geoipData, "Unknown", sizeof(ent->client->sess.geoipData) );
			}
			delete gip;
			pending_list[clientNum] = nullptr;
		}
	}

	G_LogPrintf(level.log.console, "ClientConnect: %i (%s" S_COLOR_WHITE ") [IP: %s" S_COLOR_WHITE "]\n", clientNum, client->pers.netname, tmpIP );

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime ) {
		trap->SendServerCommand( -1,
			va( "print \"%s" S_COLOR_WHITE " %s From: %s\n\"",
				client->pers.netname,
				G_GetStringEdString( "MP_SVGAME", "PLCONNECT" ),
				ent->client->sess.geoipData
			)
		);
	}

	if ( level.gametype >= GT_TEAM && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BroadcastTeamChange( client, -1 );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	te = G_TempEntity( &vec3_origin, EV_CLIENTJOIN );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = clientNum;

	return NULL;
}

// called when a client has finished connecting, and is ready to be placed into the level
// this will happen every level load, and on transition between teams, but doesn't happen on respawns
void ClientBegin( int clientNum, qboolean allowTeamReset ) {
	gentity_t *ent = g_entities + clientNum;
	gentity_t *tent;
	gclient_t *client;
	uint32_t flags;
	int spawnCount;
	char userinfo[MAX_INFO_VALUE];
	char modelname[MAX_QPATH];

	if ( (ent->r.svFlags & SVF_BOT) && level.gametype >= GT_TEAM ) {
		if ( allowTeamReset ) {
			const char *team = "Red";
			int preSess;

			//SetTeam(ent, "", qfalse);
			ent->client->sess.sessionTeam = PickTeam( -1 );
			trap->GetUserinfo( clientNum, userinfo, MAX_INFO_STRING );

			if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
				ent->client->sess.sessionTeam = TEAM_RED;
			}

			if ( ent->client->sess.sessionTeam == TEAM_RED ) {
				team = "Red";
			}
			else {
				team = "Blue";
			}

			Info_SetValueForKey( userinfo, "team", team );

			trap->SetUserinfo( clientNum, userinfo );

			ent->client->ps.persistant[PERS_TEAM] = ent->client->sess.sessionTeam;

			preSess = ent->client->sess.sessionTeam;
			G_ReadClientSessionData( ent->client );
			ent->client->sess.sessionTeam = (team_t)preSess;
			G_WriteClientSessionData( ent->client );
			if ( !ClientUserinfoChanged( clientNum ) )
				return;
			ClientBegin( clientNum, qfalse );
			return;
		}
	}

	client = level.clients + clientNum;

	if ( ent->r.linked ) {
		trap->UnlinkEntity( (sharedEntity_t *)ent );
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	//assign the pointer for bg entity access
	ent->playerState = &ent->client->ps;

#if 0
	//fix for donedl command bug, that could cause powerup dissapearing
	if ( ent->health && ent->client && ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
		clientNum == ent->client->ps.clientNum && ent->ghoul2 )
	{// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		g_dontPenalizeTeam = qtrue;
		player_die( ent, ent, ent, 100000, MOD_TEAM_CHANGE );
		g_dontPenalizeTeam = qfalse;
	}
#endif

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
	spawnCount = client->ps.persistant[PERS_SPAWN_COUNT];

	for ( int i = 0; i < NUM_FORCE_POWERS; i++ ) {
		if ( ent->client->ps.fd.forcePowersActive & (1 << i) ) {
			WP_ForcePowerStop( ent, (forcePowers_t)i );
		}
	}

	for ( int i = TRACK_CHANNEL_1; i < NUM_TRACK_CHANNELS; i++ ) {
		if ( ent->client->ps.fd.killSoundEntIndex[i - 50]
			&& ent->client->ps.fd.killSoundEntIndex[i - 50] < MAX_GENTITIES
			&& ent->client->ps.fd.killSoundEntIndex[i - 50] > 0 )
		{
			G_MuteSound( ent->client->ps.fd.killSoundEntIndex[i - 50], CHAN_VOICE );
		}
	}

	memset( &client->ps, 0, sizeof(client->ps) );
	client->ps.eFlags = flags;
	client->ps.persistant[PERS_SPAWN_COUNT] = spawnCount;

	client->ps.hasDetPackPlanted = qfalse;

	//first-time force power initialization
	WP_InitForcePowers( ent );

	//init saber ent
	WP_SaberInitBladeData( ent );

	// First time model setup for that player.
	trap->GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
	Q_strncpyz( modelname, Info_ValueForKey( userinfo, "model" ), sizeof(modelname) );
	SetupGameGhoul2Model( ent, modelname, NULL );
	Info_SetValueForKey( userinfo, "model", modelname );

	if ( ent->ghoul2 && ent->client ) {
		ent->client->renderInfo.lastG2 = NULL; //update the renderinfo bolts next update.
	}

	if ( level.gametype == GT_POWERDUEL && client->sess.sessionTeam != TEAM_SPECTATOR
		&& client->sess.duelTeam == DUELTEAM_FREE )
	{
		SetTeam( ent, "s", qfalse );
	}
	else {
		if ( level.gametype == GT_SIEGE && (!gSiegeRoundBegun || gSiegeRoundEnded) ) {
			SetTeamQuick( ent, TEAM_SPECTATOR, qfalse );
		}

		// locate ent at a spawn point
		ClientSpawn( ent );

		//NT - reset the origin trails
		G_ResetTrail( ent );
		ent->client->saved.leveltime = 0;
	}

	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		// send event
		tent = G_TempEntity( &ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = ent->s.clientNum;

		if ( level.gametype != GT_DUEL || level.gametype == GT_POWERDUEL ) {
			trap->SendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " %s\n\"", client->pers.netname, G_GetStringEdString( "MP_SVGAME", "PLENTER" ) ) );
		}
	}

	AM_ApplySessionTransition( ent );

	G_LogPrintf( level.log.console, "ClientBegin: %i\n", clientNum );

	// count current clients and rank for scoreboard
	CalculateRanks();

	JPLua::Event_ClientBegin( clientNum );

	G_ClearClientLog( clientNum );
}

static qboolean AllForceDisabled( int force ) {
	if ( force ) {
		int i;
		for ( i = 0; i < NUM_FORCE_POWERS; i++ ) {
			if ( !(force & (1 << i)) )
				return qfalse;
		}

		return qtrue;
	}

	return qfalse;
}

// convenient interface to set all my limb breakage stuff up -rww
void G_BreakArm( gentity_t *ent, int arm ) {
	int anim = -1;

	assert( ent && ent->client );

	if ( ent->s.NPC_class == CLASS_VEHICLE || ent->localAnimIndex > 1 ) { //no broken limbs for vehicles and non-humanoids
		return;
	}

	if ( !arm ) { //repair him
		ent->client->ps.brokenLimbs = 0;
		return;
	}

	if ( ent->client->ps.fd.saberAnimLevel == SS_STAFF ) { //I'm too lazy to deal with this as well for now.
		return;
	}

	if ( arm == BROKENLIMB_LARM ) {
		if ( ent->client->saber[1].model[0] &&
			ent->client->ps.weapon == WP_SABER &&
			!ent->client->ps.saberHolstered &&
			ent->client->saber[1].soundOff ) { //the left arm shuts off its saber upon being broken
			G_Sound( ent, CHAN_AUTO, ent->client->saber[1].soundOff );
		}
	}

	ent->client->ps.brokenLimbs = 0; //make sure it's cleared out
	ent->client->ps.brokenLimbs |= (1 << arm); //this arm is now marked as broken

	//Do a pain anim based on the side. Since getting your arm broken does tend to hurt.
	if ( arm == BROKENLIMB_LARM ) {
		anim = BOTH_PAIN2;
	}
	else if ( arm == BROKENLIMB_RARM ) {
		anim = BOTH_PAIN3;
	}

	if ( anim == -1 ) {
		return;
	}

	G_SetAnim( ent, &ent->client->pers.cmd, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );

	//This could be combined into a single event. But I guess limbs don't break often enough to
	//worry about it.
	G_EntitySound( ent, CHAN_VOICE, G_SoundIndex( "*pain25.wav" ) );
	//FIXME: A nice bone snapping sound instead if possible
	G_Sound( ent, CHAN_AUTO, G_SoundIndex( va( "sound/player/bodyfall_human%i.wav", Q_irand( 1, 3 ) ) ) );
}

//Update the ghoul2 instance anims based on the playerstate values
qboolean BG_SaberStanceAnim( int anim );
qboolean PM_RunningAnim( int anim );
void G_UpdateClientAnims( gentity_t *self, float animSpeedScale ) {
	static int f;
	static int firstFrame, lastFrame;
	static int aFlags;
	static float animSpeed, lAnimSpeedScale;
	qboolean setTorso = qfalse;
	animNumber_t torsoAnim = (animNumber_t)self->client->ps.torsoAnim;
	animNumber_t legsAnim = (animNumber_t)self->client->ps.legsAnim;

	if ( self->client->ps.saberLockFrame ) {
		trap->G2API_SetBoneAnim( self->ghoul2, 0, "model_root", self->client->ps.saberLockFrame, self->client->ps.saberLockFrame + 1, BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150 );
		trap->G2API_SetBoneAnim( self->ghoul2, 0, "lower_lumbar", self->client->ps.saberLockFrame, self->client->ps.saberLockFrame + 1, BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150 );
		trap->G2API_SetBoneAnim( self->ghoul2, 0, "Motion", self->client->ps.saberLockFrame, self->client->ps.saberLockFrame + 1, BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150 );
		return;
	}

	if ( self->localAnimIndex > 1 &&
		bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame == 0 &&
		bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames == 0 ) { //We'll allow this for non-humanoids.
		goto tryTorso;
	}

	if ( self->client->legsAnimExecute != legsAnim || self->client->legsLastFlip != self->client->ps.legsFlip ) {
		animSpeed = 50.0f / bgAllAnims[self->localAnimIndex].anims[legsAnim].frameLerp;
		lAnimSpeedScale = (animSpeed *= animSpeedScale);

		if ( bgAllAnims[self->localAnimIndex].anims[legsAnim].loopFrames != -1 ) {
			aFlags = BONE_ANIM_OVERRIDE_LOOP;
		}
		else {
			aFlags = BONE_ANIM_OVERRIDE_FREEZE;
		}

		if ( animSpeed < 0 ) {
			lastFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame;
			firstFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame + bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames;
		}
		else {
			firstFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame;
			lastFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame + bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames;
		}

		aFlags |= BONE_ANIM_BLEND; //since client defaults to blend. Not sure if this will make much difference if any on server position, but it's here just for the sake of matching them.

		trap->G2API_SetBoneAnim( self->ghoul2, 0, "model_root", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150 );
		self->client->legsAnimExecute = legsAnim;
		self->client->legsLastFlip = self->client->ps.legsFlip;
	}

tryTorso:
	if ( self->localAnimIndex > 1 &&
		bgAllAnims[self->localAnimIndex].anims[torsoAnim].firstFrame == 0 &&
		bgAllAnims[self->localAnimIndex].anims[torsoAnim].numFrames == 0 )

	{ //If this fails as well just return.
		return;
	}
	else if ( self->s.number >= MAX_CLIENTS &&
		self->s.NPC_class == CLASS_VEHICLE ) { //we only want to set the root bone for vehicles
		return;
	}

	if ( (self->client->torsoAnimExecute != torsoAnim || self->client->torsoLastFlip != self->client->ps.torsoFlip) &&
		!self->noLumbar ) {
		aFlags = 0;
		animSpeed = 0;

		f = torsoAnim;

		BG_SaberStartTransAnim( self->s.number, self->client->ps.fd.saberAnimLevel, self->client->ps.weapon, f, &animSpeedScale, self->client->ps.brokenLimbs );

		animSpeed = 50.0f / bgAllAnims[self->localAnimIndex].anims[f].frameLerp;
		lAnimSpeedScale = (animSpeed *= animSpeedScale);

		if ( bgAllAnims[self->localAnimIndex].anims[f].loopFrames != -1 ) {
			aFlags = BONE_ANIM_OVERRIDE_LOOP;
		}
		else {
			aFlags = BONE_ANIM_OVERRIDE_FREEZE;
		}

		aFlags |= BONE_ANIM_BLEND; //since client defaults to blend. Not sure if this will make much difference if any on client position, but it's here just for the sake of matching them.

		if ( animSpeed < 0 ) {
			lastFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame;
			firstFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame + bgAllAnims[self->localAnimIndex].anims[f].numFrames;
		}
		else {
			firstFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame;
			lastFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame + bgAllAnims[self->localAnimIndex].anims[f].numFrames;
		}

		trap->G2API_SetBoneAnim( self->ghoul2, 0, "lower_lumbar", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, /*firstFrame why was it this before?*/-1, 150 );

		self->client->torsoAnimExecute = torsoAnim;
		self->client->torsoLastFlip = self->client->ps.torsoFlip;

		setTorso = qtrue;
	}

	if ( setTorso &&
		self->localAnimIndex <= 1 ) { //only set the motion bone for humanoids.
		trap->G2API_SetBoneAnim( self->ghoul2, 0, "Motion", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150 );
	}

#if 0 //disabled for now
	if (self->client->ps.brokenLimbs != self->client->brokenLimbs ||
		setTorso)
	{
		if (self->localAnimIndex <= 1 && self->client->ps.brokenLimbs &&
			(self->client->ps.brokenLimbs & (1 << BROKENLIMB_LARM)))
		{ //broken left arm
			char *brokenBone = "lhumerus";
			animation_t *armAnim;
			int armFirstFrame;
			int armLastFrame;
			int armFlags = 0;
			float armAnimSpeed;

			armAnim = &bgAllAnims[self->localAnimIndex].anims[ BOTH_DEAD21 ];
			self->client->brokenLimbs = self->client->ps.brokenLimbs;

			armFirstFrame = armAnim->firstFrame;
			armLastFrame = armAnim->firstFrame+armAnim->numFrames;
			armAnimSpeed = 50.0f / armAnim->frameLerp;
			armFlags = (BONE_ANIM_OVERRIDE_LOOP|BONE_ANIM_BLEND);

			trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
		}
		else if (self->localAnimIndex <= 1 && self->client->ps.brokenLimbs &&
			(self->client->ps.brokenLimbs & (1 << BROKENLIMB_RARM)))
		{ //broken right arm
			char *brokenBone = "rhumerus";
			char *supportBone = "lhumerus";

			self->client->brokenLimbs = self->client->ps.brokenLimbs;

			//Only put the arm in a broken pose if the anim is such that we
			//want to allow it.
			if ((//self->client->ps.weapon == WP_MELEE ||
				self->client->ps.weapon != WP_SABER ||
				BG_SaberStanceAnim(self->client->ps.torsoAnim) ||
				PM_RunningAnim(self->client->ps.torsoAnim)) &&
				(!self->client->saber[1].model[0] || self->client->ps.weapon != WP_SABER))
			{
				int armFirstFrame;
				int armLastFrame;
				int armFlags = 0;
				float armAnimSpeed;
				animation_t *armAnim;

				if (self->client->ps.weapon == WP_MELEE ||
					self->client->ps.weapon == WP_SABER ||
					self->client->ps.weapon == WP_BRYAR_PISTOL)
				{ //don't affect this arm if holding a gun, just make the other arm support it
					armAnim = &bgAllAnims[self->localAnimIndex].anims[ BOTH_ATTACK2 ];

					//armFirstFrame = armAnim->firstFrame;
					armFirstFrame = armAnim->firstFrame+armAnim->numFrames;
					armLastFrame = armAnim->firstFrame+armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = (BONE_ANIM_OVERRIDE_LOOP|BONE_ANIM_BLEND);

					trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
				}
				else
				{ //we want to keep the broken bone updated for some cases
					trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				}

				if (self->client->ps.torsoAnim != BOTH_MELEE1 &&
					self->client->ps.torsoAnim != BOTH_MELEE2 &&
					(self->client->ps.torsoAnim == TORSO_WEAPONREADY2 || self->client->ps.torsoAnim == BOTH_ATTACK2 || self->client->ps.weapon < WP_BRYAR_PISTOL))
				{
					//Now set the left arm to "support" the right one
					armAnim = &bgAllAnims[self->localAnimIndex].anims[ BOTH_STAND2 ];
					armFirstFrame = armAnim->firstFrame;
					armLastFrame = armAnim->firstFrame+armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = (BONE_ANIM_OVERRIDE_LOOP|BONE_ANIM_BLEND);

					trap->G2API_SetBoneAnim(self->ghoul2, 0, supportBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
				}
				else
				{ //we want to keep the support bone updated for some cases
					trap->G2API_SetBoneAnim(self->ghoul2, 0, supportBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				}
			}
			else
			{ //otherwise, keep it set to the same as the torso
				trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				trap->G2API_SetBoneAnim(self->ghoul2, 0, supportBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
			}
		}
		else if (self->client->brokenLimbs)
		{ //remove the bone now so it can be set again
			char *brokenBone = NULL;
			int broken = 0;

			//Warning: Don't remove bones that you've added as bolts unless you want to invalidate your bolt index
			//(well, in theory, I haven't actually run into the problem)
			if (self->client->brokenLimbs & (1<<BROKENLIMB_LARM))
			{
				brokenBone = "lhumerus";
				broken |= (1<<BROKENLIMB_LARM);
			}
			else if (self->client->brokenLimbs & (1<<BROKENLIMB_RARM))
			{ //can only have one arm broken at once.
				brokenBone = "rhumerus";
				broken |= (1<<BROKENLIMB_RARM);

				//want to remove the support bone too then
				trap->G2API_SetBoneAnim(self->ghoul2, 0, "lhumerus", 0, 1, 0, 0, level.time, -1, 0);
				trap->G2API_RemoveBone(self->ghoul2, "lhumerus", 0);
			}

			assert(brokenBone);

			//Set the flags and stuff to 0, so that the remove will succeed
			trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, 0, 1, 0, 0, level.time, -1, 0);

			//Now remove it
			trap->G2API_RemoveBone(self->ghoul2, brokenBone, 0);
			self->client->brokenLimbs &= ~broken;
		}
	}
#endif
}

qboolean WP_HasForcePowers( const playerState_t *ps );

// Called every time a client is placed fresh in the world: after the first ClientBegin, and after each respawn
// Initializes all non-persistant parts of playerState
void ClientSpawn( gentity_t *ent ) {
	int i, index = 0, saveSaberNum = ENTITYNUM_NONE, savedSiegeIndex = 0, maxHealth = 100, gameFlags, savedPing, accuracy_hits,
		accuracy_shots, eventSequence, persistant[MAX_PERSISTANT];
	vector3 spawn_origin, spawn_angles;
	gentity_t *spawnPoint = NULL;
	gclient_t *client = NULL;
	clientPersistant_t saved;
	clientSession_t savedSess;
	forcedata_t savedForce;
	saberInfo_t saberSaved[MAX_SABERS];
	uint32_t flags, wDisable = 0;
	void *g2WeaponPtrs[MAX_SABERS];
	char userinfo[MAX_INFO_STRING] = { 0 };
	const char *key = NULL, *value = NULL, *saber = NULL;
	qboolean changedSaber = qfalse, inSiegeWithClass = qfalse;

	index = ent - g_entities;
	client = ent->client;

	//first we want the userinfo so we can see if we should update this client's saber -rww
	trap->GetUserinfo( index, userinfo, sizeof(userinfo) );

	for ( i = 0; i < MAX_SABERS; i++ ) {
		saber = (i & 1) ? ent->client->pers.saber2 : ent->client->pers.saber1;
		value = Info_ValueForKey( userinfo, va( "saber%i", i + 1 ) );
		if ( saber && value &&
			(Q_stricmp( value, saber ) || !saber[0] || !ent->client->saber[0].model[0]) ) { //doesn't match up (or our saber is BS), we want to try setting it
			if ( G_SetSaber( ent, i, value, qfalse ) )
				changedSaber = qtrue;

			//Well, we still want to say they changed then (it means this is siege and we have some overrides)
			else if ( !saber[0] || !ent->client->saber[0].model[0] )
				changedSaber = qtrue;
		}
	}

	if ( changedSaber ) { //make sure our new info is sent out to all the other clients, and give us a valid stance
		if ( !ClientUserinfoChanged( ent->s.number ) )
			return;

		//make sure the saber models are updated
		G_SaberModelSetup( ent );

		for ( i = 0; i < MAX_SABERS; i++ ) {
			saber = (i & 1) ? ent->client->pers.saber2 : ent->client->pers.saber1;
			key = va( "saber%d", i + 1 );
			value = Info_ValueForKey( userinfo, key );
			if ( Q_stricmp( value, saber ) ) {// they don't match up, force the user info
				Info_SetValueForKey( userinfo, key, saber );
				trap->SetUserinfo( ent->s.number, userinfo );
			}
		}

		if ( ent->client->saber[0].model[0] && ent->client->saber[1].model[0] ) { //dual
			ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = SS_DUAL;
		}
		else if ( (ent->client->saber[0].saberFlags&SFL_TWO_HANDED) ) { //staff
			ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = SS_STAFF;
		}
		else {
			ent->client->sess.saberLevel = Q_clampi( SS_FAST, ent->client->sess.saberLevel, SS_STRONG );
			ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel;

			// limit our saber style to our force points allocated to saber offense
			if ( level.gametype != GT_SIEGE && ent->client->ps.fd.saberAnimLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
				ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel = ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
		}
		if ( level.gametype != GT_SIEGE ) {// let's just make sure the styles we chose are cool
			if ( !WP_SaberStyleValidForSaber( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, ent->client->ps.fd.saberAnimLevel ) ) {
				WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &ent->client->ps.fd.saberAnimLevel );
				ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
			}
		}
	}

	if ( client->ps.fd.forceDoInit && !ent->client->pers.adminData.merc ) {
		// force a reread of force powers
		WP_InitForcePowers( ent );
		client->ps.fd.forceDoInit = 0;
	}

	if ( ent->client->ps.fd.saberAnimLevel != SS_STAFF && ent->client->ps.fd.saberAnimLevel != SS_DUAL
		&& ent->client->ps.fd.saberAnimLevel == ent->client->ps.fd.saberDrawAnimLevel
		&& ent->client->ps.fd.saberAnimLevel == ent->client->sess.saberLevel )
	{
		ent->client->sess.saberLevel = Q_clampi( SS_FAST, ent->client->sess.saberLevel, SS_STRONG );
		ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel;

		// limit our saber style to our force points allocated to saber offense
		if ( level.gametype != GT_SIEGE && ent->client->ps.fd.saberAnimLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
			ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel = ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
	}

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		spawnPoint = SelectSpectatorSpawnPoint( &spawn_origin, &spawn_angles );
	}
	else if ( level.gametype == GT_CTF || level.gametype == GT_CTY ) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint( client->sess.sessionTeam, client->pers.teamState.state, &spawn_origin,
			&spawn_angles );
	}
	else if ( level.gametype == GT_SIEGE ) {
		spawnPoint = SelectSiegeSpawnPoint( client->siegeClass, client->sess.sessionTeam, client->pers.teamState.state,
			&spawn_origin, &spawn_angles );
	}
	else {
		do {
			if ( level.gametype == GT_POWERDUEL ) {
				spawnPoint = SelectDuelSpawnPoint( client->sess.duelTeam, &client->ps.origin, &spawn_origin,
					&spawn_angles );
			}
			else if ( level.gametype == GT_DUEL ) {	// duel
				spawnPoint = SelectDuelSpawnPoint( DUELTEAM_SINGLE, &client->ps.origin, &spawn_origin, &spawn_angles );
			}
			else {
				// the first spawn should be at a good looking spot
				if ( !client->pers.initialSpawn && client->pers.localClient ) {
					client->pers.initialSpawn = qtrue;
					spawnPoint = SelectInitialSpawnPoint( &spawn_origin, &spawn_angles, client->sess.sessionTeam );
				}
				else {
					// don't spawn near existing origin if possible
					spawnPoint = SelectSpawnPoint( &client->ps.origin, &spawn_origin, &spawn_angles,
						client->sess.sessionTeam );
				}
			}

			if ( (spawnPoint->flags & FL_NO_BOTS) && (ent->r.svFlags & SVF_BOT) ) {
				continue;	// try again
			}
			// just to be symmetric, we have a nohumans option...
			if ( (spawnPoint->flags & FL_NO_HUMANS) && !(ent->r.svFlags & SVF_BOT) ) {
				continue;	// try again
			}

			break;
		} while ( 1 );
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT);
	flags ^= EF_TELEPORT_BIT;
	gameFlags = ent->client->mGameFlags & (PSG_VOTED);

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	//	savedAreaBits = client->areabits;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;
	for ( i = 0; i < MAX_PERSISTANT; i++ )
		persistant[i] = client->ps.persistant[i];

	eventSequence = client->ps.eventSequence;

	savedForce = client->ps.fd;

	saveSaberNum = client->ps.saberEntityNum;

	savedSiegeIndex = client->siegeClass;

	for ( i = 0; i < MAX_SABERS; i++ ) {
		saberSaved[i] = client->saber[i];
		g2WeaponPtrs[i] = client->weaponGhoul2[i];
	}

	for ( i = 0; i < HL_MAX; i++ )
		ent->locationDamage[i] = 0;

	memset( client, 0, sizeof(*client) );
	client->bodyGrabIndex = ENTITYNUM_NONE;

	//Get the skin RGB based on his userinfo
	client->ps.customRGBA[0] = (value = Info_ValueForKey( userinfo, "char_color_red" )) ? Q_clampi( 0, atoi( value ), 255 ) : 255;
	client->ps.customRGBA[1] = (value = Info_ValueForKey( userinfo, "char_color_green" )) ? Q_clampi( 0, atoi( value ), 255 ) : 255;
	client->ps.customRGBA[2] = (value = Info_ValueForKey( userinfo, "char_color_blue" )) ? Q_clampi( 0, atoi( value ), 255 ) : 255;

	//Prevent skins being too dark
	if ( japp_charRestrictRGB.integer
		&& ((client->ps.customRGBA[0] + client->ps.customRGBA[1] + client->ps.customRGBA[2]) < 100) )
	{
		client->ps.customRGBA[0] = client->ps.customRGBA[1] = client->ps.customRGBA[2] = 255;
	}

	client->ps.customRGBA[3] = 255;

	// update our customRGBA for team colors.
	if ( level.gametype >= GT_TEAM && level.gametype != GT_SIEGE && !g_jediVmerc.integer ) {
		char skin[MAX_QPATH] = { 0 }, model[MAX_QPATH] = { 0 };
		vector3 colorOverride = { 0.0f };

		VectorClear( &colorOverride );
		Q_strncpyz( model, Info_ValueForKey( userinfo, "model" ), sizeof(model) );

		BG_ValidateSkinForTeam( model, skin, savedSess.sessionTeam, &colorOverride );
		if ( colorOverride.r != 0.0f || colorOverride.g != 0.0f || colorOverride.b != 0.0f ) {
			client->ps.customRGBA[0] = colorOverride.r * 255.0f;
			client->ps.customRGBA[1] = colorOverride.g * 255.0f;
			client->ps.customRGBA[2] = colorOverride.b * 255.0f;
		}
	}

	client->siegeClass = savedSiegeIndex;

	for ( i = 0; i < MAX_SABERS; i++ ) {
		client->saber[i] = saberSaved[i];
		client->weaponGhoul2[i] = g2WeaponPtrs[i];
	}

	//or the saber ent num
	client->ps.saberEntityNum = saveSaberNum;
	client->saberStoredIndex = saveSaberNum;

	client->ps.fd = savedForce;

	client->ps.duelIndex = ENTITYNUM_NONE;

	//spawn with 100
	client->ps.jetpackFuel = 100;
	client->ps.cloakFuel = 100;

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
	//	client->areabits = savedAreaBits;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;

	for ( i = 0; i < MAX_PERSISTANT; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}

	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	// set max health
	if ( level.gametype == GT_SIEGE && client->siegeClass != -1 ) {
		siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];
		maxHealth = 100;

		if ( scl->maxhealth ) {
			maxHealth = scl->maxhealth;
		}
	}
	else {
		maxHealth = Q_clampi( 1, atoi( Info_ValueForKey( userinfo, "handicap" ) ), 100 );
	}
	client->pers.maxHealth = maxHealth;//atoi( Info_ValueForKey( userinfo, "handicap" ) );
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > maxHealth ) {
		client->pers.maxHealth = 100;
	}
	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;

	//Raz: Save empowered state
	if ( client->pers.adminData.empowered ) {
		client->ps.eFlags |= EF_BODYPUSH;
	}

	client->mGameFlags = gameFlags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->playerState = &ent->client->ps;
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	VectorCopy( &playerMins, &ent->r.mins );
	VectorCopy( &playerMaxs, &ent->r.maxs );
	client->ps.crouchheight = CROUCH_MAXS_2;
	client->ps.standheight = DEFAULT_MAXS_2;

	client->ps.clientNum = index;
	//give default weapons
	client->ps.stats[STAT_WEAPONS] = (1 << WP_NONE);

	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		wDisable = g_duelWeaponDisable.integer;
	}
	else {
		wDisable = g_weaponDisable.integer;
	}

	if ( level.gametype != GT_HOLOCRON
		&& level.gametype != GT_JEDIMASTER
		&& !BG_HasSetSaberOnly()
		&& !AllForceDisabled( g_forcePowerDisable.integer )
		&& g_jediVmerc.integer )
	{
		if ( level.gametype >= GT_TEAM && (client->sess.sessionTeam == TEAM_BLUE || client->sess.sessionTeam == TEAM_RED) ) {//In Team games, force one side to be merc and other to be jedi
			if ( level.numPlayingClients > 0 ) {//already someone in the game
				int		i, forceTeam = TEAM_SPECTATOR;
				for ( i = 0; i < level.maxclients; i++ ) {
					if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
						continue;
					}
					if ( level.clients[i].sess.sessionTeam == TEAM_BLUE || level.clients[i].sess.sessionTeam == TEAM_RED ) {//in-game
						if ( WP_HasForcePowers( &level.clients[i].ps ) ) {//this side is using force
							forceTeam = level.clients[i].sess.sessionTeam;
						}
						else {//other team is using force
							if ( level.clients[i].sess.sessionTeam == TEAM_BLUE ) {
								forceTeam = TEAM_RED;
							}
							else {
								forceTeam = TEAM_BLUE;
							}
						}
						break;
					}
				}
				if ( WP_HasForcePowers( &client->ps ) && client->sess.sessionTeam != forceTeam ) {//using force but not on right team, switch him over
					const char *teamName = TeamName( forceTeam );
					//client->sess.sessionTeam = forceTeam;
					SetTeam( ent, teamName, qfalse );
					return;
				}
			}
		}

		if ( WP_HasForcePowers( &client->ps ) ) {
			client->ps.trueNonJedi = qfalse;
			client->ps.trueJedi = qtrue;
			//make sure they only use the saber
			client->ps.weapon = WP_SABER;
			client->ps.stats[STAT_WEAPONS] = (1 << WP_SABER);
		}
		else {//no force powers set
			client->ps.trueNonJedi = qtrue;
			client->ps.trueJedi = qfalse;
			if ( !wDisable || !(wDisable & (1 << WP_BRYAR_PISTOL)) ) {
				client->ps.stats[STAT_WEAPONS] |= (1 << WP_BRYAR_PISTOL);
			}
			if ( !wDisable || !(wDisable & (1 << WP_BLASTER)) ) {
				client->ps.stats[STAT_WEAPONS] |= (1 << WP_BLASTER);
			}
			if ( !wDisable || !(wDisable & (1 << WP_BOWCASTER)) ) {
				client->ps.stats[STAT_WEAPONS] |= (1 << WP_BOWCASTER);
			}
			client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER);
			client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
			client->ps.ammo[AMMO_POWERCELL] = ammoMax[AMMO_POWERCELL];
			client->ps.weapon = WP_BRYAR_PISTOL;
		}
	}

	if ( level.gametype != GT_SIEGE ) {
		if ( ent->client->pers.adminData.merc ) {
			Merc_On( ent );
		}
		else {
			client->ps.stats[STAT_WEAPONS] = japp_spawnWeaps.integer;

			// give ammo for all available weapons
			for ( i = WP_BRYAR_PISTOL; i <= LAST_USEABLE_WEAPON; i++ ) {
				if ( japp_spawnWeaps.bits & (1 << i) ) {
					ammo_t ammo = weaponData[i].ammoIndex;
					const gitem_t *it = BG_FindItemForAmmo( ammo );
					if ( it )
						client->ps.ammo[ammo] += it->quantity;
				}
			}

			int newWeap = -1;
			for ( i = WP_SABER; i < WP_NUM_WEAPONS; i++ ) {
				if ( (client->ps.stats[STAT_WEAPONS] & (1 << i)) ) {
					newWeap = i;
					break;
				}
			}

			if ( newWeap == WP_NUM_WEAPONS ) {
				for ( i = WP_STUN_BATON; i < WP_SABER; i++ ) {
					if ( (client->ps.stats[STAT_WEAPONS] & (1 << i)) ) {
						newWeap = i;
						break;
					}
				}
				if ( newWeap == WP_SABER )
					newWeap = WP_NONE;
			}

			weapon_t wp = (weapon_t)ent->client->ps.weapon;
			if ( newWeap != -1 )	ent->client->ps.weapon = newWeap;
			else					ent->client->ps.weapon = 0;

			if ( ent->client->ps.weapon != WP_SABER ) {
				G_AddEvent( ent, EV_NOAMMO, wp );
			}
		}
	}

	else if ( client->siegeClass != -1 && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		// well then, we will use a custom weaponset for our class
		client->ps.stats[STAT_WEAPONS] = bgSiegeClasses[client->siegeClass].weapons;

		if ( client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER) )
			client->ps.weapon = WP_SABER;
		else if ( client->ps.stats[STAT_WEAPONS] & (1 << WP_BRYAR_PISTOL) )
			client->ps.weapon = WP_BRYAR_PISTOL;
		else
			client->ps.weapon = WP_MELEE;
		inSiegeWithClass = qtrue;

		for ( int m = 0; m < WP_NUM_WEAPONS; m++ ) {
			if ( client->ps.stats[STAT_WEAPONS] & (1 << m) ) {
				if ( client->ps.weapon != WP_SABER ) {
					// try to find the highest ranking weapon we have
					if ( m > client->ps.weapon ) {
						client->ps.weapon = m;
					}
				}

				if ( m >= WP_BRYAR_PISTOL ) {
					// Max his ammo out for all the weapons he has.
					if ( m == WP_ROCKET_LAUNCHER ) {
						// don't give full ammo!
						//FIXME: extern this and check it when getting ammo from supplier, pickups or ammo stations!
						if ( bgSiegeClasses[client->siegeClass].classflags & (1 << CFL_SINGLE_ROCKET) ) {
							client->ps.ammo[weaponData[m].ammoIndex] = 1;
						}
						else {
							client->ps.ammo[weaponData[m].ammoIndex] = 10;
						}
					}
					else {
						if ( bgSiegeClasses[client->siegeClass].classflags & (1 << CFL_EXTRA_AMMO) ) {
							// double ammo
							client->ps.ammo[weaponData[m].ammoIndex] = ammoMax[weaponData[m].ammoIndex] * 2;
							client->ps.eFlags |= EF_DOUBLE_AMMO;
						}
						else {
							client->ps.ammo[weaponData[m].ammoIndex] = ammoMax[weaponData[m].ammoIndex];
						}
					}
				}
			}
		}

		// use class-specified inventory
		client->ps.stats[STAT_HOLDABLE_ITEMS] = bgSiegeClasses[client->siegeClass].invenItems;
		client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	}

	else {
		if ( client->pers.adminData.merc ) {
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] = ((1 << HI_NUM_HOLDABLE) - 1) & ~1;
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] = HI_NONE + 1;
		}
		else {
			client->ps.stats[STAT_HOLDABLE_ITEMS] = japp_spawnItems.integer;
			uint32_t x = client->ps.stats[STAT_HOLDABLE_ITEMS];
			// get the right-most bit
			x &= -x;
			// log2n of x is array index of bit-value
			x = (x >= 1000000000)
				? 9 : (x >= 100000000)
				? 8 : (x >= 10000000)
				? 7 : (x >= 1000000)
				? 6 : (x >= 100000)
				? 5 : (x >= 10000)
				? 4 : (x >= 1000)
				? 3 : (x >= 100)
				? 2 : (x >= 10)
				? 1 : 0;
			client->ps.stats[STAT_HOLDABLE_ITEM] = x;
		}
	}

	if ( level.gametype == GT_SIEGE &&
		client->siegeClass != -1 &&
		bgSiegeClasses[client->siegeClass].powerups &&
		client->sess.sessionTeam != TEAM_SPECTATOR ) { //this class has some start powerups
		i = 0;
		while ( i < PW_NUM_POWERUPS ) {
			if ( bgSiegeClasses[client->siegeClass].powerups & (1 << i) ) {
				client->ps.powerups[i] = Q3_INFINITE;
			}
			i++;
		}
	}

	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		client->ps.stats[STAT_WEAPONS] = 0;
		client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
		client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	}

	// nmckenzie: DESERT_SIEGE... or well, siege generally.  This was over-writing the max value, which was NOT good for siege.
	if ( inSiegeWithClass == qfalse ) {
		client->ps.ammo[AMMO_BLASTER] = 100; //ammoMax[AMMO_BLASTER]; //100 seems fair.
	}

	client->ps.rocketLockIndex = ENTITYNUM_NONE;
	client->ps.rocketLockTime = 0;

	// rww - set here to initialize the circling seeker drone to off.
	// a quick note about this so I don't forget how it works again:
	// ps.genericEnemyIndex is kept in sync between the server and client.
	// when it gets set then an entitystate value of the same name gets set along with an entitystate flag in the shared
	//	bg code, which is why a value needs to be both on the player state and entity state.
	// it doesn't seem to just carry over the entitystate value automatically because entity state value is derived
	//	from player state data or some such)
	client->ps.genericEnemyIndex = -1;

	client->ps.isJediMaster = qfalse;

	if ( client->ps.fallingToDeath ) {
		client->ps.fallingToDeath = 0;
		client->noCorpse = qtrue;
	}

	//Do per-spawn force power initialization
	WP_SpawnInitForcePowers( ent );

	// health will count down towards max_health
	if ( level.gametype == GT_SIEGE &&
		client->siegeClass != -1 &&
		bgSiegeClasses[client->siegeClass].starthealth ) { //class specifies a start health, so use it
		ent->health = client->ps.stats[STAT_HEALTH] = bgSiegeClasses[client->siegeClass].starthealth;
	}
	else if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {//only start with 100 health in Duel
		if ( level.gametype == GT_POWERDUEL && client->sess.duelTeam == DUELTEAM_LONE ) {
			if ( duel_fraglimit.integer ) {

				ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] =
					g_powerDuelStartHealth.integer - ((g_powerDuelStartHealth.integer - g_powerDuelEndHealth.integer) * (float)client->sess.wins / (float)duel_fraglimit.integer);
			}
			else {
				ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] = 150;
			}
		}
		else {
			ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] = 100;
		}
	}
	else if ( client->ps.stats[STAT_MAX_HEALTH] <= 100 ) {
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] * 1.25f;
	}
	else if ( client->ps.stats[STAT_MAX_HEALTH] < 125 ) {
		ent->health = client->ps.stats[STAT_HEALTH] = 125;
	}
	else {
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];
	}

	// Start with a small amount of armor as well.
	if ( level.gametype == GT_SIEGE &&
		client->siegeClass != -1 /*&&
		bgSiegeClasses[client->siegeClass].startarmor*/ ) { //class specifies a start armor amount, so use it
		client->ps.stats[STAT_ARMOR] = bgSiegeClasses[client->siegeClass].startarmor;
	}
	else if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {//no armor in duel
		client->ps.stats[STAT_ARMOR] = 0;
	}
	else {
		client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_HEALTH] * 0.25f;
	}

	G_SetOrigin( ent, &spawn_origin );
	VectorCopy( &spawn_origin, &client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap->GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, &spawn_angles );

	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		const uint32_t animFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS;

		G_KillBox( ent );
		trap->LinkEntity( (sharedEntity_t *)ent );

		// force the base weapon up
		//client->ps.weapon = WP_BRYAR_PISTOL;
		//client->ps.weaponstate = FIRST_WEAPON;
		if ( client->ps.weapon <= WP_NONE ) {
			client->ps.weapon = WP_BRYAR_PISTOL;
		}

		client->ps.torsoTimer = client->ps.legsTimer = 0;

		if ( client->ps.weapon == WP_SABER ) {
			if ( japp_spawnActivateSaber.integer ) {
				G_SetAnim( ent, NULL, SETANIM_BOTH, BOTH_STAND1TO2, animFlags, 0 );
			}
			else {
				G_SetAnim( ent, NULL, SETANIM_BOTH, BOTH_STAND1, animFlags, 0 );
				ent->client->ps.saberHolstered = 2;
			}
		}
		else {
			G_SetAnim( ent, NULL, SETANIM_TORSO, TORSO_RAISEWEAP1, animFlags, 0 );
			client->ps.legsAnim = WeaponReadyAnim[client->ps.weapon];
		}
		client->ps.weaponstate = WEAPON_RAISING;
		client->ps.weaponTime = client->ps.torsoTimer;
	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	if ( client->pers.adminData.isSlept ) {
		client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
		client->ps.forceHandExtendTime = INT32_MAX;
		client->ps.forceDodgeAnim = 0;
	}

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	}
	else {
		// fire the targets of the spawn point
		G_UseTargets( spawnPoint, ent );
	}

	//set teams for NPCs to recognize
	if ( level.gametype == GT_SIEGE ) { //Imperial (team1) team is allied with "enemy" NPCs in this mode
		if ( client->sess.sessionTeam == SIEGETEAM_TEAM1 ) {
			ent->s.teamowner = NPCTEAM_ENEMY;
			client->playerTeam = (npcteam_t)ent->s.teamowner;
			client->enemyTeam = NPCTEAM_PLAYER;
		}
		else {
			ent->s.teamowner = NPCTEAM_PLAYER;
			client->playerTeam = (npcteam_t)ent->s.teamowner;
			client->enemyTeam = NPCTEAM_ENEMY;
		}
	}
	else {
		ent->s.teamowner = NPCTEAM_PLAYER;
		client->playerTeam = (npcteam_t)ent->s.teamowner;
		client->enemyTeam = NPCTEAM_ENEMY;
	}

	/*
	//scaling for the power duel opponent
	if (level.gametype == GT_POWERDUEL &&
	client->sess.duelTeam == DUELTEAM_LONE)
	{
	client->ps.iModelScale = 125;
	VectorSet(ent->modelScale, 1.25f, 1.25f, 1.25f);
	}
	*/
	//Disabled. At least for now. Not sure if I'll want to do it or not eventually.

	JPLua::Event_ClientSpawn( ent - g_entities, (client->ps.persistant[PERS_SPAWN_COUNT] == 1) ? qtrue : qfalse );

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent - g_entities, NULL );

	// positively link the client, even if the command times are weird
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
		VectorCopy( &ent->client->ps.origin, &ent->r.currentOrigin );
		trap->LinkEntity( (sharedEntity_t *)ent );
	}

	if ( g_spawnInvulnerability.integer ) {
		ent->client->ps.eFlags |= EF_INVULNERABLE;
		ent->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
	}

	// run the presend to set anything else
	if ( ent->client->sess.spectatorState != SPECTATOR_FOLLOW )
		ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

	//rww - make sure client has a valid icarus instance
	trap->ICARUS_FreeEnt( (sharedEntity_t *)ent );
	trap->ICARUS_InitEnt( (sharedEntity_t *)ent );
}

void G_ClearVote( gentity_t *ent ) {
	if ( !level.voteTime )
		return;

	if ( ent->client->mGameFlags & PSG_VOTED ) {
		if ( ent->client->pers.vote == 1 ) {
			level.voteYes--;
			trap->SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
		}
		else if ( ent->client->pers.vote == 2 ) {
			level.voteNo--;
			trap->SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
		}
	}
	ent->client->mGameFlags &= ~(PSG_VOTED);
	ent->client->pers.vote = 0;
}

// Called when a player drops from the server.
// Will not be called between levels.
// This should NOT be called directly by any game logic. Call trap->DropClient() which will call this and do server
//	system housekeeping.
void ClientDisconnect( int clientNum ) {
	gentity_t *ent, *tent, *other;
	int i;

	// cleanup if we are kicking a bot that hasn't spawned yet
	G_RemoveQueuedBotBegin( clientNum );

	ent = g_entities + clientNum;
	if ( !ent->client || ent->client->pers.connected == CON_DISCONNECTED ) {
		G_LogPrintf( level.log.security, "ClientDisconnect: tried to disconnect an inactive client %i\n", clientNum );
		return;
	}

	G_LogPrintf( level.log.console, "ClientDisconnect: %i\n", clientNum );

	JPLua::Event_ClientDisconnect( clientNum );

	for ( i = 0; i < NUM_FORCE_POWERS; i++ ) {
		if ( ent->client->ps.fd.forcePowersActive & (1 << i) )
			WP_ForcePowerStop( ent, (forcePowers_t)i );
	}

	for ( i = TRACK_CHANNEL_1; i < NUM_TRACK_CHANNELS; i++ ) {
		if ( ent->client->ps.fd.killSoundEntIndex[i - 50] && ent->client->ps.fd.killSoundEntIndex[i - 50] < MAX_GENTITIES && ent->client->ps.fd.killSoundEntIndex[i - 50] > 0 )
			G_MuteSound( ent->client->ps.fd.killSoundEntIndex[i - 50], CHAN_VOICE );
	}

	G_LeaveVehicle( ent, qtrue );

	// make sure this client slot is unignored
	for ( i = 0, other = g_entities; i < MAX_CLIENTS; i++, other++ ) {
		if ( other->inuse && other->client ) {
			other->client->pers.ignore &= ~(1 << clientNum);
		}
	}
	ent->client->pers.ignore = 0u;

	// unmindtrick this client slot
	Q_RemoveFromBitflags( level.admin.mindtricked, clientNum, 16 );

	// stop any following clients
	for ( i = 0; i < level.maxclients; i++ ) {
		if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR
			&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.clients[i].sess.spectatorClient == clientNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED && ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		tent = G_TempEntity( &ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = ent->s.clientNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems( ent );
	}

	// kill grapple
	if ( ent->client->hook ) {
		Weapon_HookFree( ent->client->hook );
	}

	// if we are playing in tourney mode, give a win to the other player and clear his frags for this round
	if ( level.gametype == GT_DUEL && !level.intermissiontime && !level.warmupTime ) {
		if ( level.sortedClients[1] == clientNum ) {
			level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] = 0;
			level.clients[level.sortedClients[0]].sess.wins++;
			ClientUserinfoChanged( level.sortedClients[0] );
		}
		else if ( level.sortedClients[0] == clientNum ) {
			level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] = 0;
			level.clients[level.sortedClients[1]].sess.wins++;
			ClientUserinfoChanged( level.sortedClients[1] );
		}
	}

	if ( level.gametype == GT_DUEL && ent->client->sess.sessionTeam == TEAM_FREE && level.intermissiontime ) {
		trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
		level.restarted = qtrue;
		level.intermissiontime = 0;
	}

	if ( ent->ghoul2 && trap->G2API_HaveWeGhoul2Models( ent->ghoul2 ) )
		trap->G2API_CleanGhoul2Models( &ent->ghoul2 );

	for ( i = 0; i < MAX_SABERS; i++ ) {
		if ( ent->client->weaponGhoul2[i] && trap->G2API_HaveWeGhoul2Models( ent->client->weaponGhoul2[i] ) )
			trap->G2API_CleanGhoul2Models( &ent->client->weaponGhoul2[i] );
	}

	trap->UnlinkEntity( (sharedEntity_t *)ent );
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;
	ent->r.contents = 0;

	G_ClearVote( ent );
	ent->userinfoChanged = 0;
	ent->userinfoSpam = 0;

	//Raz: Siege item/ATST/reconnect fix http://www.lucasforums.com/showpost.php?p=2143165&postcount=92
	if ( ent->client->holdingObjectiveItem > 0 ) {
		// carrying a siege objective item - make sure it updates and removes itself from us now in case this is an instant death-respawn situation
		gentity_t *objectiveItem = &g_entities[ent->client->holdingObjectiveItem];

		if ( objectiveItem->inuse ) {
			if ( objectiveItem->think ) {
				objectiveItem->think( objectiveItem );
			}

			JPLua::Entity_CallFunction( objectiveItem, JPLua::JPLUA_ENTITY_THINK );
		}
	}

	trap->SetConfigstring( CS_PLAYERS + clientNum, "" );

	CalculateRanks();

	if ( ent->r.svFlags & SVF_BOT )
		BotAIShutdownClient( clientNum, qfalse );

	G_ClearClientLog( clientNum );
}
