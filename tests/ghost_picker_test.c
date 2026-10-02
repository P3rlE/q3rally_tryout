/*
 * Ghost Race ladder ghost picker (cg_ghost_picker.c): list and ghost
 * transfer parsing, own-vehicle filter, picking, remembered pick and keys.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../engine/code/cgame/cg_ghost_picker.c"

cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
vmCvar_t cg_developer;
vmCvar_t cg_ghostPlayback;
static snapshot_t s_snap;

/* ---- command tokenizer (quotes like Cmd_TokenizeString) ----------------- */
static char s_args[64][MAX_STRING_CHARS];
static int s_argc;

static void Tokenize( const char *text ) {
	s_argc = 0;
	while ( *text ) {
		int n = 0;
		while ( *text == ' ' ) text++;
		if ( !*text ) break;
		if ( *text == '"' ) {
			text++;
			while ( *text && *text != '"' ) s_args[s_argc][n++] = *text++;
			if ( *text == '"' ) text++;
		} else {
			while ( *text && *text != ' ' ) s_args[s_argc][n++] = *text++;
		}
		s_args[s_argc++][n] = '\0';
	}
}
int trap_Argc( void ) { return s_argc; }
const char *CG_Argv( int arg ) { return ( arg >= 0 && arg < s_argc ) ? s_args[arg] : ""; }

static void Server( const char *text ) {
	Tokenize( text );
	assert( CG_LadderGhost_ServerCommand( s_args[0] ) );
}

/* ---- stubs ---------------------------------------------------------------- */
static char s_sent[16][256];
static int s_sentCount;
static char s_lastPick[256] = "";
static int s_catcher;
static int s_localFileExists;
static int s_localLoads;

void QDECL Com_Error( int level, const char *fmt, ... ) { (void)level; (void)fmt; assert( 0 ); }
void QDECL Com_Printf( const char *fmt, ... ) { (void)fmt; }
void QDECL CG_Printf( const char *fmt, ... ) { (void)fmt; }
void trap_SendClientCommand( const char *s ) { Q_strncpyz( s_sent[s_sentCount++ & 15], s, 256 ); }
void trap_Cvar_Set( const char *name, const char *value ) {
	if ( !strcmp( name, "cg_ladderGhostLast" ) ) Q_strncpyz( s_lastPick, value, sizeof( s_lastPick ) );
}
void trap_Cvar_VariableStringBuffer( const char *name, char *buffer, int size ) {
	Q_strncpyz( buffer, !strcmp( name, "cg_ladderGhostLast" ) ? s_lastPick : "", size );
}
int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	(void)qpath; (void)mode;
	*f = s_localFileExists ? 1 : 0;
	return s_localFileExists ? 100 : -1;
}
void trap_FS_FCloseFile( fileHandle_t f ) { (void)f; }
qboolean CG_LoadLadderGhostFile( const char *path, int lapMs ) {
	(void)path; (void)lapMs;
	s_localLoads++;
	cg.ladderGhost.valid = qtrue;
	cg.ladderGhost.frameCount = 2;
	return qtrue;
}
int trap_Key_GetCatcher( void ) { return s_catcher; }
void trap_Key_SetCatcher( int catcher ) { s_catcher = catcher; }
qboolean CG_HUDOptionsIsOpen( void ) { return qfalse; }
void CG_SetScreenPlacement( screenPlacement_e hpos, screenPlacement_e vpos ) { (void)hpos; (void)vpos; }
void CG_FillRect( float x, float y, float w, float h, const float *color ) { (void)x; (void)y; (void)w; (void)h; (void)color; }
void CG_DrawRect( float x, float y, float w, float h, float size, const float *color ) { (void)x; (void)y; (void)w; (void)h; (void)size; (void)color; }
static int s_drawn;
void CG_DrawIngameString( int x, int y, const char *text, int style, float scale, const float *color ) {
	(void)x; (void)y; (void)text; (void)style; (void)scale; (void)color;
	s_drawn++;
}

static void SendList( void ) {
	Server( "lghostlist 3 0" );
	Server( "lghostents 0 3 59000 evo \"Alpha\" ghosts/ladder/m/a.ghost"
		" 59500 sidepipe \"Gamma Ray\" ghosts/ladder/m/c.ghost 61000 evo \"Beta\" ghosts/ladder/m/b.ghost" );
	Server( "lghostlistdone" );
}

