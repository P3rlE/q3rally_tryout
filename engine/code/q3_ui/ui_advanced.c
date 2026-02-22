/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)
===========================================================================
*/

#include "ui_local.h"

#define ID_SUNSHADOWS       100
#define ID_SHADOWQUALITY    101
#define ID_SUNLIGHTMODE     102
#define ID_SSAO             103
#define ID_HDR              104
#define ID_NORMALMAPPING    105
#define ID_BACK             106
#define ID_APPLY            107

typedef struct {
	menuframework_s menu;
	menutext_s      banner;
	menulist_s      sunshadows;
	menulist_s      shadowquality;
	menulist_s      sunlightmode;
	menulist_s      ssao;
	menulist_s      hdr;
	menulist_s      normalmapping;
	menutext_s      back;
	menutext_s      apply;
} advancedGraphicsInfo_t;

typedef struct {
	int sunshadows;
	int shadowquality;
	int sunlightmode;
	int ssao;
	int hdr;
	int normalmapping;
} initialAdvancedGraphics_t;

static advancedGraphicsInfo_t s_adv;
static initialAdvancedGraphics_t s_initial;

static void AdvancedGraphics_GetInitial( void )
{
	s_initial.sunshadows = s_adv.sunshadows.curvalue;
	s_initial.shadowquality = s_adv.shadowquality.curvalue;
	s_initial.sunlightmode = s_adv.sunlightmode.curvalue;
	s_initial.ssao = s_adv.ssao.curvalue;
	s_initial.hdr = s_adv.hdr.curvalue;
	s_initial.normalmapping = s_adv.normalmapping.curvalue;
}

static void AdvancedGraphics_Update( void )
{
	s_adv.apply.generic.flags |= QMF_HIDDEN | QMF_INACTIVE;

	if (s_adv.sunshadows.curvalue == 0)
		s_adv.shadowquality.generic.flags |= QMF_GRAYED;
	else
		s_adv.shadowquality.generic.flags &= ~QMF_GRAYED;

	if (s_initial.sunshadows != s_adv.sunshadows.curvalue ||
		s_initial.shadowquality != s_adv.shadowquality.curvalue ||
		s_initial.sunlightmode != s_adv.sunlightmode.curvalue ||
		s_initial.ssao != s_adv.ssao.curvalue ||
		s_initial.hdr != s_adv.hdr.curvalue ||
		s_initial.normalmapping != s_adv.normalmapping.curvalue)
	{
		s_adv.apply.generic.flags &= ~(QMF_HIDDEN | QMF_INACTIVE);
	}
}

