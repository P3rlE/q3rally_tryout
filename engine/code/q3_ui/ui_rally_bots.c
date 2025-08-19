/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2025 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "ui_local.h"

#define BOT_SHOWCASE_BANNER "THE GARAGE"

#define ID_BOTLIST      100
#define ID_BACK         101
#define ID_LEFT_ARROW   102
#define ID_RIGHT_ARROW  103


typedef struct {
	char name[MAX_NAME_LENGTH];
	char personality[MAX_INFO_STRING];
	char description[MAX_INFO_STRING];
	char model[MAX_QPATH];
} botinfo_t;

typedef struct {
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	back;
	menulist_s		botlist;
	botinfo_t		botinfo[MAX_BOTS];
	int				numBots;
	char*			botnames[MAX_BOTS];

	playerInfo_t	playerinfo;
	menubitmap_s	botpic;
	menutext_s		name;
	menutext_s		personality;
	menutext_s		description;

	menubitmap_s	leftArrow;
	menubitmap_s	rightArrow;
} bots_t;

static bots_t s_bots;

/*
=================
UI_Bots_ParseBots
=================
*/
static void UI_Bots_ParseBots( void ) {
	static qboolean parsed = qfalse;
	char *buf;
	char *token;
	char *p;
	int len;
	fileHandle_t f;
	botinfo_t *bot;

	if (parsed) {
		return;
	}
	parsed = qtrue;

	len = trap_FS_FOpenFile( "scripts/bots.txt", &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "file not found: %s\n", "scripts/bots.txt" ) );
		return;
	}
	if ( !len ) {
		trap_Print( va( S_COLOR_RED "file is empty: %s\n", "scripts/bots.txt" ) );
		trap_FS_FCloseFile( f );
		return;
	}

	buf = UI_Alloc( len + 1 );
	trap_FS_Read( buf, len, f );
	trap_FS_FCloseFile( f );

	p = buf;
	s_bots.numBots = 0;

	while ( p ) {
		token = COM_Parse( &p );
		if ( !token[0] ) {
			break;
		}

		if ( Q_stricmp( token, "{" ) ) {
			trap_Print( va( S_COLOR_RED "unexpected token '%s' in %s\n", token, "scripts/bots.txt" ) );
			break;
		}

		if ( s_bots.numBots >= MAX_BOTS ) {
			trap_Print( va( S_COLOR_RED "MAX_BOTS exceeded in %s\n", "scripts/bots.txt" ) );
			break;
		}

		bot = &s_bots.botinfo[s_bots.numBots];

		while ( 1 ) {
			token = COM_ParseExt( &p, qfalse );
			if ( !token[0] ) {
				break;
			}

			if ( Q_stricmp( token, "}" ) == 0 ) {
				break;
			}

			if ( Q_stricmp( token, "name" ) == 0 ) {
				token = COM_ParseExt( &p, qfalse );
				Q_strncpyz( bot->name, token, sizeof( bot->name ) );
			} else if ( Q_stricmp( token, "personality" ) == 0 ) {
				token = COM_ParseExt( &p, qfalse );
				Q_strncpyz( bot->personality, token, sizeof( bot->personality ) );
			} else if ( Q_stricmp( token, "description" ) == 0 ) {
				token = COM_ParseExt( &p, qfalse );
				Q_strncpyz( bot->description, token, sizeof( bot->description ) );
			} else if ( Q_stricmp( token, "model" ) == 0 ) {
				token = COM_ParseExt( &p, qfalse );
				Q_strncpyz( bot->model, token, sizeof( bot->model ) );
			}
		}
		s_bots.botnames[s_bots.numBots] = s_bots.botinfo[s_bots.numBots].name;
		s_bots.numBots++;
	}

	// trap_Memory_Free( buf );
}

