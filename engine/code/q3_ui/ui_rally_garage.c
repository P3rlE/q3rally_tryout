/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The garage drawn like the WebUI's #garage (garage.js): the showroom with the
vehicle's picture or the 3D view, "Make it yours" with a carousel each for
the vehicle, paint, wheels, driver and license plate, Apply setup, the web
profile buttons and the four favorite slots. It does what the client did
for the page: Apply sets model, rim, head and plate; the slots save through
UI_WebSaveFavorite; the web profile goes through ui_webprofile_action.
Layout in ui_rally_garagelayout.c, the catalog in ui_rally_garagedata.c.
*/

#include "ui_local.h"
#include "ui_rally_garage.h"
#include "ui_rally_garagedata.h"
#include "ui_rally_garagelayout.h"
#include "ui_rally_library.h"
#include "ui_rally_offline.h"
#include "ui_rally_rivalslayout.h"

#define GARAGE_ARROW_LEFT	"\x9c"	/* ‹ and › in the body faces (build_ui_fonts.py) */
#define GARAGE_ARROW_RIGHT	"\x9d"
#define GARAGE_ELLIPSIS		"\x9e"

static const vec4_t garageCurrent = { 0x25 / 255.0f, 0x20 / 255.0f, 0x1d / 255.0f, 1.0f };	/* .carousel-current */
static const vec4_t garageMuted   = { 0xbb / 255.0f, 0xbb / 255.0f, 0xbb / 255.0f, 1.0f };	/* #bbb */
static const vec4_t garageCount   = { 0x99 / 255.0f, 0x99 / 255.0f, 0x99 / 255.0f, 1.0f };	/* #999 */

typedef enum { GARAGE_DRAG_NONE, GARAGE_DRAG_PAGE, GARAGE_DRAG_VIEW } garageDrag_t;

typedef struct {
	int		paint, wheel, head, plate;
	qboolean set;
} garageSetup_t;

static struct {
	menuframework_s	menu;
	garageLayout_t	layout;
	int				vehicle;
	garageSetup_t	setup[GARAGE_MAX_VEHICLES];
	int				rank;
	char			title[GARAGE_NAME + 16];
	char			status[256];
	char			profileStatus[256];
	char			load[GARAGE_SLOTS][GARAGE_NAME * 2 + 16];
	int				favorite[GARAGE_SLOTS][2];		/* vehicle, paint; -1 empty */
	char			favoriteField[GARAGE_SLOTS][5][GARAGE_KEY];
	/* the 3D view */
	qboolean		view3d;
	playerInfo_t	car;
	char			carModel[MAX_QPATH * 4];
	float			yaw, elevation, distance;
	garageDrag_t	drag;
	float			dragX, dragY, dragScroll;
	float			scroll;
} s_garage;

/* -------------------------------------------------------------------------
   The state, as garage.js keeps it
   ------------------------------------------------------------------------- */

static qboolean Garage_Locked( int rank ) {
	return rank > 0 && rank > s_garage.rank;
}

static int Garage_Wrap( int index, int count ) {
	return count > 0 ? ( index % count + count ) % count : 0;
}

/* refreshVehicle(): a vehicle seen for the first time gets its first
 * unlocked paint, SVT Cobra wheels, the classic driver and the Californian
 * plate */
static garageSetup_t *Garage_Setup( int vehicle ) {
	garageSetup_t *s = &s_garage.setup[vehicle];
	const garageVehicle_t *v = &garageCatalog.vehicle[vehicle];
	int i;
	if ( s->set ) return s;
	s->set = qtrue;
	s->paint = 0;
	for ( i = 0; i < v->paints; i++ ) {
		if ( !Garage_Locked( v->paint[i].rank ) ) { s->paint = i; break; }
	}
	s->wheel = Garage_FindOption( garageCatalog.wheel, garageCatalog.wheels, "svt_cobra" );
	if ( s->wheel < 0 ) s->wheel = 0;
	s->head = Garage_FindOption( garageCatalog.head, garageCatalog.heads, "classic" );
	if ( s->head < 0 ) s->head = 0;
	s->plate = Garage_FindOption( garageCatalog.plate, garageCatalog.plates, "usa_california" );
	if ( s->plate < 0 ) s->plate = 0;
	return s;
}

static qboolean Garage_SetupLocked( void ) {
	const garageVehicle_t *v = &garageCatalog.vehicle[s_garage.vehicle];
	return Garage_Locked( v->rank ) || Garage_Locked( v->paint[Garage_Setup( s_garage.vehicle )->paint].rank );
}

/* refreshStatus() */
static void Garage_RefreshStatus( void ) {
	const garageVehicle_t *v = &garageCatalog.vehicle[s_garage.vehicle];
	const garageOption_t *paint = &v->paint[Garage_Setup( s_garage.vehicle )->paint];
	s_garage.status[0] = '\0';
	if ( Garage_Locked( v->rank ) ) {
		Q_strcat( s_garage.status, sizeof( s_garage.status ), va( "%s unlocks at %s.", v->name, Garage_RankName( v->rank ) ) );
	}
	if ( Garage_Locked( paint->rank ) ) {
		if ( s_garage.status[0] ) Q_strcat( s_garage.status, sizeof( s_garage.status ), " " );
		Q_strcat( s_garage.status, sizeof( s_garage.status ), va( "%s unlocks at %s.", paint->name, Garage_RankName( paint->rank ) ) );
	}
	if ( !s_garage.status[0] ) Q_strncpyz( s_garage.status, "Preview. Apply setup to use it in Q3Rally.", sizeof( s_garage.status ) );
}

