/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The garage's catalog (see the header).
*/

#include "ui_local.h"
#include "ui_rally_garagedata.h"
#include "ui_rally_garage_labels.h"
#include "ui_rally_frontpage.h"
#include "../game/garage_unlocks.h"
#include "../game/profile_shared.h"

garageCatalog_t garageCatalog;

static char garageList[16384];

void Garage_TitleCase( const char *key, char *out, int size ) {
	char *c;
	qboolean start = qtrue;
	Q_strncpyz( out, key, size );
	for ( c = out; *c; c++ ) {
		if ( *c == '_' ) *c = ' ';
		*c = start ? toupper( (unsigned char)*c ) : tolower( (unsigned char)*c );
		start = !isalpha( (unsigned char)*c );
	}
}

const char *Garage_RankName( int rank ) {
#define GARAGE_RANK_NAME( name, points ) name,
	static const char *names[] = { PROFILE_RANK_TABLE( GARAGE_RANK_NAME ) };
#undef GARAGE_RANK_NAME
	if ( rank >= 0 && rank < (int)ARRAY_LEN( names ) ) return names[rank];
	return va( "rank %d", rank );
}

static qboolean Garage_Exists( const char *path ) {
	fileHandle_t file;
	int length = trap_FS_FOpenFile( path, &file, FS_READ );
	if ( length < 0 ) return qfalse;
	trap_FS_FCloseFile( file );
	return length > 0;
}

/* exporter.find_image: the icon in any of the picture formats */
static qboolean Garage_Image( const char *base, char *out, int size ) {
	static const char *extensions[] = { "tga", "png", "jpg" };
	char path[MAX_QPATH];
	int i;
	for ( i = 0; i < 3; i++ ) {
		Com_sprintf( path, sizeof( path ), "%s.%s", base, extensions[i] );
		if ( Garage_Exists( path ) ) {
			if ( out ) Q_strncpyz( out, base, size );
			return qtrue;
		}
	}
	return qfalse;
}

/* [a-z0-9_-]{1,48}, like ASSET_ID */
static qboolean Garage_ValidKey( const char *key ) {
	int i;
	if ( !key[0] || strlen( key ) >= GARAGE_KEY ) return qfalse;
	for ( i = 0; key[i]; i++ ) {
		if ( !( ( key[i] >= 'a' && key[i] <= 'z' ) || ( key[i] >= '0' && key[i] <= '9' ) || key[i] == '_' || key[i] == '-' ) ) return qfalse;
	}
	return qtrue;
}

static int Garage_Compare( const void *a, const void *b ) {
	return strcmp( (const char *)a, (const char *)b );
}

/* the names in a file list, sorted and without doubles (several paks) */
static int Garage_List( const char *path, const char *extension, char names[][GARAGE_KEY + 8], int max ) {
	int count = trap_FS_GetFileList( path, extension, garageList, sizeof( garageList ) ), found = 0, i, j;
	char *name = garageList;
	for ( i = 0; i < count; i++, name += strlen( name ) + 1 ) {
		int length = strlen( name );
		if ( length && name[length - 1] == '/' ) name[--length] = '\0';
		if ( !length || length >= GARAGE_KEY + 8 ) continue;
		for ( j = 0; j < found && strcmp( names[j], name ); j++ ) ;
		if ( j < found || found >= max ) continue;
		Q_strncpyz( names[found++], name, GARAGE_KEY + 8 );
	}
	qsort( names, found, sizeof( names[0] ), Garage_Compare );
	return found;
}

/* build_showroom.py options(): <key>.skin with an icon_<key> */
static int Garage_Skins( const char *where, garageOption_t *options, int max ) {
	static char names[256][GARAGE_KEY + 8];
	char folder[MAX_QPATH], icon[MAX_QPATH];
	int count, found = 0, i;
	Q_strncpyz( folder, where, sizeof( folder ) );	/* where may be a va() */
	count = Garage_List( folder, ".skin", names, 256 );
	for ( i = 0; i < count && found < max; i++ ) {
		char key[GARAGE_KEY + 8];
		COM_StripExtension( names[i], key, sizeof( key ) );
		Com_sprintf( icon, sizeof( icon ), "%s/icon_%s", folder, key );
		if ( !Garage_ValidKey( key ) || !Garage_Image( icon, NULL, 0 ) ) continue;
		memset( &options[found], 0, sizeof( options[found] ) );
		Q_strncpyz( options[found].key, key, GARAGE_KEY );
		Garage_TitleCase( key, options[found].name, GARAGE_NAME );
		found++;
	}
	return found;
}

