/* Server-side rules for the v0.9 Drag Race prototype. */
#ifndef G_DRAG_H
#define G_DRAG_H

void G_DragInit( void );
qboolean G_DragCanJoin( void );
void G_DragClientSpawn( gentity_t *ent );
void G_DragClientDisconnect( int clientNum );
void G_DragStarterThink( gentity_t *starter );
void G_DragFilterCommand( gentity_t *ent, usercmd_t *cmd );
void G_DragPostMove( gentity_t *ent, const vec3_t oldOrigin, int oldTime );
void G_DragRunFrame( void );
qboolean G_DragCheckExit( void );
int G_DragCompare( int clientA, int clientB );

#endif