static void Garage_UpdateCar( void ) {
	const garageVehicle_t *v = &garageCatalog.vehicle[s_garage.vehicle];
	const garageSetup_t *s = Garage_Setup( s_garage.vehicle );
	char model[MAX_QPATH * 4], skin[MAX_QPATH];
	vec3_t angles;
	Com_sprintf( model, sizeof( model ), "%s/%s %s %s %s", v->key, v->paint[s->paint].key, garageCatalog.wheel[s->wheel].key,
	             garageCatalog.head[s->head].key, garageCatalog.plate[s->plate].key );
	if ( !s_garage.view3d || !strcmp( model, s_garage.carModel ) ) return;
	Q_strncpyz( s_garage.carModel, model, sizeof( s_garage.carModel ) );
	VectorClear( angles );
	Com_sprintf( skin, sizeof( skin ), "%s/%s", v->key, v->paint[s->paint].key );
	UI_PlayerInfo_SetModel( &s_garage.car, skin, garageCatalog.wheel[s->wheel].key,
	                        garageCatalog.head[s->head].key, garageCatalog.plate[s->plate].key );
	UI_PlayerInfo_SetInfo( &s_garage.car, LEGS_IDLE, TORSO_STAND, angles, angles, WP_NONE, qfalse );
}

static void Garage_Select( int vehicle ) {
	s_garage.vehicle = Garage_Wrap( vehicle, garageCatalog.vehicles );
	Garage_Setup( s_garage.vehicle );
	Garage_RefreshStatus();
	Garage_UpdateCar();
}

/* applyInitialSetup(): the setup in use (model, rim, head, plate) */
static void Garage_Initial( void ) {
	char model[MAX_QPATH], value[GARAGE_KEY], *paint;
	int vehicle, option;
	garageSetup_t *s;

	trap_Cvar_VariableStringBuffer( "model", model, sizeof( model ) );
	paint = strchr( model, '/' );
	if ( paint ) *paint++ = '\0';
	vehicle = Garage_FindVehicle( model );
	if ( vehicle < 0 ) {
		vehicle = Garage_FindVehicle( "alpine" );
		if ( vehicle < 0 ) vehicle = 0;
		Garage_Select( vehicle );
		return;
	}
	s = &s_garage.setup[vehicle];
	memset( s, 0, sizeof( *s ) );
	Garage_Setup( vehicle );
	option = paint ? Garage_FindOption( garageCatalog.vehicle[vehicle].paint, garageCatalog.vehicle[vehicle].paints, paint ) : -1;
	if ( option >= 0 ) s->paint = option;
	trap_Cvar_VariableStringBuffer( "rim", value, sizeof( value ) );
	if ( ( option = Garage_FindOption( garageCatalog.wheel, garageCatalog.wheels, value ) ) >= 0 ) s->wheel = option;
	trap_Cvar_VariableStringBuffer( "head", value, sizeof( value ) );
	if ( ( option = Garage_FindOption( garageCatalog.head, garageCatalog.heads, value ) ) >= 0 ) s->head = option;
	trap_Cvar_VariableStringBuffer( "plate", value, sizeof( value ) );
	if ( ( option = Garage_FindOption( garageCatalog.plate, garageCatalog.plates, value ) ) >= 0 ) s->plate = option;
	Garage_Select( vehicle );
}

static qboolean Garage_FileExists( const char *path ) {
	fileHandle_t file;
	int length = trap_FS_FOpenFile( path, &file, FS_READ );
	if ( length < 0 ) return qfalse;
	trap_FS_FCloseFile( file );
	return length > 0;
}

/* what the client checked before it applied or saved a setup */
static qboolean Garage_SetupFiles( const garageSetup_t *s ) {
	const garageVehicle_t *v = &garageCatalog.vehicle[s_garage.vehicle];
	const garageOption_t *plate = &garageCatalog.plate[s->plate];
	return Garage_FileExists( va( "models/players/%s/%s.skin", v->key, v->paint[s->paint].key ) ) &&
	       Garage_FileExists( va( "models/players/wheels/%s.skin", garageCatalog.wheel[s->wheel].key ) ) &&
	       Garage_FileExists( va( "models/players/heads/%s.md3", garageCatalog.head[s->head].key ) ) &&
	       ( Garage_FileExists( va( "models/players/plates/%s.tga", plate->key ) ) ||
	         Garage_FileExists( va( "models/players/plates/%s.jpg", plate->key ) ) );
}

/* submitSetup(): Apply (slot -1) or Save here */
static void Garage_Submit( int slot ) {
	const garageVehicle_t *v = &garageCatalog.vehicle[s_garage.vehicle];
	const garageSetup_t *s = Garage_Setup( s_garage.vehicle );
	qboolean ok;

	if ( Garage_SetupLocked() ) return;
	ok = Garage_SetupFiles( s );
	if ( ok && slot >= 0 ) {
		trap_Cvar_Set( "ui_webFavoriteSetup", va( "%s %s %s %s %s", v->key, v->paint[s->paint].key, garageCatalog.wheel[s->wheel].key,
		                                          garageCatalog.head[s->head].key, garageCatalog.plate[s->plate].key ) );
		ok = UI_WebSaveFavorite( slot );
		trap_Cvar_Set( "ui_webFavoriteSetup", "" );
	} else if ( ok ) {
		trap_Cvar_Set( "model", va( "%s/%s", v->key, v->paint[s->paint].key ) );
		trap_Cvar_Set( "rim", garageCatalog.wheel[s->wheel].key );
		trap_Cvar_Set( "head", garageCatalog.head[s->head].key );
		trap_Cvar_Set( "plate", garageCatalog.plate[s->plate].key );
	}
	Garage_RefreshStatus();
	if ( slot >= 0 ) {
		Q_strncpyz( s_garage.status, ok ? va( "Setup saved in slot %d.", slot + 1 ) : "Could not save the favorite. Check that a driver profile is active.",
		            sizeof( s_garage.status ) );
	} else {
		Q_strncpyz( s_garage.status, ok ? "Setup applied in Q3Rally." : "Setup unavailable in this build. The previous setup was kept.", sizeof( s_garage.status ) );
	}
}

