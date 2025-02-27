//NPC_behavior.cpp
/*
FIXME - MCG:
These all need to make use of the snapshots.  Write something that can look for only specific
things in a snapshot or just go through the snapshot every frame and save the info in case
we need it...
*/

#include "b_local.h"
#include "g_nav.h"
#include "ICARUS/Q3_Interface.h"

extern	qboolean	showBBoxes;
extern vector3 NPCDEBUG_BLUE;
//void G_Cube( vector3 *mins, vector3 *maxs, vector3 *color, float alpha );
void NPC_CheckGetNewWeapon( void );

qboolean PM_InKnockDown( playerState_t *ps );

void NPC_AimAdjust( int change );
qboolean NPC_SomeoneLookingAtMe( gentity_t *ent );
/*
 void NPC_BSAdvanceFight (void)

 Advance towards your captureGoal and shoot anyone you can along the way.
 */
void NPC_BSAdvanceFight( void ) {//FIXME: IMPLEMENT
	//Head to Goal if I can

	//Make sure we're still headed where we want to capture
	if ( NPCInfo->captureGoal ) {//FIXME: if no captureGoal, what do we do?
		//VectorCopy( NPCInfo->captureGoal->r.currentOrigin, NPCInfo->tempGoal->r.currentOrigin );
		//NPCInfo->goalEntity = NPCInfo->tempGoal;

		NPC_SetMoveGoal( NPC, &NPCInfo->captureGoal->r.currentOrigin, 16, qtrue, -1, NULL );

		//		NAV_ClearLastRoute(NPC);
		NPCInfo->goalTime = level.time + 100000;
	}

	//	NPC_BSRun();

	NPC_CheckEnemy( qtrue, qfalse, qtrue );

	//FIXME: Need melee code
	if ( NPC->enemy ) {//See if we can shoot him
		vector3		delta, forward;
		vector3		angleToEnemy;
		vector3		hitspot, muzzle, diff, enemy_org, enemy_head;
		float		distanceToEnemy;
		qboolean	attack_ok = qfalse;
		qboolean	dead_on = qfalse;
		float		attack_scale = 1.0f;
		float		aim_off;
		float		max_aim_off = 64;

		//Yaw to enemy
		VectorMA( &NPC->enemy->r.absmin, 0.5f, &NPC->enemy->r.maxs, &enemy_org );
		CalcEntitySpot( NPC, SPOT_WEAPON, &muzzle );

		VectorSubtract( &enemy_org, &muzzle, &delta );
		vectoangles( &delta, &angleToEnemy );
		distanceToEnemy = VectorNormalize( &delta );

		if ( !NPC_EnemyTooFar( NPC->enemy, distanceToEnemy*distanceToEnemy, qtrue ) ) {
			attack_ok = qtrue;
		}

		if ( attack_ok ) {
			NPC_UpdateShootAngles( &angleToEnemy, qfalse, qtrue );

			NPCInfo->enemyLastVisibility = enemyVisibility;
			enemyVisibility = NPC_CheckVisibility( NPC->enemy, CHECK_FOV );//CHECK_360|//CHECK_PVS|

			if ( enemyVisibility == VIS_FOV ) {//He's in our FOV

				attack_ok = qtrue;
				CalcEntitySpot( NPC->enemy, SPOT_HEAD, &enemy_head );

				if ( attack_ok ) {
					trace_t		tr;
					gentity_t	*traceEnt;
					//are we gonna hit him if we shoot at his center?
					trap->Trace( &tr, &muzzle, NULL, NULL, &enemy_org, NPC->s.number, MASK_SHOT, qfalse, 0, 0 );
					traceEnt = &g_entities[tr.entityNum];
					if ( traceEnt != NPC->enemy &&
						(!traceEnt || !traceEnt->client || !NPC->client->enemyTeam || NPC->client->enemyTeam != traceEnt->client->playerTeam) ) {//no, so shoot for the head
						attack_scale *= 0.75f;
						trap->Trace( &tr, &muzzle, NULL, NULL, &enemy_head, NPC->s.number, MASK_SHOT, qfalse, 0, 0 );
						traceEnt = &g_entities[tr.entityNum];
					}

					VectorCopy( &tr.endpos, &hitspot );

					if ( traceEnt == NPC->enemy || (traceEnt->client && NPC->client->enemyTeam && NPC->client->enemyTeam == traceEnt->client->playerTeam) ) {
						dead_on = qtrue;
					}
					else {
						attack_scale *= 0.5f;
						if ( NPC->client->playerTeam ) {
							if ( traceEnt && traceEnt->client && traceEnt->client->playerTeam ) {
								if ( NPC->client->playerTeam == traceEnt->client->playerTeam ) {//Don't shoot our own team
									attack_ok = qfalse;
								}
							}
						}
					}
				}

				if ( attack_ok ) {
					//ok, now adjust pitch aim
					VectorSubtract( &hitspot, &muzzle, &delta );
					vectoangles( &delta, &angleToEnemy );
					NPC->NPC->desiredPitch = angleToEnemy.pitch;
					NPC_UpdateShootAngles( &angleToEnemy, qtrue, qfalse );

					if ( !dead_on ) {//We're not going to hit him directly, try a suppressing fire
						//see if where we're going to shoot is too far from his origin
						AngleVectors( &NPCInfo->shootAngles, &forward, NULL, NULL );
						VectorMA( &muzzle, distanceToEnemy, &forward, &hitspot );
						VectorSubtract( &hitspot, &enemy_org, &diff );
						aim_off = VectorLength( &diff );
						if ( aim_off > random() * max_aim_off )//FIXME: use aim value to allow poor aim?
						{
							attack_scale *= 0.75f;
							//see if where we're going to shoot is too far from his head
							VectorSubtract( &hitspot, &enemy_head, &diff );
							aim_off = VectorLength( &diff );
							if ( aim_off > random() * max_aim_off ) {
								attack_ok = qfalse;
							}
						}
						attack_scale *= (max_aim_off - aim_off + 1) / max_aim_off;
					}
				}
			}
		}

		if ( attack_ok ) {
			if ( NPC_CheckAttack( attack_scale ) ) {//check aggression to decide if we should shoot
				enemyVisibility = VIS_SHOOT;
				WeaponThink( qtrue );
			}
		}
		//Don't do this- only for when stationary and trying to shoot an enemy
		//		else
		//			NPC->cantHitEnemyCounter++;
	}
	else {//FIXME:
		NPC_UpdateShootAngles( &NPC->client->ps.viewangles, qtrue, qtrue );
	}

	if ( !ucmd.forwardmove && !ucmd.rightmove ) {//We reached our captureGoal
		if ( trap->ICARUS_IsInitialized( NPC->s.number ) ) {
			trap->ICARUS_TaskIDComplete( (sharedEntity_t *)NPC, TID_BSTATE );
		}
	}
}

void Disappear( gentity_t *self ) {
	//	ClientDisconnect(self);
	self->s.eFlags |= EF_NODRAW;
	self->think = 0;
	self->nextthink = -1;
}