int main( void ) {
	int i;

	cgs.gametype = GT_GHOST;
	Q_strncpyz( cgs.mapname, "maps/q3r_testtrack.bsp", sizeof( cgs.mapname ) );
	cg.clientNum = 0;
	cg.snap = &s_snap;
	Q_strncpyz( cgs.clientinfo[0].modelName, "Evo", sizeof( cgs.clientinfo[0].modelName ) );
	CG_LadderGhost_Reset();

	SendList();
	assert( cg.ladderGhostListReady && cg.ladderGhostEntryCount == 3 );
	assert( !strcmp( cg.ladderGhostEntries[1].name, "Gamma Ray" ) );
	assert( !strcmp( cg.ladderGhostEntries[1].cacheFile, "ghosts/ladder/m/c.ghost" ) );

	/* The picker opens by itself before the race; own car first. */
	CG_LadderGhost_DrawPicker();
	assert( cg.ladderPickerOpen && ( s_catcher & KEYCATCH_CGAME ) );
	{
		int visible[MAX_LADDER_GHOST_ENTRIES];
		assert( CG_LadderGhost_Visible( visible, MAX_LADDER_GHOST_ENTRIES ) == 2 );
		cg.ladderPickerAllVehicles = qtrue;
		assert( CG_LadderGhost_Visible( visible, MAX_LADDER_GHOST_ENTRIES ) == 3 );
		cg.ladderPickerAllVehicles = qfalse;
	}
	s_drawn = 0;
	CG_LadderGhost_DrawPicker();
	assert( s_drawn > 5 );

	/* Down to "Beta" (row 2 of own car), ENTER: not cached -> server pick. */
	assert( CG_LadderGhost_KeyEvent( K_DOWNARROW ) );
	assert( CG_LadderGhost_KeyEvent( K_DOWNARROW ) );
	assert( CG_LadderGhost_KeyEvent( K_DOWNARROW ) );   /* clamped */
	assert( cg.ladderPickerCursor == 2 );
	assert( CG_LadderGhost_KeyEvent( K_ENTER ) );
	assert( !cg.ladderPickerOpen && !( s_catcher & KEYCATCH_CGAME ) );
	assert( cg.ladderGhostSelected == 2 && cg.ladderGhostPending );
	assert( !strcmp( s_sent[s_sentCount - 1], "lghostpick 2" ) );
	assert( !strcmp( s_lastPick, "q3r_testtrack|61000|evo|Beta" ) );
	assert( !CG_LadderGhost_KeyEvent( K_ENTER ) );

	/* Streamed ghost: meta, data in order, done. */
	Server( "lghostmeta 2 61000 3" );
	Server( "lghostdata 0 2 0 10.0 20.0 30.0 1.0 90.0 0.0 100 20.0 20.0 30.0 2.0 91.0 0.0" );
	Server( "lghostdata 2 1 61000 30.0 20.0 30.0 3.0 92.0 0.0" );
	Server( "lghostdone 2" );
	assert( cg.ladderGhostAvailable && cg.ladderGhost.valid && !cg.ladderGhostPending );
	assert( cg.ladderGhost.frameCount == 3 && cg.ladderGhost.duration == 61000 );
	assert( cg.ladderGhost.frames[1].angles[YAW] == 91.0f );
	assert( cg.ladderGhost.frames[0].velocity[0] == 100.0f );
	{
		qboolean err;
		assert( !strcmp( CG_LadderGhost_StatusText( &err ), "VS BETA" ) && !err );
	}

	/* Out-of-order chunk fails the transfer. */
	CG_LadderGhost_Pick( 1 );
	Server( "lghostmeta 1 59500 3" );
	Server( "lghostdata 1 1 0 0 0 0 0 0 0" );
	assert( cg.ladderGhostFailed && !cg.ladderGhost.valid );
	{
		qboolean err;
		assert( !strcmp( CG_LadderGhost_StatusText( &err ), "GHOST FAILED" ) && err );
	}
	/* Data for another pick is ignored. */
	Server( "lghostmeta 0 59000 3" );
	assert( !cg.ladderGhostPending );
	Server( "lghostfail 1 download" );
	assert( cg.ladderGhostFailed );

	/* Cached locally: loaded without asking the server. */
	s_localFileExists = 1;
	i = s_sentCount;
	CG_LadderGhost_Pick( 0 );
	assert( s_sentCount == i && s_localLoads == 1 && cg.ladderGhostAvailable );
	s_localFileExists = 0;

	/* map_restart: list again, the loaded ghost stays without a new transfer. */
	i = s_sentCount;
	SendList();
	assert( cg.ladderGhostSelected == 0 && cg.ladderGhostAvailable && s_sentCount == i && s_localLoads == 1 );

	/* cgame restart: the remembered pick is selected again, no picker. */
	CG_LadderGhost_Reset();
	SendList();
	assert( cg.ladderGhostSelected == 0 && cg.ladderPickerAutoShown );
	CG_LadderGhost_DrawPicker();
	assert( !cg.ladderPickerOpen );

	/* Row 0 drops the ladder ghost and is remembered as "none". */
	CG_LadderGhost_TogglePicker_f();
	assert( cg.ladderPickerOpen );
	assert( CG_LadderGhost_KeyEvent( K_ENTER ) );
	assert( cg.ladderGhostSelected == -1 && !strcmp( s_lastPick, "none|q3r_testtrack" ) );
	CG_LadderGhost_Reset();
	SendList();
	CG_LadderGhost_DrawPicker();
	assert( !cg.ladderPickerOpen );
	/* ... but only for this map. */
	Q_strncpyz( cgs.mapname, "maps/q3r_other.bsp", sizeof( cgs.mapname ) );
	CG_LadderGhost_Reset();
	SendList();
	CG_LadderGhost_DrawPicker();
	assert( cg.ladderPickerOpen );
	CG_LadderGhost_CatcherCleared();
	s_catcher = 0;
	Q_strncpyz( cgs.mapname, "maps/q3r_testtrack.bsp", sizeof( cgs.mapname ) );

	/* ESC (catcher cleared by the engine) closes; race start closes too. */
	s_lastPick[0] = '\0';
	CG_LadderGhost_Reset();
	SendList();
	CG_LadderGhost_DrawPicker();
	assert( cg.ladderPickerOpen );
	CG_LadderGhost_CatcherCleared();
	assert( !cg.ladderPickerOpen );
	CG_LadderGhost_TogglePicker_f();
	cg_entities[0].startRaceTime = 5000;
	CG_LadderGhost_DrawPicker();
	assert( !cg.ladderPickerOpen && !( s_catcher & KEYCATCH_CGAME ) );

	/* Explicit personal / base playback: no automatic picker. */
	cg_entities[0].startRaceTime = 0;
	cg_ghostPlayback.integer = 1;
	CG_LadderGhost_Reset();
	SendList();
	CG_LadderGhost_DrawPicker();
	assert( !cg.ladderPickerOpen );

	assert( !CG_LadderGhost_ServerCommand( "ghostmeta" ) );
	puts( "ok" );
	return 0;
}