/* the favorites as the profile keeps them (UI_WebFavoriteChar) */
static void Garage_ReadFavorites( void ) {
	int slot, field, i;
	for ( slot = 0; slot < GARAGE_SLOTS; slot++ ) {
		int vehicle, paint = -1;
		for ( field = 0; field < 5; field++ ) {
			char *out = s_garage.favoriteField[slot][field];
			for ( i = 0; i < GARAGE_KEY - 1; i++ ) {
				out[i] = (char)UI_WebFavoriteChar( slot, field, i );
				if ( !out[i] ) break;
			}
			out[i] = '\0';
		}
		vehicle = s_garage.favoriteField[slot][0][0] ? Garage_FindVehicle( s_garage.favoriteField[slot][0] ) : -1;
		if ( vehicle >= 0 ) paint = Garage_FindOption( garageCatalog.vehicle[vehicle].paint, garageCatalog.vehicle[vehicle].paints, s_garage.favoriteField[slot][1] );
		s_garage.favorite[slot][0] = paint >= 0 ? vehicle : -1;
		s_garage.favorite[slot][1] = paint;
	}
}

/* a slot's load button: preview the saved setup */
static void Garage_LoadFavorite( int slot ) {
	int vehicle = s_garage.favorite[slot][0], option;
	garageSetup_t *s;
	if ( vehicle < 0 ) return;
	s = Garage_Setup( vehicle );
	s->paint = s_garage.favorite[slot][1];
	if ( ( option = Garage_FindOption( garageCatalog.wheel, garageCatalog.wheels, s_garage.favoriteField[slot][2] ) ) >= 0 ) s->wheel = option;
	if ( ( option = Garage_FindOption( garageCatalog.head, garageCatalog.heads, s_garage.favoriteField[slot][3] ) ) >= 0 ) s->head = option;
	if ( ( option = Garage_FindOption( garageCatalog.plate, garageCatalog.plates, s_garage.favoriteField[slot][4] ) ) >= 0 ) s->plate = option;
	Garage_Select( vehicle );
}

/* "Upload to web profile": what the client sent for the page */
static void Garage_Upload( int slot ) {
	char fields[5][GARAGE_KEY];
	int field;
	if ( s_garage.favorite[slot][0] < 0 ) return;
	for ( field = 0; field < 5; field++ ) {
		Q_strncpyz( fields[field], s_garage.favoriteField[slot][field], sizeof( fields[field] ) );
		if ( field >= 3 && !fields[field][0] ) trap_Cvar_VariableStringBuffer( field == 3 ? "head" : "plate", fields[field], sizeof( fields[field] ) );
		if ( !fields[field][0] ) return;
	}
	trap_Cvar_Set( "ui_webprofile_action", va( "favorite:%d %s %s %s %s %s", slot, fields[0], fields[1], fields[2], fields[3], fields[4] ) );
}

/* the web profile's answers: a setup to preview, a status line */
static void Garage_WebProfile( void ) {
	char payload[400], *cursor, uuid[40], current[40], fields[5][GARAGE_KEY];
	int i, paint, wheel, option;

	UI_WebImportFavorites();
	trap_Cvar_VariableStringBuffer( "ui_webprofile_setup", payload, sizeof( payload ) );
	if ( payload[0] ) {
		trap_Cvar_Set( "ui_webprofile_setup", "" );
		cursor = payload;
		Q_strncpyz( uuid, COM_Parse( &cursor ), sizeof( uuid ) );
		trap_Cvar_VariableStringBuffer( "cl_uuid", current, sizeof( current ) );
		for ( i = 0; i < 5; i++ ) Q_strncpyz( fields[i], COM_Parse( &cursor ), sizeof( fields[i] ) );
		/* cloudSetup(): only for the vehicle in the showroom */
		if ( !Q_stricmp( uuid, current ) && !strcmp( fields[0], garageCatalog.vehicle[s_garage.vehicle].key ) ) {
			garageSetup_t *s = Garage_Setup( s_garage.vehicle );
			paint = Garage_FindOption( garageCatalog.vehicle[s_garage.vehicle].paint, garageCatalog.vehicle[s_garage.vehicle].paints, fields[1] );
			wheel = Garage_FindOption( garageCatalog.wheel, garageCatalog.wheels, fields[2] );
			if ( paint >= 0 && wheel >= 0 ) {
				s->paint = paint;
				s->wheel = wheel;
				if ( ( option = Garage_FindOption( garageCatalog.head, garageCatalog.heads, fields[3] ) ) >= 0 ) s->head = option;
				if ( ( option = Garage_FindOption( garageCatalog.plate, garageCatalog.plates, fields[4] ) ) >= 0 ) s->plate = option;
				Garage_Select( s_garage.vehicle );
				Q_strncpyz( s_garage.status, "Web setup loaded for preview. Apply setup to use it in Q3Rally.", sizeof( s_garage.status ) );
			}
		}
	}
	trap_Cvar_VariableStringBuffer( "ui_webprofile_status", s_garage.profileStatus, sizeof( s_garage.profileStatus ) );
}