/*
=================
UI_Bots_Menu_Draw
=================
*/
static void UI_Bots_Menu_Draw( void ) {
	UI_DrawBannerString( 320, 16, BOT_SHOWCASE_BANNER, UI_CENTER, color_white );
}

/*
=================
UI_Bots_Menu_Key
=================
*/
static sfxHandle_t UI_Bots_Menu_Key( int key ) {
	if ( key == K_ESCAPE ) {
		UI_PopMenu();
		return menu_out_sound;
	}
	return menu_out_sound;
}

/*
=================
UI_Bots_Update
=================
*/
static void UI_Bots_Update( void ) {
	botinfo_t *bot;
	vec3_t	viewangles;
	vec3_t	moveangles;

	bot = &s_bots.botinfo[s_bots.botlist.curvalue];

	s_bots.name.string = bot->name;
	s_bots.personality.string = bot->personality;
	s_bots.description.string = bot->description;

	VectorClear( viewangles );
	VectorClear( moveangles );

	UI_PlayerInfo_SetModel( &s_bots.playerinfo, bot->model, "default", "default", "plate" );
	UI_PlayerInfo_SetInfo( &s_bots.playerinfo, LEGS_IDLE, TORSO_STAND, viewangles, moveangles, WP_NONE, qfalse );
}

/*
=================
UI_Bots_Menu_DrawPlayer
=================
*/
static void UI_Bots_Menu_DrawPlayer( void *self ) {
	menubitmap_s *b;

	b = (menubitmap_s*) self;
	UI_DrawPlayer( b->generic.x, b->generic.y, b->width, b->height, &s_bots.playerinfo, uis.realtime );
}

/*
=================
UI_Bots_Menu_Event
=================
*/
static void UI_Bots_Menu_Event( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
		case ID_BOTLIST:
			UI_Bots_Update();
			break;
		case ID_LEFT_ARROW:
			if (s_bots.botlist.curvalue > 0) {
				s_bots.botlist.curvalue--;
				UI_Bots_Update();
			}
			break;
		case ID_RIGHT_ARROW:
			if (s_bots.botlist.curvalue < s_bots.numBots - 1) {
				s_bots.botlist.curvalue++;
				UI_Bots_Update();
			}
			break;
		case ID_BACK:
			UI_PopMenu();
			break;
	}
}

