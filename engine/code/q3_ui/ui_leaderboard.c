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

#define ID_BACK			10
#define ID_MAPLIST		11

typedef struct {
	char name[MAX_NETNAME];
	int time;
	char car[64];
} leaderboard_t;

typedef struct {
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	back;
	menulist_s		maplist;
	leaderboard_t	leaderboard[10];
	char			mapname[MAX_QPATH];
} leaderboardmenu_t;

static leaderboardmenu_t s_leaderboard;

static void UI_Leaderboard_Event( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_BACK:
		UI_PopMenu();
		break;

	case ID_MAPLIST:
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "getbesttimes %s", s_leaderboard.maplist.itemnames[s_leaderboard.maplist.curvalue] ) );
		break;
	}
}

static void UI_Leaderboard_Draw( void ) {
	int i;
	int y;
	char buffer[1024];

	UI_DrawHandlePic( 0, 0, 640, 480, uis.menuBackShader );
	UI_DrawNamedPic( 320-233, 10, 466, 80, "menu/art/banner_q3r" );

	UI_DrawString( 320, 100, "LEADERBOARDS", UI_CENTER|UI_BIGFONT, text_color_normal );

	y = 160;
	for ( i = 0; i < 10; i++ ) {
		if ( s_leaderboard.leaderboard[i].time == 0 ) {
			break;
		}

		Com_sprintf( buffer, sizeof(buffer), "%2i. %-16s %s %s", i + 1, s_leaderboard.leaderboard[i].name, UI_TimeString( s_leaderboard.leaderboard[i].time ), s_leaderboard.leaderboard[i].car );
		UI_DrawString( 100, y, buffer, UI_LEFT|UI_SMALLFONT, text_color_normal );
		y += 20;
	}

	Menu_Draw( &s_leaderboard.menu );
}

void UI_LeaderboardMenu_f( void ) {
	char *token;
	int i;

	memset(&s_leaderboard.leaderboard, 0, sizeof(s_leaderboard.leaderboard));

	i = 0;
	while ( (token = UI_Argv( i * 3 + 1 )) && *token && i < 10 ) {
		Q_strncpyz(s_leaderboard.leaderboard[i].name, token, sizeof(s_leaderboard.leaderboard[i].name));

		token = UI_Argv( i * 3 + 2 );
		if (!token || !*token) break;
		s_leaderboard.leaderboard[i].time = atoi(token);

		token = UI_Argv( i * 3 + 3 );
		if (!token || !*token) break;
		Q_strncpyz(s_leaderboard.leaderboard[i].car, token, sizeof(s_leaderboard.leaderboard[i].car));

		i++;
	}
}

void UI_LeaderboardMenu( void ) {
	int i;
	static char *mapnames[256];
	static char mapinfos[256][MAX_INFO_STRING];

	memset( &s_leaderboard, 0, sizeof(s_leaderboard) );

	s_leaderboard.menu.draw = UI_Leaderboard_Draw;
	s_leaderboard.menu.fullscreen = qtrue;
	s_leaderboard.menu.wrapAround = qtrue;
	s_leaderboard.menu.showlogo = qtrue;

	s_leaderboard.banner.generic.type = MTYPE_BTEXT;
	s_leaderboard.banner.generic.x = 320;
	s_leaderboard.banner.generic.y = 16;
	s_leaderboard.banner.string = "LEADERBOARDS";
	s_leaderboard.banner.color = text_color_normal;
	s_leaderboard.banner.style = UI_CENTER;

	s_leaderboard.back.generic.type = MTYPE_BITMAP;
	s_leaderboard.back.generic.name = "menu/art/back_0";
	s_leaderboard.back.generic.flags = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_leaderboard.back.generic.id = ID_BACK;
	s_leaderboard.back.generic.callback = UI_Leaderboard_Event;
	s_leaderboard.back.generic.x = 10;
	s_leaderboard.back.generic.y = 10;
	s_leaderboard.back.width = 96;
	s_leaderboard.back.height = 32;
	s_leaderboard.back.focuspic = "menu/art/back_1";

	s_leaderboard.maplist.generic.type = MTYPE_LISTBOX;
	s_leaderboard.maplist.generic.flags = QMF_PULSEIFFOCUS;
	s_leaderboard.maplist.generic.id = ID_MAPLIST;
	s_leaderboard.maplist.generic.callback = UI_Leaderboard_Event;
	s_leaderboard.maplist.generic.x = 400;
	s_leaderboard.maplist.generic.y = 160;
	s_leaderboard.maplist.numitems = trap_FS_GetFileList( "maps", ".bsp", (char *)mapinfos, 256 * MAX_INFO_STRING );
	s_leaderboard.maplist.itemnames = (const char **)mapnames;
	s_leaderboard.maplist.width = 16;
	s_leaderboard.maplist.height = 10;

	for (i = 0; i < s_leaderboard.maplist.numitems; i++) {
		mapnames[i] = mapinfos[i];
		COM_StripExtension(mapnames[i], mapnames[i], MAX_QPATH);
	}

	Menu_AddItem( &s_leaderboard.menu, &s_leaderboard.banner );
	Menu_AddItem( &s_leaderboard.menu, &s_leaderboard.back );
	Menu_AddItem( &s_leaderboard.menu, &s_leaderboard.maplist );

	trap_Cmd_ExecuteText( EXEC_APPEND, va( "getbesttimes %s", s_leaderboard.maplist.itemnames[s_leaderboard.maplist.curvalue] ) );

	UI_PushMenu( &s_leaderboard.menu );
}