/* -------------------------------------------------------------------------
   Layout inputs
   ------------------------------------------------------------------------- */

static int Garage_PickerCount( int picker ) {
	switch ( picker ) {
	case 0: return garageCatalog.vehicles;
	case 1: return garageCatalog.vehicle[s_garage.vehicle].paints;
	case 2: return garageCatalog.wheels;
	case 3: return garageCatalog.heads;
	default: return garageCatalog.plates;
	}
}

static int Garage_PickerIndex( int picker ) {
	const garageSetup_t *s = Garage_Setup( s_garage.vehicle );
	switch ( picker ) {
	case 0: return s_garage.vehicle;
	case 1: return s->paint;
	case 2: return s->wheel;
	case 3: return s->head;
	default: return s->plate;
	}
}

/* carouselEntry(): name, lock and picture of an option */
static const char *Garage_Entry( int picker, int index, qboolean *locked, char *icon, int size ) {
	const garageVehicle_t *v;
	*locked = qfalse;
	switch ( picker ) {
	case 0: {
		/* the vehicle with its chosen paint, or its first unlocked one */
		int paint = 0, i;
		v = &garageCatalog.vehicle[index];
		if ( s_garage.setup[index].set ) paint = s_garage.setup[index].paint;
		else {
			for ( i = 0; i < v->paints; i++ ) if ( !Garage_Locked( v->paint[i].rank ) ) { paint = i; break; }
		}
		*locked = Garage_Locked( v->rank );
		if ( icon ) Garage_PaintIcon( index, paint, icon, size );
		return v->name;
	}
	case 1:
		v = &garageCatalog.vehicle[s_garage.vehicle];
		*locked = Garage_Locked( v->paint[index].rank );
		if ( icon ) Garage_PaintIcon( s_garage.vehicle, index, icon, size );
		return v->paint[index].name;
	case 2:
		if ( icon ) Garage_OptionIcon( "wheels", &garageCatalog.wheel[index], icon, size );
		return garageCatalog.wheel[index].name;
	case 3:
		if ( icon ) Garage_OptionIcon( "heads", &garageCatalog.head[index], icon, size );
		return garageCatalog.head[index].name;
	default:
		if ( icon ) Garage_OptionIcon( "plates", &garageCatalog.plate[index], icon, size );
		return garageCatalog.plate[index].name;
	}
}

static void Garage_Build( void ) {
	garageLayout_t *l = &s_garage.layout;
	const garageVehicle_t *v = &garageCatalog.vehicle[s_garage.vehicle];
	int i, slot;

	s_garage.rank = UI_Profile_UnlockedRank();
	Garage_ReadFavorites();
	Com_sprintf( s_garage.title, sizeof( s_garage.title ), "%s%s", v->name, Garage_Locked( v->rank ) ? " (Locked)" : "" );
	l->vehicle = s_garage.title;
	l->view3d = s_garage.view3d;
	l->status = s_garage.status;
	l->profileStatus = s_garage.profileStatus;
	for ( i = 0; i < GARAGE_PICKERS; i++ ) {
		garagePicker_t *p = &l->picker[i];
		int c;
		p->count = Garage_PickerCount( i );
		p->index = Garage_PickerIndex( i );
		for ( c = 0; c < 3; c++ ) {
			p->locked[c] = qfalse;
			if ( p->count ) Garage_Entry( i, Garage_Wrap( p->index + c - 1, p->count ), &p->locked[c], NULL, 0 );
		}
	}
	for ( slot = 0; slot < GARAGE_SLOTS; slot++ ) {
		garageSlot_t *s = &l->slot[slot];
		int vehicle = s_garage.favorite[slot][0], paint = s_garage.favorite[slot][1];
		s->used = s->image = vehicle >= 0;
		if ( vehicle >= 0 ) {
			const garageVehicle_t *fv = &garageCatalog.vehicle[vehicle];
			Com_sprintf( s_garage.load[slot], sizeof( s_garage.load[slot] ), "%s / %s%s", fv->name, fv->paint[paint].name,
			             Garage_Locked( fv->rank ) || Garage_Locked( fv->paint[paint].rank ) ? " (Locked)" : "" );
		} else {
			Q_strncpyz( s_garage.load[slot], "Empty", sizeof( s_garage.load[slot] ) );
		}
		s->load = s_garage.load[slot];
	}
	l->scroll = s_garage.scroll;
	Garage_Layout( l, WS_ViewportWidth(), WS_ViewportHeight() );
	s_garage.scroll = l->scroll;
}

/* -------------------------------------------------------------------------
   Drawing (the page is laid out unscrolled; dy moves it)
   ------------------------------------------------------------------------- */

static float garageDY;

static wsRect_t Garage_Moved( const wsRect_t *r ) {
	wsRect_t moved = *r;
	moved.y += garageDY;
	return moved;
}