void MakeOwnerInvis( gentity_t *self );
void BeamOut( gentity_t *self ) {
	//	gentity_t *tent = G_Spawn();

	/*
		tent->owner = self;
		tent->think = MakeOwnerInvis;
		tent->nextthink = level.time + 1800;
		//G_AddEvent( ent, EV_PLAYER_TELEPORT, 0 );
		tent = G_TempEntity( self->client->pcurrentOrigin, EV_PLAYER_TELEPORT );
		*/
	//fixme: doesn't actually go away!
	self->nextthink = level.time + 1500;
	self->think = Disappear;
	self->s.teamowner = TEAM_FREE;
	self->client->playerTeam = (npcteam_t)self->s.teamowner;
	//self->r.svFlags |= SVF_BEAMING; //this appears unused in SP as well
}

void NPC_BSCinematic( void ) {

	if ( NPCInfo->scriptFlags & SCF_FIRE_WEAPON ) {
		WeaponThink( qtrue );
	}

	if ( UpdateGoal() ) {//have a goalEntity
		//move toward goal, should also face that goal
		NPC_MoveToGoal( qtrue );
	}

	if ( NPCInfo->watchTarget ) {//have an entity which we want to keep facing
		//NOTE: this will override any angles set by NPC_MoveToGoal
		vector3 eyes, viewSpot, viewvec, viewangles;

		CalcEntitySpot( NPC, SPOT_HEAD_LEAN, &eyes );
		CalcEntitySpot( NPCInfo->watchTarget, SPOT_HEAD_LEAN, &viewSpot );

		VectorSubtract( &viewSpot, &eyes, &viewvec );

		vectoangles( &viewvec, &viewangles );

		NPCInfo->lockedDesiredYaw = NPCInfo->desiredYaw = viewangles.yaw;
		NPCInfo->lockedDesiredPitch = NPCInfo->desiredPitch = viewangles.pitch;
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

void NPC_BSWait( void ) {
	NPC_UpdateAngles( qtrue, qtrue );
}


void NPC_BSInvestigate( void ) {
}

qboolean NPC_CheckInvestigate( int alertEventNum ) {
	gentity_t	*owner = level.alertEvents[alertEventNum].owner;
	int		invAdd = level.alertEvents[alertEventNum].level;
	vector3	soundPos;
	float	soundRad = level.alertEvents[alertEventNum].radius;
	float	earshot = NPCInfo->stats.earshot;

	VectorCopy( &level.alertEvents[alertEventNum].position, &soundPos );

	//NOTE: Trying to preserve previous investigation behavior
	if ( !owner )
		return qfalse;

	if ( owner->s.eType != ET_PLAYER && owner->s.eType != ET_NPC && owner == NPCInfo->goalEntity )
		return qfalse;

	if ( owner->s.eFlags & EF_NODRAW )
		return qfalse;

	if ( owner->flags & FL_NOTARGET )
		return qfalse;

	if ( soundRad < earshot )
		return qfalse;

	//if(!trap->InPVSIgnorePortals(ent->r.currentOrigin, NPC->r.currentOrigin))//should we be able to hear through areaportals?
	if ( !trap->InPVS( &soundPos, &NPC->r.currentOrigin ) ) {//can hear through doors?
		return qfalse;
	}

	if ( owner->client && owner->client->playerTeam && NPC->client->playerTeam && owner->client->playerTeam != NPC->client->playerTeam ) {
		if ( (float)NPCInfo->investigateCount >= (NPCInfo->stats.vigilance * 200) && owner ) {//If investigateCount == 10, just take it as enemy and go
			if ( ValidEnemy( owner ) ) {//FIXME: run angerscript
				G_SetEnemy( NPC, owner );
				NPCInfo->goalEntity = NPC->enemy;
				NPCInfo->goalRadius = 12;
				NPCInfo->behaviorState = BS_HUNT_AND_KILL;
				return qtrue;
			}
		}
		else
			NPCInfo->investigateCount += invAdd;
		//run awakescript
		G_ActivateBehavior( NPC, BSET_AWAKE );

		/*
		if ( Q_irand(0, 10) > 7 )
		{
		NPC_AngerSound();
		}
		*/

		//NPCInfo->hlookCount = NPCInfo->vlookCount = 0;
		NPCInfo->eventOwner = owner;
		VectorCopy( &soundPos, &NPCInfo->investigateGoal );
		if ( NPCInfo->investigateCount > 20 )
			NPCInfo->investigateDebounceTime = level.time + 10000;
		else
			NPCInfo->investigateDebounceTime = level.time + (NPCInfo->investigateCount * 500);
		NPCInfo->tempBehavior = BS_INVESTIGATE;
		return qtrue;
	}

	return qfalse;
}


/*
void NPC_BSSleep( void )
*/
void NPC_BSSleep( void ) {
	int alertEvent = NPC_CheckAlertEvents( qtrue, qfalse, -1, qfalse, AEL_MINOR );

	//There is an event to look at
	if ( alertEvent >= 0 ) {
		G_ActivateBehavior( NPC, BSET_AWAKE );
		return;
	}

	/*
	if ( level.time > NPCInfo->enemyCheckDebounceTime )
	{
	if ( NPC_CheckSoundEvents() != -1 )
	{//only 1 alert per second per 0.1f of vigilance
	NPCInfo->enemyCheckDebounceTime = level.time + (NPCInfo->stats.vigilance * 10000);
	G_ActivateBehavior(NPC, BSET_AWAKE);
	}
	}
	*/
}

qboolean NPC_MoveDirClear( int forwardmove, int rightmove, qboolean reset );
void NPC_BSFollowLeader( void ) {
	vector3		vec;
	float		leaderDist;
	visibility_t	leaderVis;
	int			curAnim;

	if ( !NPC->client->leader ) {//ok, stand guard until we find an enemy
		if ( NPCInfo->tempBehavior == BS_HUNT_AND_KILL )
			NPCInfo->tempBehavior = BS_DEFAULT;
		else {
			NPCInfo->tempBehavior = BS_STAND_GUARD;
			NPC_BSStandGuard();
		}
		return;
	}

	if ( !NPC->enemy ) {//no enemy, find one
		NPC_CheckEnemy( NPCInfo->confusionTime < level.time, qfalse, qtrue );//don't find new enemy if this is tempbehav
		if ( NPC->enemy ) {//just found one
			NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
		}
		else {
			if ( !(NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) ) {
				int eventID = NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_MINOR );
				if ( level.alertEvents[eventID].level >= AEL_SUSPICIOUS && (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES) ) {
					NPCInfo->lastAlertID = level.alertEvents[eventID].ID;
					if ( !level.alertEvents[eventID].owner ||
						!level.alertEvents[eventID].owner->client ||
						level.alertEvents[eventID].owner->health <= 0 ||
						level.alertEvents[eventID].owner->client->playerTeam != NPC->client->enemyTeam ) {//not an enemy
					}
					else {
						//FIXME: what if can't actually see enemy, don't know where he is... should we make them just become very alert and start looking for him?  Or just let combat AI handle this... (act as if you lost him)
						G_SetEnemy( NPC, level.alertEvents[eventID].owner );
						NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
						NPCInfo->enemyLastSeenTime = level.time;
						TIMER_Set( NPC, "attackDelay", Q_irand( 500, 1000 ) );
					}
				}

			}
		}
		if ( !NPC->enemy ) {
			if ( NPC->client->leader
				&& NPC->client->leader->enemy
				&& NPC->client->leader->enemy != NPC
				&& ((NPC->client->leader->enemy->client&&NPC->client->leader->enemy->client->playerTeam == NPC->client->enemyTeam)
				|| (NPC->client->leader->enemy->r.svFlags&SVF_NONNPC_ENEMY && NPC->client->leader->enemy->alliedTeam == NPC->client->enemyTeam))
				&& NPC->client->leader->enemy->health > 0 ) {
				G_SetEnemy( NPC, NPC->client->leader->enemy );
				NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
				NPCInfo->enemyLastSeenTime = level.time;
			}
		}
	}
	else {
		if ( NPC->enemy->health <= 0 || (NPC->enemy->flags&FL_NOTARGET) ) {
			G_ClearEnemy( NPC );
			if ( NPCInfo->enemyCheckDebounceTime > level.time + 1000 ) {
				NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 1000, 2000 );
			}
		}
		else if ( NPC->client->ps.weapon && NPCInfo->enemyCheckDebounceTime < level.time ) {
			NPC_CheckEnemy( (NPCInfo->confusionTime < level.time || NPCInfo->tempBehavior != BS_FOLLOW_LEADER), qfalse, qtrue );//don't find new enemy if this is tempbehav
		}
	}

	if ( NPC->enemy && NPC->client->ps.weapon ) {//If have an enemy, face him and fire
		if ( NPC->client->ps.weapon == WP_SABER )//|| NPCInfo->confusionTime>level.time )
		{//lightsaber user or charmed enemy
			if ( NPCInfo->tempBehavior != BS_FOLLOW_LEADER ) {//not already in a temp bState
				//go after the guy
				NPCInfo->tempBehavior = BS_HUNT_AND_KILL;
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}

		enemyVisibility = NPC_CheckVisibility( NPC->enemy, CHECK_FOV | CHECK_SHOOT );//CHECK_360|CHECK_PVS|
		if ( enemyVisibility > VIS_PVS ) {//face
			vector3	enemy_org, muzzle, delta, angleToEnemy;

			CalcEntitySpot( NPC->enemy, SPOT_HEAD, &enemy_org );
			NPC_AimWiggle( &enemy_org );

			CalcEntitySpot( NPC, SPOT_WEAPON, &muzzle );

			VectorSubtract( &enemy_org, &muzzle, &delta );
			vectoangles( &delta, &angleToEnemy );

			NPCInfo->desiredYaw = angleToEnemy.yaw;
			NPCInfo->desiredPitch = angleToEnemy.pitch;
			NPC_UpdateFiringAngles( qtrue, qtrue );

			if ( enemyVisibility >= VIS_SHOOT ) {//shoot
				NPC_AimAdjust( 2 );
				if ( NPC_GetHFOVPercentage( &NPC->enemy->r.currentOrigin, &NPC->r.currentOrigin, &NPC->client->ps.viewangles, NPCInfo->stats.hfov ) > 0.6f
					&& NPC_GetHFOVPercentage( &NPC->enemy->r.currentOrigin, &NPC->r.currentOrigin, &NPC->client->ps.viewangles, NPCInfo->stats.vfov ) > 0.5f ) {//actually withing our front cone
					WeaponThink( qtrue );
				}
			}
			else
				NPC_AimAdjust( 1 );

			//NPC_CheckCanAttack(1.0f, qfalse);
		}
		else
			NPC_AimAdjust( -1 );
	}
	else {//FIXME: combine with vector calc below
		vector3	head, leaderHead, delta, angleToLeader;

		CalcEntitySpot( NPC->client->leader, SPOT_HEAD, &leaderHead );
		CalcEntitySpot( NPC, SPOT_HEAD, &head );
		VectorSubtract( &leaderHead, &head, &delta );
		vectoangles( &delta, &angleToLeader );
		VectorNormalize( &delta );
		NPC->NPC->desiredYaw = angleToLeader.yaw;
		NPC->NPC->desiredPitch = angleToLeader.pitch;

		NPC_UpdateAngles( qtrue, qtrue );
	}

	//leader visible?
	leaderVis = NPC_CheckVisibility( NPC->client->leader, CHECK_PVS | CHECK_360 | CHECK_SHOOT );//			ent->e_UseFunc = useF_NULL;


	//Follow leader, stay within visibility and a certain distance, maintain a distance from.
	curAnim = NPC->client->ps.legsAnim;
	if ( curAnim != BOTH_ATTACK1 && curAnim != BOTH_ATTACK2 && curAnim != BOTH_ATTACK3 && curAnim != BOTH_MELEE1 && curAnim != BOTH_MELEE2 ) {//Don't move toward leader if we're in a full-body attack anim
		//FIXME, use IdealDistance to determine if we need to close distance
		float	followDist = 96.0f;//FIXME:  If there are enmies, make this larger?
		float	backupdist, walkdist, minrundist;
		float	leaderHDist;

		if ( (int)NPCInfo->followDist ) {
			followDist = NPCInfo->followDist;
		}
		backupdist = followDist / 2.0f;
		walkdist = followDist*0.83f;
		minrundist = followDist*1.33f;

		VectorSubtract( &NPC->client->leader->r.currentOrigin, &NPC->r.currentOrigin, &vec );
		leaderDist = VectorLength( &vec );//FIXME: make this just nav distance?
		//never get within their radius horizontally
		vec.z = 0;
		leaderHDist = VectorLength( &vec );
		if ( leaderHDist > backupdist && (leaderVis != VIS_SHOOT || leaderDist > walkdist) ) {//We should close in?
			NPCInfo->goalEntity = NPC->client->leader;

			NPC_SlideMoveToGoal();
			if ( leaderVis == VIS_SHOOT && leaderDist < minrundist )
				ucmd.buttons |= BUTTON_WALKING;
		}
		else if ( leaderDist < backupdist ) {//We should back off?
			NPCInfo->goalEntity = NPC->client->leader;
			NPC_SlideMoveToGoal();

			//reversing direction
			ucmd.forwardmove = -ucmd.forwardmove;
			ucmd.rightmove = -ucmd.rightmove;
			VectorScale( &NPC->client->ps.moveDir, -1, &NPC->client->ps.moveDir );
		}//otherwise, stay where we are
		//check for do not enter and stop if there's one there...
		if ( ucmd.forwardmove || ucmd.rightmove || VectorCompare( &vec3_origin, &NPC->client->ps.moveDir ) )
			NPC_MoveDirClear( ucmd.forwardmove, ucmd.rightmove, qtrue );
	}
}
#define	APEX_HEIGHT		200.0f
#define	PARA_WIDTH		(sqrtf(APEX_HEIGHT)+sqrtf(APEX_HEIGHT))
#define	JUMP_SPEED		200.0f
void NPC_BSJump( void ) {
	vector3		dir, angles, p1, p2, apex;
	float		time, height, forward, z, xy, dist, yawError, apexHeight;

	if ( !NPCInfo->goalEntity ) {//Should have task completed the navgoal
		return;
	}

	if ( NPCInfo->jumpState != JS_JUMPING && NPCInfo->jumpState != JS_LANDING ) {
		//Face navgoal
		VectorSubtract( &NPCInfo->goalEntity->r.currentOrigin, &NPC->r.currentOrigin, &dir );
		vectoangles( &dir, &angles );
		NPCInfo->desiredPitch = NPCInfo->lockedDesiredPitch = AngleNormalize360( angles.pitch );
		NPCInfo->desiredYaw = NPCInfo->lockedDesiredYaw = AngleNormalize360( angles.yaw );
	}

	NPC_UpdateAngles( qtrue, qtrue );
	yawError = AngleDelta( NPC->client->ps.viewangles.yaw, NPCInfo->desiredYaw );
	//We don't really care about pitch here

	switch ( NPCInfo->jumpState ) {
	case JS_FACING:
		if ( yawError < MIN_ANGLE_ERROR ) {//Facing it, Start crouching
			NPC_SetAnim( NPC, SETANIM_LEGS, BOTH_CROUCH1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			NPCInfo->jumpState = JS_CROUCHING;
		}
		break;
	case JS_CROUCHING:
		if ( NPC->client->ps.legsTimer > 0 ) {//Still playing crouching anim
			return;
		}

		//Create a parabola

		if ( NPC->r.currentOrigin.z > NPCInfo->goalEntity->r.currentOrigin.z ) {
			VectorCopy( &NPC->r.currentOrigin, &p1 );
			VectorCopy( &NPCInfo->goalEntity->r.currentOrigin, &p2 );
		}
		else if ( NPC->r.currentOrigin.z < NPCInfo->goalEntity->r.currentOrigin.z ) {
			VectorCopy( &NPCInfo->goalEntity->r.currentOrigin, &p1 );
			VectorCopy( &NPC->r.currentOrigin, &p2 );
		}
		else {
			VectorCopy( &NPC->r.currentOrigin, &p1 );
			VectorCopy( &NPCInfo->goalEntity->r.currentOrigin, &p2 );
		}

		//z = xy*xy
		VectorSubtract( &p2, &p1, &dir );
		dir.z = 0;

		//Get xy and z diffs
		xy = VectorNormalize( &dir );
		z = p1.z - p2.z;

		apexHeight = APEX_HEIGHT / 2;
		/*
		//Determine most desirable apex height
		apexHeight = (APEX_HEIGHT * PARA_WIDTH/xy) + (APEX_HEIGHT * z/128);
		if ( apexHeight < APEX_HEIGHT * 0.5f )
		{
		apexHeight = APEX_HEIGHT*0.5f;
		}
		else if ( apexHeight > APEX_HEIGHT * 2 )
		{
		apexHeight = APEX_HEIGHT*2;
		}
		*/

		//FIXME: length of xy will change curve of parabola, need to account for this
		//somewhere... PARA_WIDTH

		z = (sqrtf( apexHeight + z ) - sqrtf( apexHeight ));

		assert( z >= 0 );

		//		Com_Printf("apex is %4.2f percent from p1: ", (xy-z)*0.5f/xy*100.0f);

		// Don't need to set apex xy if NPC is jumping directly up.
		if ( xy > 0.0f ) {
			xy -= z;
			xy *= 0.5f;
			assert( xy > 0 );
		}

		VectorMA( &p1, xy, &dir, &apex );
		apex.z += apexHeight;

		VectorCopy( &apex, &NPC->pos1 );

		//Now we have the apex, aim for it
		height = apex.z - NPC->r.currentOrigin.z;
		time = sqrtf( height / (.5f * NPC->client->ps.gravity) );
		if ( !(int)time ) {
			//			Com_Printf("ERROR no time in jump\n");
			return;
		}

		// set s.origin2 to the push velocity
		VectorSubtract( &apex, &NPC->r.currentOrigin, &NPC->client->ps.velocity );
		NPC->client->ps.velocity.z = 0;
		dist = VectorNormalize( &NPC->client->ps.velocity );

		forward = dist / time;
		VectorScale( &NPC->client->ps.velocity, forward, &NPC->client->ps.velocity );

		NPC->client->ps.velocity.z = time * NPC->client->ps.gravity;

		//		Com_Printf( "%s jumping %s, gravity at %4.0f percent\n", NPC->targetname, vtos(NPC->client->ps.velocity), NPC->client->ps.gravity/8.0f );

		NPC->flags |= FL_NO_KNOCKBACK;
		NPCInfo->jumpState = JS_JUMPING;
		//FIXME: jumpsound?
		break;
	case JS_JUMPING:

		if ( showBBoxes ) {
			VectorAdd( &NPC->r.mins, &NPC->pos1, &p1 );
			VectorAdd( &NPC->r.maxs, &NPC->pos1, &p2 );
			//	G_Cube( &p1, &p2, &NPCDEBUG_BLUE, 0.5f );
		}

		if ( NPC->s.groundEntityNum != ENTITYNUM_NONE ) {//Landed, start landing anim
			//FIXME: if the
			VectorClear( &NPC->client->ps.velocity );
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_LAND1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			NPCInfo->jumpState = JS_LANDING;
			//FIXME: landsound?
		}
		else if ( NPC->client->ps.legsTimer > 0 ) {//Still playing jumping anim
			//FIXME: apply jump velocity here, a couple frames after start, not right away
			return;
		}
		else {//still in air, but done with jump anim, play inair anim
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_INAIR1, SETANIM_FLAG_OVERRIDE );
		}
		break;
	case JS_LANDING:
		if ( NPC->client->ps.legsTimer > 0 ) {//Still playing landing anim
			return;
		}
		else {
			NPCInfo->jumpState = JS_WAITING;


			//task complete no matter what...
			NPC_ClearGoal();
			NPCInfo->goalTime = level.time;
			NPCInfo->aiFlags &= ~NPCAI_MOVING;
			ucmd.forwardmove = 0;
			NPC->flags &= ~FL_NO_KNOCKBACK;
			//Return that the goal was reached
			trap->ICARUS_TaskIDComplete( (sharedEntity_t *)NPC, TID_MOVE_NAV );

			//Or should we keep jumping until reached goal?

			/*
			NPCInfo->goalEntity = UpdateGoal();
			if ( !NPCInfo->goalEntity )
			{
			NPC->flags &= ~FL_NO_KNOCKBACK;
			Q3_TaskIDComplete( NPC, TID_MOVE_NAV );
			}
			*/

		}
		break;
	case JS_WAITING:
	default:
		NPCInfo->jumpState = JS_FACING;
		break;
	}
}

void NPC_BSRemove( void ) {
	NPC_UpdateAngles( qtrue, qtrue );
	if ( !trap->InPVS( &NPC->r.currentOrigin, &g_entities[0].r.currentOrigin ) )//FIXME: use cg.vieworg?
	{ //rwwFIXMEFIXME: Care about all clients instead of just 0?
		G_UseTargets2( NPC, NPC, NPC->target3 );
		NPC->s.eFlags |= EF_NODRAW;
		NPC->s.eType = ET_INVISIBLE;
		NPC->r.contents = 0;
		NPC->health = 0;
		NPC->targetname = NULL;

		//Disappear in half a second
		NPC->think = G_FreeEntity;
		NPC->nextthink = level.time + FRAMETIME;
	}//FIXME: else allow for out of FOV???
}

void NPC_BSSearch( void ) {
	NPC_CheckEnemy( qtrue, qfalse, qtrue );
	//Look for enemies, if find one:
	if ( NPC->enemy ) {
		if ( NPCInfo->tempBehavior == BS_SEARCH ) {//if tempbehavior, set tempbehavior to default
			NPCInfo->tempBehavior = BS_DEFAULT;
		}
		else {//if bState, change to run and shoot
			NPCInfo->behaviorState = BS_HUNT_AND_KILL;
			NPC_BSRunAndShoot();
		}
		return;
	}

	//FIXME: what if our goalEntity is not NULL and NOT our tempGoal - they must
	//want us to do something else?  If tempBehavior, just default, else set
	//to run and shoot...?

	//FIXME: Reimplement

	if ( !NPCInfo->investigateDebounceTime ) {//On our way to a tempGoal
		float	minGoalReachedDistSquared = 32 * 32;
		vector3	vec;

		//Keep moving toward our tempGoal
		NPCInfo->goalEntity = NPCInfo->tempGoal;

		VectorSubtract( &NPCInfo->tempGoal->r.currentOrigin, &NPC->r.currentOrigin, &vec );
		if ( vec.z < 24 ) {
			vec.z = 0;
		}

		if ( NPCInfo->tempGoal->waypoint != WAYPOINT_NONE ) {
			/*
			//FIXME: can't get the radius...
			float	wpRadSq = waypoints[NPCInfo->tempGoal->waypoint].radius * waypoints[NPCInfo->tempGoal->waypoint].radius;
			if ( minGoalReachedDistSquared > wpRadSq )
			{
			minGoalReachedDistSquared = wpRadSq;
			}
			*/

			minGoalReachedDistSquared = 32 * 32;//12*12;
		}

		if ( VectorLengthSquared( &vec ) < minGoalReachedDistSquared ) {
			//Close enough, just got there
			NPC->waypoint = NAV_FindClosestWaypointForEnt( NPC, WAYPOINT_NONE );

			if ( (NPCInfo->homeWp == WAYPOINT_NONE) || (NPC->waypoint == WAYPOINT_NONE) ) {
				//Heading for or at an invalid waypoint, get out of this bState
				if ( NPCInfo->tempBehavior == BS_SEARCH ) {//if tempbehavior, set tempbehavior to default
					NPCInfo->tempBehavior = BS_DEFAULT;
				}
				else {//if bState, change to stand guard
					NPCInfo->behaviorState = BS_STAND_GUARD;
					NPC_BSRunAndShoot();
				}
				return;
			}

			if ( NPC->waypoint == NPCInfo->homeWp ) {
				//Just Reached our homeWp, if this is the first time, run your lostenemyscript
				if ( NPCInfo->aiFlags & NPCAI_ENROUTE_TO_HOMEWP ) {
					NPCInfo->aiFlags &= ~NPCAI_ENROUTE_TO_HOMEWP;
					G_ActivateBehavior( NPC, BSET_LOSTENEMY );
				}

			}

			//Com_Printf("Got there.\n");
			//Com_Printf("Looking...");
			if ( !Q_irand( 0, 1 ) ) {
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, SETANIM_FLAG_NORMAL );
			}
			else {
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_GUARD_IDLE1, SETANIM_FLAG_NORMAL );
			}
			NPCInfo->investigateDebounceTime = level.time + Q_irand( 3000, 10000 );
		}
		else {
			NPC_MoveToGoal( qtrue );
		}
	}
	else {
		//We're there
		if ( NPCInfo->investigateDebounceTime > level.time ) {
			//Still waiting around for a bit
			//Turn angles every now and then to look around
			if ( NPCInfo->tempGoal->waypoint != WAYPOINT_NONE ) {
				if ( !Q_irand( 0, 30 ) ) {
					int	numEdges = trap->Nav_GetNodeNumEdges( NPCInfo->tempGoal->waypoint );

					if ( numEdges != WAYPOINT_NONE ) {
						int branchNum = Q_irand( 0, numEdges - 1 );

						vector3	branchPos, lookDir;

						int nextWp = trap->Nav_GetNodeEdge( NPCInfo->tempGoal->waypoint, branchNum );
						trap->Nav_GetNodePosition( nextWp, &branchPos );

						VectorSubtract( &branchPos, &NPCInfo->tempGoal->r.currentOrigin, &lookDir );
						NPCInfo->desiredYaw = AngleNormalize360( vectoyaw( &lookDir ) + flrand( -45, 45 ) );
					}

					//pick an angle +-45 degrees off of the dir of a random branch
					//from NPCInfo->tempGoal->waypoint
					//int branch = Q_irand( 0, (waypoints[NPCInfo->tempGoal->waypoint].numNeighbors - 1) );
					//int	nextWp = waypoints[NPCInfo->tempGoal->waypoint].nextWaypoint[branch][NPC->client->moveType];
					//vector3	lookDir;

					//VectorSubtract( waypoints[nextWp].origin, NPCInfo->tempGoal->r.currentOrigin, lookDir );
					//Look in that direction +- 45 degrees
					//NPCInfo->desiredYaw = AngleNormalize360( vectoyaw( lookDir ) + flrand( -45, 45 ) );
				}
			}
			//Com_Printf(".");
		}
		else {//Just finished waiting
			NPC->waypoint = NAV_FindClosestWaypointForEnt( NPC, WAYPOINT_NONE );

			if ( NPC->waypoint == NPCInfo->homeWp ) {
				int	numEdges = trap->Nav_GetNodeNumEdges( NPCInfo->tempGoal->waypoint );

				if ( numEdges != WAYPOINT_NONE ) {
					int branchNum = Q_irand( 0, numEdges - 1 );

					int nextWp = trap->Nav_GetNodeEdge( NPCInfo->homeWp, branchNum );
					trap->Nav_GetNodePosition( nextWp, &NPCInfo->tempGoal->r.currentOrigin );
					NPCInfo->tempGoal->waypoint = nextWp;
				}

				/*
				//Pick a random branch
				int branch = Q_irand( 0, (waypoints[NPCInfo->homeWp].numNeighbors - 1) );
				int	nextWp = waypoints[NPCInfo->homeWp].nextWaypoint[branch][NPC->client->moveType];

				VectorCopy( waypoints[nextWp].origin, NPCInfo->tempGoal->r.currentOrigin );
				NPCInfo->tempGoal->waypoint = nextWp;
				//Com_Printf("\nHeading for wp %d...\n", waypoints[NPCInfo->homeWp].nextWaypoint[branch][NPC->client->moveType]);
				*/
			}
			else {//At a branch, so return home
				trap->Nav_GetNodePosition( NPCInfo->homeWp, &NPCInfo->tempGoal->r.currentOrigin );
				NPCInfo->tempGoal->waypoint = NPCInfo->homeWp;
				/*
				VectorCopy( waypoints[NPCInfo->homeWp].origin, NPCInfo->tempGoal->r.currentOrigin );
				NPCInfo->tempGoal->waypoint = NPCInfo->homeWp;
				//Com_Printf("\nHeading for wp %d...\n", NPCInfo->homeWp);
				*/
			}

			NPCInfo->investigateDebounceTime = 0;
			//Start moving toward our tempGoal
			NPCInfo->goalEntity = NPCInfo->tempGoal;
			NPC_MoveToGoal( qtrue );
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

void NPC_BSSearchStart( int homeWp, bState_t bState ) {
	//FIXME: Reimplement
	if ( homeWp == WAYPOINT_NONE ) {
		homeWp = NAV_FindClosestWaypointForEnt( NPC, WAYPOINT_NONE );
		if ( NPC->waypoint == WAYPOINT_NONE ) {
			NPC->waypoint = homeWp;
		}
	}
	NPCInfo->homeWp = homeWp;
	NPCInfo->tempBehavior = bState;
	NPCInfo->aiFlags |= NPCAI_ENROUTE_TO_HOMEWP;
	NPCInfo->investigateDebounceTime = 0;
	trap->Nav_GetNodePosition( homeWp, &NPCInfo->tempGoal->r.currentOrigin );
	NPCInfo->tempGoal->waypoint = homeWp;
	//Com_Printf("\nHeading for wp %d...\n", NPCInfo->homeWp);
}

// Use in extreme circumstances only
void NPC_BSNoClip( void ) {
	if ( UpdateGoal() ) {
		vector3	dir, forward, right, angles, up = { 0, 0, 1 };
		float	fDot, rDot, uDot;

		VectorSubtract( &NPCInfo->goalEntity->r.currentOrigin, &NPC->r.currentOrigin, &dir );

		vectoangles( &dir, &angles );
		NPCInfo->desiredYaw = angles.yaw;

		AngleVectors( &NPC->r.currentAngles, &forward, &right, NULL );

		VectorNormalize( &dir );

		fDot = DotProduct( &forward, &dir ) * 127;
		rDot = DotProduct( &right, &dir ) * 127;
		uDot = DotProduct( &up, &dir ) * 127;

		ucmd.forwardmove = floorf( fDot );
		ucmd.rightmove = floorf( rDot );
		ucmd.upmove = floorf( uDot );
	}
	else {
		//Cut velocity?
		VectorClear( &NPC->client->ps.velocity );
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

void NPC_BSWander( void ) {//FIXME: don't actually go all the way to the next waypoint, just move in fits and jerks...?
	if ( !NPCInfo->investigateDebounceTime ) {//Starting out
		float	minGoalReachedDistSquared = 64;//32*32;
		vector3	vec;

		//Keep moving toward our tempGoal
		NPCInfo->goalEntity = NPCInfo->tempGoal;

		VectorSubtract( &NPCInfo->tempGoal->r.currentOrigin, &NPC->r.currentOrigin, &vec );

		if ( NPCInfo->tempGoal->waypoint != WAYPOINT_NONE ) {
			minGoalReachedDistSquared = 64;
		}

		if ( VectorLengthSquared( &vec ) < minGoalReachedDistSquared ) {
			//Close enough, just got there
			NPC->waypoint = NAV_FindClosestWaypointForEnt( NPC, WAYPOINT_NONE );

			if ( !Q_irand( 0, 1 ) ) {
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, SETANIM_FLAG_NORMAL );
			}
			else {
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_GUARD_IDLE1, SETANIM_FLAG_NORMAL );
			}
			//Just got here, so Look around for a while
			NPCInfo->investigateDebounceTime = level.time + Q_irand( 3000, 10000 );
		}
		else {
			//Keep moving toward goal
			NPC_MoveToGoal( qtrue );
		}
	}
	else {
		//We're there
		if ( NPCInfo->investigateDebounceTime > level.time ) {
			//Still waiting around for a bit
			//Turn angles every now and then to look around
			if ( NPCInfo->tempGoal->waypoint != WAYPOINT_NONE ) {
				if ( !Q_irand( 0, 30 ) ) {
					int	numEdges = trap->Nav_GetNodeNumEdges( NPCInfo->tempGoal->waypoint );

					if ( numEdges != WAYPOINT_NONE ) {
						int branchNum = Q_irand( 0, numEdges - 1 );

						vector3	branchPos, lookDir;

						int	nextWp = trap->Nav_GetNodeEdge( NPCInfo->tempGoal->waypoint, branchNum );
						trap->Nav_GetNodePosition( nextWp, &branchPos );

						VectorSubtract( &branchPos, &NPCInfo->tempGoal->r.currentOrigin, &lookDir );
						NPCInfo->desiredYaw = AngleNormalize360( vectoyaw( &lookDir ) + flrand( -45, 45 ) );
					}
				}
			}
		}
		else {//Just finished waiting
			NPC->waypoint = NAV_FindClosestWaypointForEnt( NPC, WAYPOINT_NONE );

			if ( NPC->waypoint != WAYPOINT_NONE ) {
				int	numEdges = trap->Nav_GetNodeNumEdges( NPC->waypoint );

				if ( numEdges != WAYPOINT_NONE ) {
					int branchNum = Q_irand( 0, numEdges - 1 );

					int nextWp = trap->Nav_GetNodeEdge( NPC->waypoint, branchNum );
					trap->Nav_GetNodePosition( nextWp, &NPCInfo->tempGoal->r.currentOrigin );
					NPCInfo->tempGoal->waypoint = nextWp;
				}

				NPCInfo->investigateDebounceTime = 0;
				//Start moving toward our tempGoal
				NPCInfo->goalEntity = NPCInfo->tempGoal;
				NPC_MoveToGoal( qtrue );
			}
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
void WP_DropWeapon( gentity_t *dropper, vector3 *velocity );
void ChangeWeapon( gentity_t *ent, int newWeapon );

//FIXME: say "don't shoot!" if we weren't already surrendering
void NPC_Surrender( void ) {
	if ( NPC->client->ps.weaponTime || PM_InKnockDown( &NPC->client->ps ) ) {
		return;
	}
	if ( NPC->s.weapon != WP_NONE &&
		NPC->s.weapon != WP_STUN_BATON &&
		NPC->s.weapon != WP_SABER ) {
		//WP_DropWeapon( NPC, NULL ); //rwwFIXMEFIXME: Do this (gonna need a system for notifying client of removal)
	}
	if ( NPCInfo->surrenderTime < level.time - 5000 ) {//haven't surrendered for at least 6 seconds, tell them what you're doing
		//FIXME: need real dialogue EV_SURRENDER
		NPCInfo->blockedSpeechDebounceTime = 0;//make sure we say this
		G_AddVoiceEvent( NPC, Q_irand( EV_PUSHED1, EV_PUSHED3 ), 3000 );
	}
	//	NPC_SetAnim( NPC, SETANIM_TORSO, TORSO_SURRENDER_START, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
	//	NPC->client->ps.torsoTimer = 1000;
	NPCInfo->surrenderTime = level.time + 1000;//stay surrendered for at least 1 second
	//FIXME: while surrendering, make a big sight/sound alert? Or G_AlertTeam?
}

qboolean NPC_CheckSurrender( void ) {
	if ( !trap->ICARUS_TaskIDPending( (sharedEntity_t *)NPC, TID_MOVE_NAV )
		&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
		&& !NPC->client->ps.weaponTime && !PM_InKnockDown( &NPC->client->ps )
		&& NPC->enemy && NPC->enemy->client && NPC->enemy->enemy == NPC && NPC->enemy->s.weapon != WP_NONE && NPC->enemy->s.weapon != WP_STUN_BATON
		&& NPC->enemy->health > 20 && NPC->enemy->painDebounceTime < level.time - 3000 && NPC->enemy->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time - 1000 ) {//don't surrender if scripted to run somewhere or if we're in the air or if we're busy or if we don't have an enemy or if the enemy is not mad at me or is hurt or not a threat or busy being attacked
		//FIXME: even if not in a group, don't surrender if there are other enemies in the PVS and within a certain range?
		if ( NPC->s.weapon != WP_ROCKET_LAUNCHER
			&& NPC->s.weapon != WP_REPEATER
			&& NPC->s.weapon != WP_FLECHETTE
			&& NPC->s.weapon != WP_SABER ) {//jedi and heavy weapons guys never surrender
			//FIXME: rework all this logic into some orderly fashion!!!
			if ( NPC->s.weapon != WP_NONE ) {//they have a weapon so they'd have to drop it to surrender
				//don't give up unless low on health
				if ( NPC->health > 25 /*|| NPC->health >= NPC->max_health*/ ) { //rwwFIXMEFIXME: Keep max health not a ps state?
					return qfalse;
				}
				//if ( g_crosshairEntNum == NPC->s.number && NPC->painDebounceTime > level.time )
				if ( NPC_SomeoneLookingAtMe( NPC ) && NPC->painDebounceTime > level.time ) {//if he just shot me, always give up
					//fall through
				}
				else {//don't give up unless facing enemy and he's very close
					if ( !InFOV( NPC->enemy, NPC, 60, 30 ) ) {//I'm not looking at them
						return qfalse;
					}
					else if ( DistanceSquared( &NPC->r.currentOrigin, &NPC->enemy->r.currentOrigin ) < 65536/*256*256*/ ) {//they're not close
						return qfalse;
					}
					else if ( !trap->InPVS( &NPC->r.currentOrigin, &NPC->enemy->r.currentOrigin ) ) {//they're not in the same room
						return qfalse;
					}
				}
			}
			//fixme: this logic keeps making npc's randomly surrender
			/*
			if ( NPCInfo->group && NPCInfo->group->numGroup <= 1 )
			{//I'm alone but I was in a group//FIXME: surrender anyway if just melee or no weap?
			if ( NPC->s.weapon == WP_NONE
			//NPC has a weapon
			|| (NPC->enemy && NPC->enemy->s.number < MAX_CLIENTS)
			|| (NPC->enemy->s.weapon == WP_SABER&&NPC->enemy->client&&!NPC->enemy->client->ps.saberHolstered)
			|| (NPC->enemy->NPC && NPC->enemy->NPC->group && NPC->enemy->NPC->group->numGroup > 2) )
			{//surrender only if have no weapon or fighting a player or jedi or if we are outnumbered at least 3 to 1
			if ( (NPC->enemy && NPC->enemy->s.number < MAX_CLIENTS) )
			{//player is the guy I'm running from
			//if ( g_crosshairEntNum == NPC->s.number )
			if (NPC_SomeoneLookingAtMe(NPC))
			{//give up if player is aiming at me
			NPC_Surrender();
			NPC_UpdateAngles( qtrue, qtrue );
			return qtrue;
			}
			else if ( NPC->enemy->s.weapon == WP_SABER )
			{//player is using saber
			if ( InFOV( NPC, NPC->enemy, 60, 30 ) )
			{//they're looking at me
			if ( DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) < 16384 )
			{//they're close
			if ( trap->InPVS( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) )
			{//they're in the same room
			NPC_Surrender();
			NPC_UpdateAngles( qtrue, qtrue );
			return qtrue;
			}
			}
			}
			}
			}
			else if ( NPC->enemy )
			{//???
			//should NPC's surrender to others?
			if ( InFOV( NPC, NPC->enemy, 30, 30 ) )
			{//they're looking at me
			if ( DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) < 4096 )
			{//they're close
			if ( trap->InPVS( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) )
			{//they're in the same room
			//FIXME: should player-team NPCs not fire on surrendered NPCs?
			NPC_Surrender();
			NPC_UpdateAngles( qtrue, qtrue );
			return qtrue;
			}
			}
			}
			}
			}
			}
			*/
		}
	}
	return qfalse;
}

void NPC_BSFlee( void ) {//FIXME: keep checking for danger
	gentity_t *goal;

	if ( TIMER_Done( NPC, "flee" ) && NPCInfo->tempBehavior == BS_FLEE ) {
		NPCInfo->tempBehavior = BS_DEFAULT;
		NPCInfo->squadState = SQUAD_IDLE;
		//FIXME: should we set some timer to make him stay in this spot for a bit,
		//so he doesn't just suddenly turn around and come back at the enemy?
		//OR, just stop running toward goal for last second or so of flee?
	}
	if ( NPC_CheckSurrender() ) {
		return;
	}
	goal = NPCInfo->goalEntity;
	if ( !goal ) {
		goal = NPCInfo->lastGoalEntity;
		if ( !goal ) {//???!!!
			goal = NPCInfo->tempGoal;
		}
	}

	if ( goal ) {
		qboolean moved;
		qboolean reverseCourse = qtrue;

		//FIXME: if no weapon, find one and run to pick it up?

		//Let's try to find a waypoint that gets me away from this thing
		if ( NPC->waypoint == WAYPOINT_NONE ) {
			NPC->waypoint = NAV_GetNearestNode( NPC, NPC->lastWaypoint );
		}
		if ( NPC->waypoint != WAYPOINT_NONE ) {
			int	numEdges = trap->Nav_GetNodeNumEdges( NPC->waypoint );

			if ( numEdges != WAYPOINT_NONE ) {
				vector3	dangerDir;
				int		nextWp;
				int		branchNum;

				VectorSubtract( &NPCInfo->investigateGoal, &NPC->r.currentOrigin, &dangerDir );
				VectorNormalize( &dangerDir );

				for ( branchNum = 0; branchNum < numEdges; branchNum++ ) {
					vector3	branchPos, runDir;

					nextWp = trap->Nav_GetNodeEdge( NPC->waypoint, branchNum );
					trap->Nav_GetNodePosition( nextWp, &branchPos );

					VectorSubtract( &branchPos, &NPC->r.currentOrigin, &runDir );
					VectorNormalize( &runDir );
					if ( DotProduct( &runDir, &dangerDir ) > flrand( 0, 0.5f ) ) {//don't run toward danger
						continue;
					}
					//FIXME: don't want to ping-pong back and forth
					NPC_SetMoveGoal( NPC, &branchPos, 0, qtrue, -1, NULL );
					reverseCourse = qfalse;
					break;
				}
			}
		}

		moved = NPC_MoveToGoal( qfalse );//qtrue? (do try to move straight to (away from) goal)

		if ( NPC->s.weapon == WP_NONE && (moved == qfalse || reverseCourse) ) {//No weapon and no escape route... Just cower?  Need anim.
			NPC_Surrender();
			NPC_UpdateAngles( qtrue, qtrue );
			return;
		}
		//If our move failed, then just run straight away from our goal
		//FIXME: We really shouldn't do this.
		if ( moved == qfalse ) {
			vector3	dir;
			if ( reverseCourse )
				VectorSubtract( &NPC->r.currentOrigin, &goal->r.currentOrigin, &dir );
			else
				VectorSubtract( &goal->r.currentOrigin, &NPC->r.currentOrigin, &dir );
			NPCInfo->distToGoal = VectorNormalize( &dir );
			NPCInfo->desiredYaw = vectoyaw( &dir );
			NPCInfo->desiredPitch = 0;
			ucmd.forwardmove = 127;
		}
		else if ( reverseCourse ) {
			//ucmd.forwardmove *= -1;
			//ucmd.rightmove *= -1;
			//VectorScale( NPC->client->ps.moveDir, -1, NPC->client->ps.moveDir );
			NPCInfo->desiredYaw *= -1;
		}
		//FIXME: can stop after a safe distance?
		//ucmd.upmove = 0;
		ucmd.buttons &= ~BUTTON_WALKING;
		//FIXME: what do we do once we've gotten to our goal?
	}
	NPC_UpdateAngles( qtrue, qtrue );

	NPC_CheckGetNewWeapon();
}

void NPC_StartFlee( gentity_t *enemy, vector3 *dangerPoint, int dangerLevel, int fleeTimeMin, int fleeTimeMax ) {
	int cp = -1;

	if ( trap->ICARUS_TaskIDPending( (sharedEntity_t *)NPC, TID_MOVE_NAV ) ) {//running somewhere that a script requires us to go, don't interrupt that!
		return;
	}

	//if have a fleescript, run that instead
	if ( G_ActivateBehavior( NPC, BSET_FLEE ) ) {
		return;
	}
	//FIXME: play a flee sound?  Appropriate to situation?
	if ( enemy ) {
		G_SetEnemy( NPC, enemy );
	}

	//FIXME: if don't have a weapon, find nearest one we have a route to and run for it?
	if ( dangerLevel > AEL_DANGER || NPC->s.weapon == WP_NONE || ((!NPCInfo->group || NPCInfo->group->numGroup <= 1) && NPC->health <= 10) ) {//IF either great danger OR I have no weapon OR I'm alone and low on health, THEN try to find a combat point out of PVS
		cp = NPC_FindCombatPoint( &NPC->r.currentOrigin, &NPC->r.currentOrigin, dangerPoint, CP_COVER | CP_AVOID | CP_HAS_ROUTE | CP_NO_PVS, 128, -1 );
	}
	//FIXME: still happens too often...
	if ( cp == -1 ) {//okay give up on the no PVS thing
		cp = NPC_FindCombatPoint( &NPC->r.currentOrigin, &NPC->r.currentOrigin, dangerPoint, CP_COVER | CP_AVOID | CP_HAS_ROUTE, 128, -1 );
		if ( cp == -1 ) {//okay give up on the avoid
			cp = NPC_FindCombatPoint( &NPC->r.currentOrigin, &NPC->r.currentOrigin, dangerPoint, CP_COVER | CP_HAS_ROUTE, 128, -1 );
			if ( cp == -1 ) {//okay give up on the cover
				cp = NPC_FindCombatPoint( &NPC->r.currentOrigin, &NPC->r.currentOrigin, dangerPoint, CP_HAS_ROUTE, 128, -1 );
			}
		}
	}

	//see if we got a valid one
	if ( cp != -1 ) {//found a combat point
		NPC_SetCombatPoint( cp );
		NPC_SetMoveGoal( NPC, &level.combatPoints[cp].origin, 8, qtrue, cp, NULL );
		NPCInfo->behaviorState = BS_HUNT_AND_KILL;
		NPCInfo->tempBehavior = BS_DEFAULT;
	}
	else {//need to just run like hell!
		if ( NPC->s.weapon != WP_NONE ) {
			return;//let's just not flee?
		}
		else {
			//FIXME: other evasion AI?  Duck?  Strafe?  Dodge?
			NPCInfo->tempBehavior = BS_FLEE;
			//Run straight away from here... FIXME: really want to find farthest waypoint/navgoal from this pos... maybe based on alert event radius?
			NPC_SetMoveGoal( NPC, dangerPoint, 0, qtrue, -1, NULL );
			//store the danger point
			VectorCopy( dangerPoint, &NPCInfo->investigateGoal );//FIXME: make a new field for this?
		}
	}
	//FIXME: localize this Timer?
	TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
	//FIXME: is this always applicable?
	NPCInfo->squadState = SQUAD_RETREAT;
	TIMER_Set( NPC, "flee", Q_irand( fleeTimeMin, fleeTimeMax ) );
	TIMER_Set( NPC, "panic", Q_irand( 1000, 4000 ) );//how long to wait before trying to nav to a dropped weapon

	if ( NPC->client->NPC_class != CLASS_PROTOCOL ) {
		TIMER_Set( NPC, "duck", 0 );
	}
}

void G_StartFlee( gentity_t *self, gentity_t *enemy, vector3 *dangerPoint, int dangerLevel, int fleeTimeMin, int fleeTimeMax ) {
	if ( !self->NPC ) {//player
		return;
	}
	SaveNPCGlobals();
	SetNPCGlobals( self );

	NPC_StartFlee( enemy, dangerPoint, dangerLevel, fleeTimeMin, fleeTimeMax );

	RestoreNPCGlobals();
}

void NPC_BSEmplaced( void ) {
	qboolean enemyLOS = qfalse;
	qboolean enemyCS = qfalse;
	qboolean faceEnemy = qfalse;
	qboolean shoot = qfalse;
	vector3	impactPos;

	//Don't do anything if we're hurt
	if ( NPC->painDebounceTime > level.time ) {
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	if ( NPCInfo->scriptFlags & SCF_FIRE_WEAPON ) {
		WeaponThink( qtrue );
	}

	//If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt( qfalse ) == qfalse ) {
		if ( !Q_irand( 0, 30 ) ) {
			NPCInfo->desiredYaw = NPC->s.angles.y + Q_irand( -90, 90 );
		}
		if ( !Q_irand( 0, 30 ) ) {
			NPCInfo->desiredPitch = Q_irand( -20, 20 );
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	if ( NPC_ClearLOS4( NPC->enemy ) ) {
		int hit;
		gentity_t *hitEnt;

		enemyLOS = qtrue;

		hit = NPC_ShotEntity( NPC->enemy, &impactPos );
		hitEnt = &g_entities[hit];

		if ( hit == NPC->enemy->s.number || (hitEnt && hitEnt->takedamage) ) {//can hit enemy or will hit glass or other minor breakable (or in emplaced gun), so shoot anyway
			enemyCS = qtrue;
			NPC_AimAdjust( 2 );//adjust aim better longer we have clear shot at enemy
			VectorCopy( &NPC->enemy->r.currentOrigin, &NPCInfo->enemyLastSeenLocation );
		}
	}
	/*
		else if ( trap->InPVS( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) )
		{
		NPCInfo->enemyLastSeenTime = level.time;
		faceEnemy = qtrue;
		NPC_AimAdjust( -1 );//adjust aim worse longer we cannot see enemy
		}
		*/

	if ( enemyLOS ) {//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
		faceEnemy = qtrue;
	}
	if ( enemyCS ) {
		shoot = qtrue;
	}

	if ( faceEnemy ) {//face the enemy
		NPC_FaceEnemy( qtrue );
	}
	else {//we want to face in the dir we're running
		NPC_UpdateAngles( qtrue, qtrue );
	}

	if ( NPCInfo->scriptFlags & SCF_DONT_FIRE ) {
		shoot = qfalse;
	}

	if ( NPC->enemy && NPC->enemy->enemy ) {
		if ( NPC->enemy->s.weapon == WP_SABER && NPC->enemy->enemy->s.weapon == WP_SABER ) {//don't shoot at an enemy jedi who is fighting another jedi, for fear of injuring one or causing rogue blaster deflections (a la Obi Wan/Vader duel at end of ANH)
			shoot = qfalse;
		}
	}
	if ( shoot ) {//try to shoot if it's time
		if ( !(NPCInfo->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
		{
			WeaponThink( qtrue );
		}
	}
}