/*
=================
UI_BotsMenu_Init
=================
*/
static void UI_BotsMenu_Init( void ) {
	int i;
	memset( &s_bots, 0, sizeof(s_bots) );
	s_bots.menu.wrapAround = qtrue;
	s_bots.menu.fullscreen = qtrue;
	s_bots.menu.draw = UI_Bots_Menu_Draw;
	s_bots.menu.key = UI_Bots_Menu_Key;

	s_bots.banner.generic.type	= MTYPE_BTEXT;
	s_bots.banner.generic.x		= 320;
	s_bots.banner.generic.y		= 16;
	s_bots.banner.string		= BOT_SHOWCASE_BANNER;
	s_bots.banner.color			= color_white;
	s_bots.banner.style			= UI_CENTER;

	s_bots.back.generic.type		= MTYPE_BITMAP;
	s_bots.back.generic.name		= "menu/art/back_0";
	s_bots.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_bots.back.generic.x			= 0;
	s_bots.back.generic.y			= 480-64;
	s_bots.back.width				= 128;
	s_bots.back.height				= 64;
	s_bots.back.focuspic			= "menu/art/back_1";
	s_bots.back.generic.id			= ID_BACK;
	s_bots.back.generic.callback	= UI_Bots_Menu_Event;

	UI_Bots_ParseBots();

	s_bots.botlist.generic.type    = MTYPE_LISTBOX;
	s_bots.botlist.generic.flags   = QMF_HIGHLIGHT_IF_FOCUS;
	s_bots.botlist.generic.x       = 50;
	s_bots.botlist.generic.y       = 100;
	s_bots.botlist.generic.callback= UI_Bots_Menu_Event;
	s_bots.botlist.generic.id      = ID_BOTLIST;
	s_bots.botlist.width           = 200;
	s_bots.botlist.height          = 200;
	s_bots.botlist.numitems        = s_bots.numBots;
	s_bots.botlist.itemnames       = (const char **)s_bots.botnames;

	s_bots.botpic.generic.type      = MTYPE_BITMAP;
	s_bots.botpic.generic.flags     = QMF_INACTIVE;
	s_bots.botpic.generic.ownerdraw = UI_Bots_Menu_DrawPlayer;
	s_bots.botpic.generic.x         = 320;
	s_bots.botpic.generic.y         = 100;
	s_bots.botpic.width             = 256;
	s_bots.botpic.height            = 256;

	s_bots.name.generic.type = MTYPE_PTEXT;
	s_bots.name.generic.flags = QMF_LEFT_JUSTIFY;
	s_bots.name.generic.x = 320;
	s_bots.name.generic.y = 360;
	s_bots.name.style = UI_BIGFONT;
	s_bots.name.color = color_white;

	s_bots.personality.generic.type = MTYPE_PTEXT;
	s_bots.personality.generic.flags = QMF_LEFT_JUSTIFY;
	s_bots.personality.generic.x = 320;
	s_bots.personality.generic.y = 380;
	s_bots.personality.style = UI_SMALLFONT;
	s_bots.personality.color = color_white;

	s_bots.description.generic.type = MTYPE_PTEXT;
	s_bots.description.generic.flags = QMF_LEFT_JUSTIFY;
	s_bots.description.generic.x = 320;
	s_bots.description.generic.y = 400;
	s_bots.description.style = UI_SMALLFONT;
	s_bots.description.color = color_white;

	Menu_AddItem( &s_bots.menu, &s_bots.banner );
	Menu_AddItem( &s_bots.menu, &s_bots.back );
	s_bots.leftArrow.generic.type		= MTYPE_BITMAP;
	s_bots.leftArrow.generic.name		= "menu/art/arrow_l0";
	s_bots.leftArrow.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_bots.leftArrow.generic.x			= 280;
	s_bots.leftArrow.generic.y			= 220;
	s_bots.leftArrow.width				= 32;
	s_bots.leftArrow.height				= 32;
	s_bots.leftArrow.focuspic			= "menu/art/arrow_l1";
	s_bots.leftArrow.generic.id			= ID_LEFT_ARROW;
	s_bots.leftArrow.generic.callback	= UI_Bots_Menu_Event;

	s_bots.rightArrow.generic.type		= MTYPE_BITMAP;
	s_bots.rightArrow.generic.name		= "menu/art/arrow_r0";
	s_bots.rightArrow.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_bots.rightArrow.generic.x			= 580;
	s_bots.rightArrow.generic.y			= 220;
	s_bots.rightArrow.width				= 32;
	s_bots.rightArrow.height			= 32;
	s_bots.rightArrow.focuspic			= "menu/art/arrow_r1";
	s_bots.rightArrow.generic.id		= ID_RIGHT_ARROW;
	s_bots.rightArrow.generic.callback	= UI_Bots_Menu_Event;

	Menu_AddItem( &s_bots.menu, &s_bots.botlist );
	Menu_AddItem( &s_bots.menu, &s_bots.botpic );
	Menu_AddItem( &s_bots.menu, &s_bots.name );
	Menu_AddItem( &s_bots.menu, &s_bots.personality );
	Menu_AddItem( &s_bots.menu, &s_bots.description );
	Menu_AddItem( &s_bots.menu, &s_bots.leftArrow );
	Menu_AddItem( &s_bots.menu, &s_bots.rightArrow );

	UI_Bots_Update();
}

/*
=================
UI_BotsMenu
=================
*/
void UI_BotsMenu( void ) {
	UI_BotsMenu_Init();
	UI_PushMenu( &s_bots.menu );
}