/* an <img> with object-fit: contain, at an opacity */
static void Garage_DrawImage( const char *name, const wsRect_t *box, float alpha, qboolean shadow ) {
	qhandle_t shader = trap_R_RegisterShaderNoMip( name );
	float aspect = Offline_ImageAspect( name ), w = box->w, h = box->h, x, y;
	vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };
	if ( !shader || box->w <= 0.0f || box->h <= 0.0f ) return;
	if ( aspect <= 0.0f ) aspect = 1.0f;
	if ( w / h > aspect ) w = h * aspect;
	else h = w / aspect;
	x = box->x + ( box->w - w ) * 0.5f;
	y = box->y + garageDY + ( box->h - h ) * 0.5f;
	if ( shadow ) {
		/* filter: drop-shadow(0 1.1rem 1rem #000a): the picture's shape in
		 * black, blurred, 1.1rem down */
		static const float ring[8][2] = { { 1, 0 }, { 0.7f, 0.7f }, { 0, 1 }, { -0.7f, 0.7f }, { -1, 0 }, { -0.7f, -0.7f }, { 0, -1 }, { 0.7f, -0.7f } };
		vec4_t dark = { 0.0f, 0.0f, 0.0f, 0xaa / 255.0f / 9.0f };
		int i;
		trap_R_SetColor( dark );
		trap_R_DrawStretchPic( x, y + 1.1f * WS_REM, w, h, 0, 0, 1, 1, shader );
		for ( i = 0; i < 8; i++ ) {
			trap_R_DrawStretchPic( x + ring[i][0] * 0.5f * WS_REM, y + 1.1f * WS_REM + ring[i][1] * 0.5f * WS_REM, w, h, 0, 0, 1, 1, shader );
		}
	}
	color[3] = alpha;
	trap_R_SetColor( color );
	trap_R_DrawStretchPic( x, y, w, h, 0, 0, 1, 1, shader );
	trap_R_SetColor( NULL );
}

/* white-space: nowrap; text-overflow: ellipsis; text-align: center */
static void Garage_DrawName( const wsTextStyle_t *style, const char *text, float x, float w, float y, const float *color ) {
	char shown[GARAGE_NAME * 2 + 8];
	float width = WS_TextWidth( style, text, strlen( text ) );
	int length;
	if ( width <= w + 0.01f ) {
		WS_DrawText( style, x + ( w - width ) * 0.5f, y, text, strlen( text ), color );
		return;
	}
	for ( length = strlen( text ); length > 0; length-- ) {
		Q_strncpyz( shown, text, length + 1 );
		Q_strcat( shown, sizeof( shown ), GARAGE_ELLIPSIS );
		if ( WS_TextWidth( style, shown, strlen( shown ) ) <= w + 0.01f ) break;
	}
	if ( !length ) Q_strncpyz( shown, GARAGE_ELLIPSIS, sizeof( shown ) );
	WS_SetClipX( x, x + w );
	WS_DrawText( style, x, y, shown, strlen( shown ), color );
	WS_SetClipX( 0.0f, 0.0f );
}

static void Garage_Fade( const float *in, float alpha, vec4_t out ) {
	Vector4Copy( in, out );
	out[3] *= alpha;
}

static void Garage_DrawCard( int picker, int slot, const garageCard_t *c, int index ) {
	wsTextStyle_t name, small, mono;
	char icon[MAX_QPATH];
	qboolean locked;
	const char *text;
	vec4_t white, accent;
	float alpha = slot == 1 ? 1.0f : 0.5f;	/* .carousel-neighbor { opacity: .5 } */
	wsRect_t box = Garage_Moved( &c->box );

	if ( c->box.w <= 0.0f ) return;
	text = Garage_Entry( picker, index, &locked, icon, sizeof( icon ) );
	if ( slot == 1 ) {
		/* .carousel-current: #25201d, border-bottom 2px accent */
		float x0 = WS_Round( box.x ), y0 = WS_Round( box.y ), x1 = WS_Round( box.x + box.w ), y1 = WS_Round( box.y + box.h );
		WS_FillRect( x0, y0, x1 - x0, y1 - y0 - 2, garageCurrent );
		WS_FillRect( x0, y1 - 2, x1 - x0, 2, wsAccent );
	}
	Garage_DrawImage( icon, &c->image, alpha, qfalse );
	WS_BodyStyle( &name );
	name.size = slot == 1 ? 0.95f * WS_REM : 0.7f * WS_REM;
	name.lineHeight = WS_LU( 1.5f * name.size );
	if ( slot != 1 ) name.font = WS_BODY_BOLD;		/* button text: font-weight 750 */
	Garage_Fade( wsWhite, alpha, white );
	Garage_Fade( wsAccent, alpha, accent );
	Garage_DrawName( &name, text, c->nameX, c->nameW, c->nameY + garageDY, white );
	if ( locked && c->lockY ) {
		small = name;
		small.size = 0.65f * WS_REM;
		small.lineHeight = WS_LU( 1.5f * small.size );
		Garage_DrawName( &small, "Locked", c->nameX, c->nameW, c->lockY + garageDY, accent );
	}
	if ( slot == 1 && c->countY ) {
		char count[32];
		mono.font = WS_MONO;
		mono.size = 0.65f * WS_REM;
		mono.spacing = 0.0f;
		mono.lineHeight = 12.0f;
		mono.uppercase = qfalse;
		Com_sprintf( count, sizeof( count ), "%d / %d", index + 1, Garage_PickerCount( picker ) );
		Garage_DrawName( &mono, count, c->nameX, c->nameW, c->countY + garageDY, garageCount );
	}
}