static void AdvancedGraphics_Apply( void )
{
	static const int shadowFilterValues[] = { 0, 1, 2 };
	static const int shadowMapSizeValues[] = { 512, 1024, 2048 };
	int idx = s_adv.shadowquality.curvalue;

	if (idx < 0) idx = 0;
	if (idx > 2) idx = 2;

	trap_Cvar_SetValue( "r_sunShadows", s_adv.sunshadows.curvalue );
	trap_Cvar_SetValue( "r_sunlightMode", s_adv.sunlightmode.curvalue );
	trap_Cvar_SetValue( "r_ssao", s_adv.ssao.curvalue );
	trap_Cvar_SetValue( "r_hdr", s_adv.hdr.curvalue );
	trap_Cvar_SetValue( "r_normalMapping", s_adv.normalmapping.curvalue );

	if (s_adv.sunshadows.curvalue)
	{
		trap_Cvar_SetValue( "r_shadowFilter", shadowFilterValues[idx] );
		trap_Cvar_SetValue( "r_shadowMapSize", shadowMapSizeValues[idx] );
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
	AdvancedGraphics_GetInitial();
	AdvancedGraphics_Update();
}

static void AdvancedGraphics_Event( void *ptr, int event )
{
	if (event != QM_ACTIVATED)
		return;

	switch (((menucommon_s *)ptr)->id)
	{
	case ID_BACK:
		UI_PopMenu();
		break;
	case ID_APPLY:
		AdvancedGraphics_Apply();
		break;
	default:
		AdvancedGraphics_Update();
		break;
	}
}

static void AdvancedGraphics_Draw( void )
{
	AdvancedGraphics_Update();
	Menu_Draw( &s_adv.menu );
}

static void AdvancedGraphics_SetMenuItems( void )
{
	s_adv.sunshadows.curvalue = trap_Cvar_VariableValue( "r_sunShadows" ) != 0;
	{
		int shadowMapSize = (int)trap_Cvar_VariableValue( "r_shadowMapSize" );
		if (shadowMapSize >= 2048)
			s_adv.shadowquality.curvalue = 2;
		else if (shadowMapSize >= 1024)
			s_adv.shadowquality.curvalue = 1;
		else
			s_adv.shadowquality.curvalue = 0;
	}
	s_adv.sunlightmode.curvalue = (int)trap_Cvar_VariableValue( "r_sunlightMode" );
	if (s_adv.sunlightmode.curvalue < 0) s_adv.sunlightmode.curvalue = 0;
	if (s_adv.sunlightmode.curvalue > 2) s_adv.sunlightmode.curvalue = 2;
	s_adv.ssao.curvalue = trap_Cvar_VariableValue( "r_ssao" ) != 0;
	s_adv.hdr.curvalue = trap_Cvar_VariableValue( "r_hdr" ) != 0;
	s_adv.normalmapping.curvalue = trap_Cvar_VariableValue( "r_normalMapping" ) != 0;
}

void UI_AdvancedGraphicsMenu_Cache( void )
{
}

static void UI_AdvancedGraphicsMenu_Init( void )
{
	int y;
	static const char *enabled_names[] = { "Off", "On", NULL };
	static const char *quality_names[] = { "Low", "Medium", "High", NULL };
	static const char *sunlight_names[] = { "Off", "Multiply", "Add", NULL };

	memset( &s_adv, 0, sizeof(s_adv) );
	UI_AdvancedGraphicsMenu_Cache();

	s_adv.menu.wrapAround = qtrue;
	s_adv.menu.fullscreen = qtrue;
	s_adv.menu.draw = AdvancedGraphics_Draw;

	s_adv.banner.generic.type = MTYPE_BTEXT;
	s_adv.banner.generic.x = 320;
	s_adv.banner.generic.y = 16;
	s_adv.banner.string = "ADVANCED GRAPHICS";
	s_adv.banner.color = color_white;
	s_adv.banner.style = UI_CENTER;

	y = 240 - 4 * (BIGCHAR_HEIGHT + 2);

	s_adv.sunshadows.generic.type = MTYPE_SPINCONTROL;
	s_adv.sunshadows.generic.name = "Sun Shadows:";
	s_adv.sunshadows.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_adv.sunshadows.generic.callback = AdvancedGraphics_Event;
	s_adv.sunshadows.generic.id = ID_SUNSHADOWS;
	s_adv.sunshadows.generic.x = 400;
	s_adv.sunshadows.generic.y = y;
	s_adv.sunshadows.itemnames = enabled_names;
	y += BIGCHAR_HEIGHT + 2;

	s_adv.shadowquality.generic.type = MTYPE_SPINCONTROL;
	s_adv.shadowquality.generic.name = "Shadow Quality:";
	s_adv.shadowquality.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_adv.shadowquality.generic.callback = AdvancedGraphics_Event;
	s_adv.shadowquality.generic.id = ID_SHADOWQUALITY;
	s_adv.shadowquality.generic.x = 400;
	s_adv.shadowquality.generic.y = y;
	s_adv.shadowquality.itemnames = quality_names;
	y += BIGCHAR_HEIGHT + 2;

	s_adv.sunlightmode.generic.type = MTYPE_SPINCONTROL;
	s_adv.sunlightmode.generic.name = "Sunlight Mode:";
	s_adv.sunlightmode.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_adv.sunlightmode.generic.callback = AdvancedGraphics_Event;
	s_adv.sunlightmode.generic.id = ID_SUNLIGHTMODE;
	s_adv.sunlightmode.generic.x = 400;
	s_adv.sunlightmode.generic.y = y;
	s_adv.sunlightmode.itemnames = sunlight_names;
	y += BIGCHAR_HEIGHT + 2;

	s_adv.ssao.generic.type = MTYPE_SPINCONTROL;
	s_adv.ssao.generic.name = "SSAO:";
	s_adv.ssao.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_adv.ssao.generic.callback = AdvancedGraphics_Event;
	s_adv.ssao.generic.id = ID_SSAO;
	s_adv.ssao.generic.x = 400;
	s_adv.ssao.generic.y = y;
	s_adv.ssao.itemnames = enabled_names;
	y += BIGCHAR_HEIGHT + 2;

	s_adv.hdr.generic.type = MTYPE_SPINCONTROL;
	s_adv.hdr.generic.name = "HDR:";
	s_adv.hdr.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_adv.hdr.generic.callback = AdvancedGraphics_Event;
	s_adv.hdr.generic.id = ID_HDR;
	s_adv.hdr.generic.x = 400;
	s_adv.hdr.generic.y = y;
	s_adv.hdr.itemnames = enabled_names;
	y += BIGCHAR_HEIGHT + 2;

	s_adv.normalmapping.generic.type = MTYPE_SPINCONTROL;
	s_adv.normalmapping.generic.name = "Normal Mapping:";
	s_adv.normalmapping.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_adv.normalmapping.generic.callback = AdvancedGraphics_Event;
	s_adv.normalmapping.generic.id = ID_NORMALMAPPING;
	s_adv.normalmapping.generic.x = 400;
	s_adv.normalmapping.generic.y = y;
	s_adv.normalmapping.itemnames = enabled_names;

	s_adv.back.generic.type = MTYPE_PTEXT;
	s_adv.back.generic.flags = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_adv.back.generic.x = 20;
	s_adv.back.generic.y = 480 - 50;
	s_adv.back.generic.id = ID_BACK;
	s_adv.back.generic.callback = AdvancedGraphics_Event;
	s_adv.back.string = "< BACK";
	s_adv.back.color = text_color_normal;
	s_adv.back.style = UI_LEFT | UI_SMALLFONT;

	s_adv.apply.generic.type = MTYPE_PTEXT;
	s_adv.apply.generic.flags = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_adv.apply.generic.x = 640 - 20;
	s_adv.apply.generic.y = 480 - 50;
	s_adv.apply.generic.id = ID_APPLY;
	s_adv.apply.generic.callback = AdvancedGraphics_Event;
	s_adv.apply.string = "APPLY";
	s_adv.apply.color = text_color_normal;
	s_adv.apply.style = UI_RIGHT | UI_SMALLFONT;

	Menu_AddItem( &s_adv.menu, (void *)&s_adv.banner );
	Menu_AddItem( &s_adv.menu, (void *)&s_adv.sunshadows );
	Menu_AddItem( &s_adv.menu, (void *)&s_adv.shadowquality );
	Menu_AddItem( &s_adv.menu, (void *)&s_adv.sunlightmode );
	Menu_AddItem( &s_adv.menu, (void *)&s_adv.ssao );
	Menu_AddItem( &s_adv.menu, (void *)&s_adv.hdr );
	Menu_AddItem( &s_adv.menu, (void *)&s_adv.normalmapping );
	Menu_AddItem( &s_adv.menu, (void *)&s_adv.back );
	Menu_AddItem( &s_adv.menu, (void *)&s_adv.apply );

	AdvancedGraphics_SetMenuItems();
	AdvancedGraphics_GetInitial();
}

void UI_AdvancedGraphicsMenu( void )
{
	UI_AdvancedGraphicsMenu_Init();
	UI_PushMenu( &s_adv.menu );
}