void Garage_LoadCatalog( void ) {
	static char names[256][GARAGE_KEY + 8];
	garageCatalog_t *c = &garageCatalog;
	int count, i, j;

	if ( c->loaded ) return;
	memset( c, 0, sizeof( *c ) );
	c->loaded = qtrue;

	count = Garage_List( "models/players", "/", names, 256 );
	for ( i = 0; i < count && c->vehicles < GARAGE_MAX_VEHICLES; i++ ) {
		garageVehicle_t *v = &c->vehicle[c->vehicles];
		if ( !Garage_ValidKey( names[i] ) || !Garage_Exists( va( "models/players/%s/body.md3", names[i] ) ) ) continue;
		Q_strncpyz( v->key, names[i], sizeof( v->key ) );
		WS_VehicleName( v->key, v->name, sizeof( v->name ) );
		v->rank = 0;
#define GARAGE_CAR_RANK( id, label, unlock ) if ( !strcmp( v->key, id ) ) v->rank = unlock;
		GARAGE_CAR_UNLOCKS( GARAGE_CAR_RANK )
#undef GARAGE_CAR_RANK
		v->paints = Garage_Skins( va( "models/players/%s", v->key ), v->paint, GARAGE_MAX_PAINTS );
		for ( j = 0; j < v->paints; j++ ) {
			garageOption_t *p = &v->paint[j];
#define GARAGE_PAINT_LABEL( car, paint, label ) if ( !strcmp( v->key, car ) && !strcmp( p->key, paint ) ) Q_strncpyz( p->name, label, sizeof( p->name ) );
#define GARAGE_SKIN_UNLOCK( id, label, unlock ) if ( !strcmp( p->key, id ) ) { Q_strncpyz( p->name, label, sizeof( p->name ) ); p->rank = unlock; }
			UI_PAINT_LABELS( GARAGE_PAINT_LABEL )
			GARAGE_SKIN_UNLOCKS( GARAGE_SKIN_UNLOCK )
#undef GARAGE_PAINT_LABEL
#undef GARAGE_SKIN_UNLOCK
		}
		if ( v->paints ) c->vehicles++;
	}

	c->wheels = Garage_Skins( "models/players/wheels", c->wheel, GARAGE_MAX_WHEELS );
	for ( j = 0; j < c->wheels; j++ ) {
#define GARAGE_WHEEL_LABEL( id, label ) if ( !strcmp( c->wheel[j].key, id ) ) Q_strncpyz( c->wheel[j].name, label, GARAGE_NAME );
		UI_WHEEL_LABELS( GARAGE_WHEEL_LABEL )
#undef GARAGE_WHEEL_LABEL
	}
	c->heads = Garage_Skins( "models/players/heads", c->head, GARAGE_MAX_HEADS );

	/* plates: the pictures themselves, not the player's own */
	count = Garage_List( "models/players/plates", "", names, 256 );
	for ( i = 0; i < count && c->plates < GARAGE_MAX_PLATES; i++ ) {
		char key[GARAGE_KEY + 8], *ext = strrchr( names[i], '.' );
		int k;
		if ( !ext || ( Q_stricmp( ext, ".tga" ) && Q_stricmp( ext, ".jpg" ) && Q_stricmp( ext, ".png" ) ) ) continue;
		COM_StripExtension( names[i], key, sizeof( key ) );
		for ( k = 0; key[k]; k++ ) key[k] = tolower( (unsigned char)key[k] );
		if ( !Garage_ValidKey( key ) || !Q_stricmpn( key, "player", 6 ) ) continue;
		if ( Garage_FindOption( c->plate, c->plates, key ) >= 0 ) continue;
		memset( &c->plate[c->plates], 0, sizeof( c->plate[0] ) );
		Q_strncpyz( c->plate[c->plates].key, key, GARAGE_KEY );
		Q_strncpyz( c->plate[c->plates].ext, ext + 1, sizeof( c->plate[0].ext ) );
		Garage_TitleCase( key, c->plate[c->plates].name, GARAGE_NAME );
		c->plates++;
	}
}

int Garage_FindVehicle( const char *key ) {
	int i;
	for ( i = 0; i < garageCatalog.vehicles; i++ ) {
		if ( !strcmp( garageCatalog.vehicle[i].key, key ) ) return i;
	}
	return -1;
}

int Garage_FindOption( const garageOption_t *options, int count, const char *key ) {
	int i;
	for ( i = 0; i < count; i++ ) {
		if ( !strcmp( options[i].key, key ) ) return i;
	}
	return -1;
}

void Garage_PaintIcon( int vehicle, int paint, char *out, int size ) {
	Com_sprintf( out, size, "models/players/%s/icon_%s", garageCatalog.vehicle[vehicle].key, garageCatalog.vehicle[vehicle].paint[paint].key );
}

void Garage_OptionIcon( const char *folder, const garageOption_t *option, char *out, int size ) {
	if ( option->ext[0] ) Com_sprintf( out, size, "models/players/plates/%s.%s", option->key, option->ext );
	else Com_sprintf( out, size, "models/players/%s/icon_%s", folder, option->key );
}