static void Garage_DrawArrow( const wsRect_t *r, const char *glyph, qboolean disabled ) {
	wsTextStyle_t arrow = { WS_BODY_BOLD, 2.0f * WS_REM, 0.0f, 2.0f * WS_REM, qfalse };
	wsRect_t box = Garage_Moved( r );
	vec4_t color;
	Library_DrawButton( &box, "", qtrue, disabled );
	Garage_Fade( wsWhite, disabled ? 0.45f : 1.0f, color );
	WS_DrawText( &arrow, box.x + ( box.w - WS_TextWidth( &arrow, glyph, 1 ) ) * 0.5f, box.y + ( box.h - arrow.lineHeight ) * 0.5f,
	             glyph, 1, color );
}

/* a button as wide as its column, its label centred on each line */
static void Garage_DrawWide( const wsRect_t *r, const char *label, const wsLines_t *lines, qboolean secondary, qboolean disabled ) {
	wsTextStyle_t bold;
	wsRect_t box = Garage_Moved( r );
	vec4_t text;
	char shown[128];
	int i, c;
	float alpha = disabled ? 0.45f : 1.0f;
	const float *t = secondary ? wsWhite : wsButtonText;

	Library_DrawButton( &box, "", secondary, disabled );
	WS_BodyStyle( &bold );
	bold.font = WS_BODY_BOLD;
	/* the button is one group at .45 (see Library_DrawButton) */
	for ( c = 0; c < 3; c++ ) text[c] = alpha * t[c] + ( 1.0f - alpha ) * wsSurface[c];
	text[3] = 1.0f;
	WS_Transform( &bold, label, shown, sizeof( shown ) );
	for ( i = 0; i < lines->count; i++ ) {
		float w = WS_TextWidth( &bold, shown + lines->start[i], lines->length[i] );
		WS_DrawText( &bold, box.x + ( box.w - w ) * 0.5f, box.y + 1.0f + WS_LU( WS_BUTTON_PAD_Y ) + i * WS_BODY_LINE,
		             shown + lines->start[i], lines->length[i], text );
	}
}

static void Garage_DrawH2( const wsTextStyle_t *h2, float x, float y, const char *text ) {
	char shown[64];
	WS_Transform( h2, text, shown, sizeof( shown ) );
	WS_DrawText( h2, x, y, shown, strlen( shown ), wsWhite );
}

static void Garage_DrawSection( const wsRect_t *r, qboolean showroom ) {
	wsRect_t box = Garage_Moved( r );
	WS_DrawSection( &box, showroom );
}

