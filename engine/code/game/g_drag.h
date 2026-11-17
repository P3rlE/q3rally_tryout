/* Server-side rules for the v0.9 Drag Race prototype. */
#ifndef G_DRAG_H
#define G_DRAG_H

void G_DragInit( void );
qboolean G_DragCanJoin( void );
void G_DragClientSpawn( gentity_t *ent );
void G_DragClientDisconnect( int clientNum );
void G_DragStarterThink( gentity_t *starter );
void G_DragFilterCommand( gentity_t *ent, usercmd_t *cmd );
qboolean G_DragTryGearChange( gentity_t *ent, int direction, int commandTime );
qboolean G_DragBotInput( int clientNum, usercmd_t *cmd );
int G_DragMatchBestOf( void );
void G_DragSimulateMatch( int skillA, int skillB, int bestOf, int *winsA, int *winsB );

/* g_drag_tourney.c */
qboolean G_DragTourneyActive( void );
void G_DragTourneyInit( void );
void G_DragTourneyReset( void );
void G_DragTourneyPublish( int recipient );
qboolean G_DragTourneyLineup( void );
void G_DragTourneyMatchOver( int winnerClient, const int *winsByClient );
qboolean G_DragTourneyDriver( int clientNum );
void Cmd_DragSkip_f( gentity_t *ent );
void G_DragPostMove( gentity_t *ent, const vec3_t oldOrigin, int oldTime );
void G_DragRunFrame( void );
qboolean G_DragCheckExit( void );
int G_DragCompare( int clientA, int clientB );
qboolean G_DragLadderParticipant( int clientNum );
qboolean G_DragLadderDuel( void );
qboolean G_DragLadderIdentityMatches( int clientNum );
qboolean G_DragPopulateLadderPlayer( int clientNum, ladderPlayerPayload_t *player );

#endif