static void Garage_Draw( void ) {
	garageLayout_t *l = &s_garage.layout;
	wsTextStyle_t body, picker, view;
	char icon[MAX_QPATH];
	float x, y;
	int i;

	Garage_LoadCatalog();
	if ( !garageCatalog.vehicles ) {
		UI_PopMenu();
		return;
	}
	Garage_WebProfile();

	/* a drag on the scrollbar or the 3D view */
	if ( s_garage.drag != GARAGE_DRAG_NONE ) {
		Library_Cursor( &x, &y );
		if ( !trap_Key_IsDown( K_MOUSE1 ) ) s_garage.drag = GARAGE_DRAG_NONE;
		else if ( s_garage.drag == GARAGE_DRAG_PAGE ) {
			float track = l->viewport.h - 2.0f * RIVALS_ARROW - 9.0f - l->pageThumb.h;
			if ( track > 0.0f ) s_garage.scroll = s_garage.dragScroll + ( y - s_garage.dragY ) * ( l->pageHeight - l->viewport.h ) / track;
		} else if ( l->image.h > 0.0f ) {
			/* OrbitControls: 2 pi per element height */
			s_garage.yaw = AngleNormalize360( s_garage.yaw + 360.0f * ( x - s_garage.dragX ) / l->image.h );
			s_garage.elevation = WS_Clamp( WS_VIEW_MIN_ELEVATION, s_garage.elevation + 360.0f * ( y - s_garage.dragY ) / l->image.h, WS_VIEW_MAX_ELEVATION );
			s_garage.dragX = x;
			s_garage.dragY = y;
		}
	}
	Garage_Build();
	garageDY = -l->scroll;

	WS_BodyStyle( &body );
	WS_DrawBackground( l->page.pageWidth, l->pageHeight );
	{
		wsFrontPage_t page = l->page;
		wsRect_t back = Garage_Moved( &l->back );
		page.headerY += garageDY;
		page.h1Y += garageDY;
		page.statusY += garageDY;
		WS_DrawFrontHeader( &page, "Your ", "garage.", "" );
		WS_DrawButton( &back, "Back to game menu", qtrue, qfalse );
	}

	/* 01 / SHOWROOM */
	Garage_DrawSection( &l->showroom, qtrue );
	WS_DrawLabel( l->showroomX, l->showroomLabelY + garageDY, "01 / SHOWROOM", NULL );
	WS_DrawLines( &l->h2, l->showroomX, l->vehicleY + garageDY, l->vehicle, &l->vehicleLines, wsWhite );
	if ( s_garage.view3d ) {
		wsRect_t view = Garage_Moved( &l->image );
		Garage_UpdateCar();
		UI_DrawVehicleWebView( view.x, view.y, view.w, view.h, &s_garage.car, uis.realtime, s_garage.yaw, s_garage.elevation, s_garage.distance );
	} else {
		Garage_PaintIcon( s_garage.vehicle, Garage_Setup( s_garage.vehicle )->paint, icon, sizeof( icon ) );
		Garage_DrawImage( icon, &l->image, 1.0f, qtrue );
	}
	{
		wsRect_t toggle = Garage_Moved( &l->toggle );
		WS_DrawButton( &toggle, s_garage.view3d ? "Photo view" : "3D view", qtrue, qfalse );
		if ( l->reset.w > 0.0f ) {
			wsRect_t reset = Garage_Moved( &l->reset );
			WS_DrawButton( &reset, "Reset view", qtrue, qfalse );
		}
		if ( s_garage.view3d ) {
			view = body;
			view.size = 0.85f * WS_REM;
			view.lineHeight = WS_LU( 1.5f * view.size );
			/* align-items: center in the button row */
			WS_DrawText( &view, l->viewStatusX, l->viewStatusY + garageDY + ( WS_BUTTON_HEIGHT - view.lineHeight ) * 0.5f,
			             "Drag to rotate \xb7 scroll to zoom", 31, garageMuted );
		}
	}

	/* 02 / CONFIGURATION */
	Garage_DrawSection( &l->config, qfalse );
	WS_DrawLabel( l->configX, l->configLabelY + garageDY, "02 / CONFIGURATION", NULL );
	Garage_DrawH2( &l->h2, l->configX, l->configH2Y + garageDY, "Make it yours" );
	picker = body;
	picker.size = 0.9f * WS_REM;
	picker.lineHeight = WS_LU( 1.5f * picker.size );
	for ( i = 0; i < GARAGE_PICKERS; i++ ) {
		garagePicker_t *p = &l->picker[i];
		qboolean disabled = p->count < 2;
		int c;
		WS_DrawText( &picker, l->configX, p->labelY + garageDY, garagePickerLabel[i], strlen( garagePickerLabel[i] ), garageMuted );
		Garage_DrawArrow( &p->previous, GARAGE_ARROW_LEFT, disabled );
		Garage_DrawArrow( &p->next, GARAGE_ARROW_RIGHT, disabled );
		for ( c = 0; c < 3; c++ ) {
			if ( p->count ) Garage_DrawCard( i, c, &p->card[c], Garage_Wrap( p->index + c - 1, p->count ) );
		}
	}
	{
		wsRect_t apply = Garage_Moved( &l->apply ), link = Garage_Moved( &l->link ), pull = Garage_Moved( &l->pull );
		Library_DrawButton( &apply, "Apply setup", qfalse, Garage_SetupLocked() );
		WS_DrawButton( &link, "Link web profile", qtrue, qfalse );
		WS_DrawButton( &pull, "Load web setup and favorites", qtrue, qfalse );
	}
	WS_DrawLines( &body, l->configX, l->profileStatusY + garageDY, l->profileStatus, &l->profileStatusLines, wsWhite );
	WS_DrawLines( &body, l->configX, l->statusY + garageDY, l->status, &l->statusLines, garageMuted );

	/* 03 / SAVED SETUPS */
	Garage_DrawSection( &l->favorites, qfalse );
	WS_DrawLabel( l->favoritesX, l->favoritesLabelY + garageDY, "03 / SAVED SETUPS", NULL );
	Garage_DrawH2( &l->h2, l->favoritesX, l->favoritesH2Y + garageDY, "Your favorites" );
	WS_DrawLines( &body, l->favoritesX, l->favoritesTextY + garageDY, "Preview a saved setup or save your current selection in one of four slots.",
	              &l->favoritesTextLines, wsWhite );
	for ( i = 0; i < GARAGE_SLOTS; i++ ) {
		garageSlot_t *s = &l->slot[i];
		wsLines_t one;
		char label[16];
		Com_sprintf( label, sizeof( label ), "Slot %d", i + 1 );
		WS_DrawText( &body, s->box.x, s->labelY + garageDY, label, strlen( label ), wsWhite );
		if ( s->image ) {
			Garage_PaintIcon( s_garage.favorite[i][0], s_garage.favorite[i][1], icon, sizeof( icon ) );
			Garage_DrawImage( icon, &s->picture, 1.0f, qfalse );
		}
		Garage_DrawWide( &s->loadButton, s->load, &s->loadLines, qtrue, !s->used );
		WS_StyledWrap( &body, "Save here", 1.0e6f, &one );
		Garage_DrawWide( &s->save, "Save here", &one, qfalse, Garage_SetupLocked() );
		Garage_DrawWide( &s->upload, "Upload to web profile", &s->uploadLines, qtrue, !s->used );
	}

	if ( l->pageBar ) WS_DrawScrollbar( &l->viewport, &l->pageThumb );
}

/* -------------------------------------------------------------------------
   Input
   ------------------------------------------------------------------------- */

static qboolean Garage_Inside( const wsRect_t *r, float x, float y ) {
	wsRect_t moved = Garage_Moved( r );
	return Library_Inside( &moved, x, y );
}

static void Garage_Move( int picker, int offset ) {
	int count = Garage_PickerCount( picker ), index;
	garageSetup_t *s = Garage_Setup( s_garage.vehicle );
	if ( count < 2 ) return;
	index = Garage_Wrap( Garage_PickerIndex( picker ) + offset, count );
	switch ( picker ) {
	case 0: Garage_Select( index ); return;
	case 1: s->paint = index; break;
	case 2: s->wheel = index; break;
	case 3: s->head = index; break;
	default: s->plate = index; break;
	}
	Garage_RefreshStatus();
	Garage_UpdateCar();
}

static void Garage_Click( float x, float y ) {
	garageLayout_t *l = &s_garage.layout;
	int i;

	if ( l->pageBar && x >= l->viewport.w - RIVALS_SCROLLBAR ) {
		if ( y < RIVALS_ARROW ) s_garage.scroll -= 40.0f;
		else if ( y >= l->viewport.h - RIVALS_ARROW ) s_garage.scroll += 40.0f;
		else if ( y < l->pageThumb.y ) s_garage.scroll -= 0.875f * l->viewport.h;
		else if ( y >= l->pageThumb.y + l->pageThumb.h ) s_garage.scroll += 0.875f * l->viewport.h;
		else {
			s_garage.drag = GARAGE_DRAG_PAGE;
			s_garage.dragY = y;
			s_garage.dragScroll = s_garage.scroll;
		}
		return;
	}
	if ( Garage_Inside( &l->back, x, y ) ) { UI_PopMenu(); return; }
	if ( Garage_Inside( &l->toggle, x, y ) ) {
		s_garage.view3d = !s_garage.view3d;
		s_garage.carModel[0] = '\0';
		s_garage.yaw = WS_VIEW_AZIMUTH;
		s_garage.elevation = WS_VIEW_ELEVATION;
		s_garage.distance = 2.1f;		/* garage.js: distance 2.1 */
		Garage_UpdateCar();
		return;
	}
	if ( l->reset.w > 0.0f && Garage_Inside( &l->reset, x, y ) ) {
		s_garage.yaw = WS_VIEW_AZIMUTH;
		s_garage.elevation = WS_VIEW_ELEVATION;
		s_garage.distance = 2.1f;
		return;
	}
	if ( s_garage.view3d && Garage_Inside( &l->image, x, y ) ) {
		s_garage.drag = GARAGE_DRAG_VIEW;
		s_garage.dragX = x;
		s_garage.dragY = y;
		return;
	}
	for ( i = 0; i < GARAGE_PICKERS; i++ ) {
		garagePicker_t *p = &l->picker[i];
		if ( Garage_Inside( &p->previous, x, y ) || Garage_Inside( &p->card[0].box, x, y ) ) { Garage_Move( i, -1 ); return; }
		if ( Garage_Inside( &p->next, x, y ) || Garage_Inside( &p->card[2].box, x, y ) ) { Garage_Move( i, 1 ); return; }
	}
	if ( Garage_Inside( &l->apply, x, y ) ) { Garage_Submit( -1 ); return; }
	if ( Garage_Inside( &l->link, x, y ) ) { trap_Cvar_Set( "ui_webprofile_action", "link" ); return; }
	if ( Garage_Inside( &l->pull, x, y ) ) {
		const char *key = garageCatalog.vehicle[s_garage.vehicle].key;
		if ( Garage_FileExists( va( "models/players/%s/body.md3", key ) ) ) trap_Cvar_Set( "ui_webprofile_action", va( "sync:%s", key ) );
		return;
	}
	for ( i = 0; i < GARAGE_SLOTS; i++ ) {
		garageSlot_t *s = &l->slot[i];
		if ( Garage_Inside( &s->loadButton, x, y ) ) { Garage_LoadFavorite( i ); return; }
		if ( Garage_Inside( &s->save, x, y ) ) { Garage_Submit( i ); return; }
		if ( Garage_Inside( &s->upload, x, y ) ) { Garage_Upload( i ); return; }
	}
}

static sfxHandle_t Garage_Key( int key ) {
	garageLayout_t *l = &s_garage.layout;
	float x, y;

	Library_Cursor( &x, &y );
	switch ( key ) {
	case K_ESCAPE:
	case K_PAD0_B:
	case K_MOUSE2:
		UI_PopMenu();
		return 0;
	case K_MOUSE1:
		Garage_Click( x, y );
		return 0;
	case K_MWHEELUP:
	case K_MWHEELDOWN:
		if ( s_garage.view3d && Garage_Inside( &l->image, x, y ) ) {
			s_garage.distance = WS_Clamp( WS_VIEW_MIN_DISTANCE, s_garage.distance * ( key == K_MWHEELUP ? 0.95f : 1.0f / 0.95f ), WS_VIEW_MAX_DISTANCE );
		} else {
			s_garage.scroll += key == K_MWHEELUP ? -100.0f : 100.0f;
		}
		return 0;
	case K_UPARROW:
	case K_PGUP:
		s_garage.scroll -= key == K_PGUP ? 0.875f * l->viewport.h : 40.0f;
		return 0;
	case K_DOWNARROW:
	case K_PGDN:
		s_garage.scroll += key == K_PGDN ? 0.875f * l->viewport.h : 40.0f;
		return 0;
	}
	return 0;
}

/* -------------------------------------------------------------------------
   Entry
   ------------------------------------------------------------------------- */

qboolean UI_WebGarageNative( void ) {
	return uis.activemenu == &s_garage.menu;
}

void UI_RallyGarageMenu( void ) {
	memset( &s_garage, 0, sizeof( s_garage ) );
	s_garage.menu.fullscreen = qtrue;
	s_garage.menu.draw = Garage_Draw;
	s_garage.menu.key = Garage_Key;
	s_garage.yaw = WS_VIEW_AZIMUTH;
	s_garage.elevation = WS_VIEW_ELEVATION;
	s_garage.distance = 2.1f;
	/* unlike the page, the garage opens on the 3D view; "Photo view" shows
	 * the paint's picture */
	s_garage.view3d = qtrue;
	Garage_LoadCatalog();
	if ( garageCatalog.vehicles ) Garage_Initial();
	UI_PushMenu( &s_garage.menu );
}
