/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS of the WebUI's driver page, rule for rule (see the header).
*/

#include "ui_local.h"
#include "ui_rally_driverlayout.h"
#include "ui_rally_demoslayout.h"
#include "ui_rally_rivalslayout.h"

#define REM		WS_REM
#define LU		WS_LU

/* the Stats tab, as driver.js groups it */
const char *driverGroups[] = { "General", "Racing", "Racing deathmatch", "Sprint", "Elimination", "Last car standing", "Derby", "Deathmatch", "Capture the flag", "Four-team CTF", "Team deathmatch", "Team racing", "Team racing deathmatch", "Domination", "King of the hill", "Autoball" };
const int driverGroupSize[] = { 20, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
const char *driverStatKeys[] = {
	"gamesPlayed", "wins", "losses", "playerScore", "distanceKm", "fuelUsed", "topSpeedKph", "bestLapMs", "kills", "deaths", "damageDealt", "damageTaken", "flagCaptures", "flagAssists", "accuracyAwards", "excellentAwards", "impressiveAwards", "perfectAwards", "mostUsedVehicle", "mostUsedVehicleTimeMs",
	"racingWins", "racingPodiums", "racingCompleted", "racingTotalMs",
	"racingDmWins", "racingDmPodiums", "racingDmCompleted", "racingDmTotalMs",
	"sprintWins", "sprintCompleted", "sprintBestMs",
	"eliminationWins", "eliminationCompleted", "eliminationTotalRoundsLasted",
	"lcsWins", "lcsCompleted", "lcsTotalSurvivalMs",
	"derbyWins", "derbyCompleted", "derbyKills",
	"dmWins", "dmCompleted", "dmKills",
	"ctfWins", "ctfCompleted", "ctfCaptures",
	"ctf4Wins", "ctf4Completed", "ctf4Captures",
	"teamWins", "teamCompleted", "teamKills",
	"teamRacingWins", "teamRacingCompleted", "teamRacingPodiums",
	"teamRacingDmWins", "teamRacingDmCompleted", "teamRacingDmPodiums",
	"dominationWins", "dominationCompleted", "dominationZoneHoldMs",
	"kothWins", "kothCompleted", "kothZoneHoldMs",
	"autoballWins", "autoballCompleted", "autoballGoals"
};
/* key.replace(/([A-Z])/g, " $1").replace(/ Ms$/, "") with a capital first letter */
const char *driverStatLabels[] = {
	"Games Played", "Wins", "Losses", "Player Score", "Distance Km", "Fuel Used", "Top Speed Kph", "Best Lap", "Kills", "Deaths", "Damage Dealt", "Damage Taken", "Flag Captures", "Flag Assists", "Accuracy Awards", "Excellent Awards", "Impressive Awards", "Perfect Awards", "Most Used Vehicle", "Most Used Vehicle Time",
	"Racing Wins", "Racing Podiums", "Racing Completed", "Racing Total",
	"Racing Dm Wins", "Racing Dm Podiums", "Racing Dm Completed", "Racing Dm Total",
	"Sprint Wins", "Sprint Completed", "Sprint Best",
	"Elimination Wins", "Elimination Completed", "Elimination Total Rounds Lasted",
	"Lcs Wins", "Lcs Completed", "Lcs Total Survival",
	"Derby Wins", "Derby Completed", "Derby Kills",
	"Dm Wins", "Dm Completed", "Dm Kills",
	"Ctf Wins", "Ctf Completed", "Ctf Captures",
	"Ctf4 Wins", "Ctf4 Completed", "Ctf4 Captures",
	"Team Wins", "Team Completed", "Team Kills",
	"Team Racing Wins", "Team Racing Completed", "Team Racing Podiums",
	"Team Racing Dm Wins", "Team Racing Dm Completed", "Team Racing Dm Podiums",
	"Domination Wins", "Domination Completed", "Domination Zone Hold",
	"Koth Wins", "Koth Completed", "Koth Zone Hold",
	"Autoball Wins", "Autoball Completed", "Autoball Goals"
};


const char *driverOverviewLabels[4] = { "Profile points", "Games played", "Wins", "Distance" };

#define DRIVER_GAP			LU( 0.6f * REM )		/* .driver-tabs, .driver-actions: gap .6rem */
#define DRIVER_VALUE_LINE	LU( 1.5f * 1.6f * REM )
#define DRIVER_SELECT		( 2.0f * LU( 0.65f * REM ) + 2.0f + 21.0f )
#define DRIVER_INPUT		( 2.0f * LU( 0.65f * REM ) + 2.0f + WS_BODY_LINE )
#define DRIVER_DATE			( 2.0f * LU( 0.65f * REM ) + 2.0f + 26.0f )	/* Chrome's date field is 26px inside */
#define DRIVER_FILTER		( 2.0f * LU( 0.65f * REM ) + 2.0f + 17.0f )	/* the system font's line */
#define DRIVER_H3			( 1.17f * REM )			/* h3: 1.17em, margins 1.17em */

void Driver_ValueStyle( wsTextStyle_t *style ) {
	WS_BodyStyle( style );
	style->font = WS_BODY_BOLD;
	style->size = 1.6f * REM;
	style->lineHeight = DRIVER_VALUE_LINE;
}

void Driver_MedalStyle( wsTextStyle_t *style ) {
	style->font = WS_CONDENSED;
	style->size = 1.5f * REM;
	style->spacing = 0.0f;
	style->lineHeight = 1.5f * 1.5f * REM;
	style->uppercase = qfalse;
}

/* flex-wrap: boxes of the given widths from x; returns the height used */
static float Driver_Flow( wsRect_t *box, int count, float x, float y, float width, float h, float gap ) {
	float cx = x, cy = y;
	int i;
	for ( i = 0; i < count; i++ ) {
		if ( cx > x && cx + box[i].w > x + width + 0.01f ) {
			cx = x;
			cy += h + gap;
		}
		box[i].x = cx;
		box[i].y = cy;
		box[i].h = h;
		cx += box[i].w + gap;
	}
	return cy + h - y;
}

/* .driver-metrics: repeat(auto-fit, minmax(9rem, 1fr)), gap 1rem; returns the height */
static float Driver_Cards( driverCard_t *card, const char **labels, const char **values, int count, float x, float y, float width ) {
	wsTextStyle_t label, value;
	int columns, i, j;
	float column, top = y;

	WS_LabelStyle( &label );
	Driver_ValueStyle( &value );
	columns = (int)( ( width + REM ) / ( 9.0f * REM + REM ) );
	if ( columns < 1 ) columns = 1;
	if ( columns > count ) columns = count;
	column = ( width - ( columns - 1 ) * REM ) / columns;
	for ( i = 0; i < count; i += columns ) {
		float rowH = 0.0f;
		for ( j = i; j < count && j < i + columns; j++ ) {
			float h;
			WS_StyledWrap( &label, labels[j], column, &card[j].label );
			Demos_WrapAnywhere( &value, values[j] ? values[j] : "", column, &card[j].value );
			h = card[j].label.count * LU( WS_LABEL_LINE ) + card[j].value.count * DRIVER_VALUE_LINE;
			if ( h > rowH ) rowH = h;
		}
		for ( j = i; j < count && j < i + columns; j++ ) {
			card[j].box.x = x + ( j - i ) * ( column + REM );
			card[j].box.y = y;
			card[j].box.w = column;
			card[j].box.h = rowH;
		}
		y += rowH + REM;
	}
	return count ? y - REM - top : 0.0f;
}

/* line boxes beside a float are shorter: greedy wrapping where the lines
 * that start above floatBottom lose floatW */
static float Driver_WrapAround( const wsTextStyle_t *style, const char *text, float width, float y, float floatBottom, float floatW, wsLines_t *lines ) {
	char shown[256];
	int start = 0, length, end, next;

	WS_Transform( style, text, shown, sizeof( shown ) );
	length = strlen( shown );
	memset( lines, 0, sizeof( *lines ) );
	while ( start < length && lines->count < WS_MAX_LINES ) {
		float room = y + lines->count * LU( style->lineHeight ) < floatBottom ? width - floatW : width;
		end = start;
		while ( end < length && shown[end] != ' ' ) end++;
		while ( end < length ) {
			next = end;
			while ( next < length && shown[next] == ' ' ) next++;
			while ( next < length && shown[next] != ' ' ) next++;
			if ( WS_TextWidth( style, shown + start, next - start ) > room + 0.01f ) break;
			end = next;
		}
		lines->start[lines->count] = start;
		lines->length[lines->count] = end - start;
		lines->width[lines->count] = WS_TextWidth( style, shown + start, end - start );
		lines->count++;
		start = end;
		while ( start < length && shown[start] == ' ' ) start++;
	}
	return lines->count * LU( style->lineHeight );
}

/* an achievement card: padding 1.25rem, the medal floated right */
static float Driver_Medal( driverLayout_t *l, int i, float x, float y, float width ) {
	driverMedal_t *m = &l->medal[i];
	wsTextStyle_t label, h3, body;
	float inner = width - 2.0f - 2.5f * REM, cy, floatBottom, line;

	WS_LabelStyle( &label );
	Driver_MedalStyle( &h3 );
	WS_BodyStyle( &body );
	m->box.x = x;
	m->box.y = y;
	m->box.w = width;
	m->top = y + ( l->earned[i] ? 3.0f : 1.0f ) + 1.25f * REM;
	m->medal.w = m->medal.h = 4.0f * REM;
	m->medal.x = x + width - 1.0f - 1.25f * REM - m->medal.w;
	m->medal.y = m->top;
	floatBottom = m->top + m->medal.h;
	cy = m->top;
	m->labelY = cy;
	cy += Driver_WrapAround( &label, l->medalLabel[i], inner, cy, floatBottom, m->medal.w, &m->label );
	cy += 0.5f * REM;
	m->nameY = cy;
	cy += Driver_WrapAround( &h3, l->medalName[i], inner, cy, floatBottom, m->medal.w, &m->name );
	cy += REM;	/* .5rem and the p's 1rem collapse */
	m->textY = cy;
	cy += Driver_WrapAround( &body, l->medalText[i], inner, cy, floatBottom, m->medal.w, &m->text );
	cy += REM;
	/* the progress bar sits on the line's baseline; at 100% it does not fit beside the medal */
	if ( cy < floatBottom ) cy = floatBottom;
	line = cy;
	m->progressY = line + WS_Baseline( &body ) + 0.2f * REM - 16.0f;	/* progress { vertical-align: -.2em } */
	cy = line + WS_BODY_LINE + REM;
	m->statusY = cy;
	cy += Driver_WrapAround( &body, l->medalStatus[i], inner, cy, floatBottom, m->medal.w, &m->status );
	return cy + REM + 1.25f * REM + 1.0f - y;
}

static void Driver_Dialog( driverLayout_t *l, float width, float height ) {
	wsTextStyle_t body, label;
	float pad = WS_Clamp( REM, 0.03f * width, 2.0f * REM ), y, max;
	int i, columns;

	WS_BodyStyle( &body );
	WS_LabelStyle( &label );
	l->dialogBox.w = width - 2.0f * REM < 46.0f * REM ? width - 2.0f * REM : 46.0f * REM;
	l->dialogBox.x = ( l->page.pageWidth - l->dialogBox.w ) * 0.5f;
	l->dialogW = l->dialogBox.w - 2.0f - 2.0f * pad;
	l->dialogX = l->dialogBox.x + 1.0f + pad;
	for ( i = 0; i < 2; i++ ) {
		/* twice when the content scrolls: the scrollbar takes its room */
		y = 3.0f + pad;
		l->dialogLabelY = y;
		l->dialogH2Y = y + LU( WS_LABEL_LINE ) + 0.5f * REM;
		WS_StyledWrap( &l->h2, l->dialog == DRIVER_EDIT ? "Edit driver" : "Your drivers", l->dialogW, &l->nameLines );
		y = l->dialogH2Y + l->nameLines.count * l->h2.size + 1.5f * REM;
		if ( l->dialog == DRIVER_EDIT ) {
			float controls[3], cell;
			int rows;
			controls[0] = controls[2] = DRIVER_SELECT;
			controls[1] = DRIVER_DATE;
			l->profileLineY = y + REM;
			y = l->profileLineY + WS_BODY_LINE + REM + REM;
			for ( i = 0; i < 3; i++ ) {
				l->fieldLabelY[i] = y;
				l->field[i].x = l->dialogX;
				l->field[i].w = l->dialogW;
				l->field[i].y = y + WS_BODY_LINE + LU( 0.4f * REM );
				l->field[i].h = controls[i];
				y = l->field[i].y + controls[i] + REM;
			}
			l->avatarLabelY = y;
			y += LU( WS_LABEL_LINE ) + REM;
			/* .driver-avatar-choices: repeat(auto-fit, minmax(4.5rem, 1fr)), gap .5rem; square buttons */
			columns = (int)( ( l->dialogW + 0.5f * REM ) / ( 4.5f * REM + 0.5f * REM ) );
			if ( columns < 1 ) columns = 1;
			if ( columns > l->avatars && l->avatars > 0 ) columns = l->avatars;
			cell = ( l->dialogW - ( columns - 1 ) * 0.5f * REM ) / columns;
			for ( i = 0; i < l->avatars && i < DRIVER_AVATARS; i++ ) {
				l->avatarChoice[i].x = l->dialogX + ( i % columns ) * ( cell + 0.5f * REM );
				l->avatarChoice[i].y = y + ( i / columns ) * ( cell + 0.5f * REM );
				l->avatarChoice[i].w = l->avatarChoice[i].h = cell;
			}
			rows = ( l->avatars + columns - 1 ) / columns;
			y += rows ? rows * cell + ( rows - 1 ) * 0.5f * REM + REM : 0.0f;
			l->submit.w = WS_ButtonWidth( "Save driver" );
		} else {
			float listY = 0.0f;
			l->list.x = l->dialogX;
			l->list.w = l->dialogW;
			l->list.y = y;
			if ( l->profiles ) {
				for ( i = 0; i < l->profiles; i++ ) {
					l->profileButton[i].x = 0.0f;
					l->profileButton[i].y = listY;
					l->profileButton[i].w = l->dialogW;
					l->profileButton[i].h = WS_BUTTON_HEIGHT;
					listY += WS_BUTTON_HEIGHT + 0.5f * REM;
				}
				l->listHeight = listY - 0.5f * REM;
			} else {
				l->listHeight = 2.0f * REM + WS_BODY_LINE;
			}
			l->list.h = l->listHeight < 15.0f * REM ? l->listHeight : 15.0f * REM;
			y += l->list.h + 2.0f * REM;
			/* the form: h3 (1.17em, margins 1.17em), the name, the help, the button */
			l->createY = y + DRIVER_H3;
			y = l->createY + 1.5f * DRIVER_H3 + DRIVER_H3 + REM;
			l->nameLabelY = y;
			l->nameInput.x = l->dialogX;
			l->nameInput.w = l->dialogW;
			l->nameInput.y = y + WS_BODY_LINE + LU( 0.4f * REM );
			l->nameInput.h = DRIVER_INPUT;
			y = l->nameInput.y + DRIVER_INPUT + REM;
			l->helpY = y + REM;
			WS_Wrap( &body, "Use letters, numbers, a dash or an underscore. A new profile starts with its own progress.", l->dialogW, &l->helpLines );
			y = l->helpY + l->helpLines.count * WS_BODY_LINE + REM + REM;
			l->submit.w = WS_ButtonWidth( "Create & select" );
		}
		l->submit.x = l->dialogX;
		l->submit.w = l->dialogW;		/* a grid item stretches */
		l->submit.y = y;
		l->submit.h = WS_BUTTON_HEIGHT;
		y += WS_BUTTON_HEIGHT + REM;
		/* the status: min-height 1.5em */
		l->statusY = y;
		WS_Wrap( &body, l->status ? l->status : "", l->dialogW, &l->statusLines );
		y += ( l->statusLines.count > 1 ? l->statusLines.count : 1 ) * WS_BODY_LINE + REM;
		l->close.x = l->dialogX;
		l->close.y = y;
		l->close.w = WS_ButtonWidth( "Close" );
		l->close.h = WS_BUTTON_HEIGHT;
		l->dialogHeight = y + WS_BUTTON_HEIGHT + pad + 1.0f;
		max = height - 3.0f * REM;
		l->dialogBar = l->dialogHeight > max + 0.5f;
		if ( !l->dialogBar || l->hideScrollbar || i == 1 ) break;
		l->dialogW -= RIVALS_SCROLLBAR;
	}
	l->dialogBox.h = l->dialogBar ? max : l->dialogHeight;
	l->dialogBox.y = ( height - l->dialogBox.h ) * 0.5f;
	l->dialogPort.x = l->dialogBox.x + 1.0f;
	l->dialogPort.w = l->dialogBox.w - 2.0f;
	l->dialogPort.y = l->dialogBox.y + 3.0f;
	l->dialogPort.h = l->dialogBox.h - 4.0f;
	l->dialogScroll = l->dialogBar ? WS_Clamp( 0.0f, floor( l->dialogScroll + 0.5f ), floor( l->dialogHeight - l->dialogBox.h + 0.5f ) ) : 0.0f;
	memset( &l->dialogThumb, 0, sizeof( l->dialogThumb ) );
	if ( l->dialogBar ) WS_ScrollThumb( &l->dialogPort, l->dialogHeight - 4.0f, l->dialogScroll, &l->dialogThumb );
	{
		float dy = l->dialogBox.y - l->dialogScroll;
		l->dialogLabelY += dy; l->dialogH2Y += dy; l->profileLineY += dy; l->avatarLabelY += dy;
		for ( i = 0; i < 3; i++ ) { l->fieldLabelY[i] += dy; l->field[i].y += dy; }
		for ( i = 0; i < l->avatars && i < DRIVER_AVATARS; i++ ) l->avatarChoice[i].y += dy;
		l->list.y += dy; l->createY += dy; l->nameLabelY += dy; l->nameInput.y += dy; l->helpY += dy;
		l->submit.y += dy; l->statusY += dy; l->close.y += dy;
	}
	l->listBar = l->dialog == DRIVER_SWITCH && l->listHeight > l->list.h + 0.5f;
	l->listScroll = l->listBar ? WS_Clamp( 0.0f, floor( l->listScroll + 0.5f ), floor( l->listHeight - l->list.h + 0.5f ) ) : 0.0f;
	memset( &l->listThumb, 0, sizeof( l->listThumb ) );
	if ( l->listBar ) WS_ScrollThumb( &l->list, l->listHeight, l->listScroll, &l->listThumb );
}

void Driver_Layout( driverLayout_t *l, float width, float height ) {
	wsTextStyle_t body, label;
	static const char *tabText[3] = { "Overview", "Stats", "Achievements" };
	float top, pad, y, inner, panelBottom;
	int i;

	WS_BodyStyle( &body );
	WS_LabelStyle( &label );
	WS_FrontPageLabeled( &l->page, width, height, "Q3RALLY / YOUR DRIVER", "Driver ", "profile.", "" );
	top = WS_PlaceHeaderButton( &l->page, "Back", &l->back );
	pad = l->page.sectionPad;
	l->h2.font = WS_CONDENSED;
	l->h2.size = l->h2.lineHeight = LU( WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM ) );
	l->h2.spacing = 0.0f;
	l->h2.uppercase = qtrue;

	/* .driver-tabs: flex, wrap, gap .6rem, margin-bottom 1.5rem */
	for ( i = 0; i < 3; i++ ) l->tabs[i].w = WS_ButtonWidth( tabText[i] );
	top += Driver_Flow( l->tabs, 3, l->page.contentX, top, l->page.contentW, WS_BUTTON_HEIGHT, DRIVER_GAP ) + 1.5f * REM;
	l->panelY = top;
	panelBottom = top;

	if ( l->tab == 0 ) {
		/* #driver-overview: minmax(0, .8fr) minmax(0, 1.5fr), gap 1rem; one column up to 48rem */
		qboolean two = width > 48.0f * REM;
		float column1 = two ? ( l->page.contentW - REM ) * 0.8f / 2.3f : l->page.contentW;
		float divW, divH, lineH[2], identityH, careerH, rankW, labelW;
		int lines;
		l->identity.x = l->page.contentX;
		l->identity.w = column1;
		l->identity.y = top;
		l->career.x = two ? l->page.contentX + column1 + REM : l->page.contentX;
		l->career.w = two ? l->page.contentW - REM - column1 : l->page.contentW;
		inner = column1 - 2.0f - 2.0f * pad;
		/* the identity: the avatar (7rem) and the text, flex, wrap, centred, gap 1.5rem */
		divW = WS_StyledWidth( &l->h2, l->name );
		labelW = WS_StyledWidth( &label, "YOUR DRIVER" );
		rankW = WS_TextWidth( &body, l->rank, strlen( l->rank ) );
		if ( labelW > divW ) divW = labelW;
		if ( rankW > divW ) divW = rankW;
		if ( l->country[0] && WS_TextWidth( &body, l->country, strlen( l->country ) ) > divW ) divW = WS_TextWidth( &body, l->country, strlen( l->country ) );
		if ( divW > inner ) divW = inner;
		WS_StyledWrap( &l->h2, l->name, divW + 0.01f, &l->nameLines );
		WS_Wrap( &body, l->rank, divW + 0.01f, &l->rankLines );
		WS_Wrap( &body, l->country, divW + 0.01f, &l->countryLines );
		divH = LU( WS_LABEL_LINE ) + WS_HEADING_MARGIN + l->nameLines.count * l->h2.size + REM + l->rankLines.count * WS_BODY_LINE +
		       ( l->country[0] ? REM + l->countryLines.count * WS_BODY_LINE : 0.0f ) + REM;
		lines = l->avatar && 7.0f * REM + 1.5f * REM + divW > inner + 0.01f ? 2 : 1;
		lineH[0] = l->avatar && lines == 1 ? ( 7.0f * REM > divH ? 7.0f * REM : divH ) : l->avatar ? 7.0f * REM : divH;
		lineH[1] = divH;
		identityH = lineH[0] + ( lines == 2 ? 1.5f * REM + lineH[1] : 0.0f ) + 2.0f * pad + 2.0f;

		/* the progression */
		l->careerX = l->career.x + 1.0f + pad;
		l->careerW = l->career.w - 2.0f - 2.0f * pad;
		l->careerH2Y = top + 1.0f + pad + WS_HEADING_MARGIN;
		{
			wsLines_t h2Lines;
			WS_StyledWrap( &l->h2, "Your progression", l->careerW, &h2Lines );
			y = l->careerH2Y + h2Lines.count * l->h2.size + 1.5f * REM;
		}
		y += Driver_Cards( l->overviewCard, driverOverviewLabels, l->overview, 4, l->careerX, y, l->careerW ) + 1.5f * REM;
		l->progressY = y + WS_Baseline( &body ) + 0.2f * REM - 16.0f;
		y += WS_BODY_LINE + REM;
		l->progressTextY = y;
		y += WS_BODY_LINE + REM;
		{
			static const char *actionText[3] = { "Edit driver", "Select / create profile", "Web profile & garage" };
			for ( i = 0; i < 3; i++ ) l->actions[i].w = WS_ButtonWidth( actionText[i] );
		}
		y += Driver_Flow( l->actions, 3, l->careerX, y, l->careerW, WS_BUTTON_HEIGHT, DRIVER_GAP );
		careerH = y + pad + 1.0f - top;

		if ( two ) {
			float h = identityH > careerH ? identityH : careerH;
			l->identity.h = l->career.h = h;
			l->career.y = top;
			panelBottom = top + h;
		} else {
			float dy = identityH + REM;
			l->identity.h = identityH;
			l->career.y = top + dy;
			l->career.h = careerH;
			l->careerH2Y += dy;
			l->progressY += dy;
			l->progressTextY += dy;
			for ( i = 0; i < 4; i++ ) l->overviewCard[i].box.y += dy;
			for ( i = 0; i < 3; i++ ) l->actions[i].y += dy;
			panelBottom = l->career.y + careerH;
		}
		/* align-content stretch: the free height goes to the lines in equal parts */
		{
			float content = l->identity.h - 2.0f - 2.0f * pad, free = content - ( identityH - 2.0f - 2.0f * pad ), lineTop;
			lineTop = top + 1.0f + pad;
			lineH[0] += free / lines;
			lineH[1] += free / lines;
			l->identityX = l->identity.x + 1.0f + pad;
			l->identityW = divW;
			if ( l->avatar ) {
				l->avatarBox.x = l->identityX;
				l->avatarBox.w = l->avatarBox.h = 7.0f * REM;
				l->avatarBox.y = lineTop + ( lineH[0] - 7.0f * REM ) * 0.5f;
				if ( lines == 1 ) {
					l->identityX += 7.0f * REM + 1.5f * REM;
				} else {
					lineTop += lineH[0] + 1.5f * REM;
				}
			}
			l->identityY = lineTop + ( ( lines == 2 ? lineH[1] : lineH[0] ) - divH ) * 0.5f;
			l->identityLabelY = l->identityY;
			l->nameY = l->identityY + LU( WS_LABEL_LINE ) + WS_HEADING_MARGIN;
			l->rankY = l->nameY + l->nameLines.count * l->h2.size + REM;
			l->countryY = l->rankY + l->rankLines.count * WS_BODY_LINE + REM;
		}
	} else if ( l->tab == 1 ) {
		/* #driver-stats: a section per group, gap 1rem */
		int first = 0;
		y = top;
		for ( i = 0; i < DRIVER_GROUPS; i++ ) {
			wsLines_t h2Lines;
			float cy;
			l->group[i].x = l->page.contentX;
			l->group[i].w = l->page.contentW;
			l->group[i].y = y;
			inner = l->page.contentW - 2.0f - 2.0f * pad;
			l->groupH2Y[i] = y + 1.0f + pad + WS_HEADING_MARGIN;
			WS_StyledWrap( &l->h2, driverGroups[i], inner, &h2Lines );
			cy = l->groupH2Y[i] + h2Lines.count * l->h2.size + 1.5f * REM;
			cy += Driver_Cards( &l->statCard[first], &driverStatLabels[first], &l->stats[first], driverGroupSize[i],
			                    l->page.contentX + 1.0f + pad, cy, inner ) + 1.5f * REM;
			l->group[i].h = cy + pad + 1.0f - y;
			y += l->group[i].h + REM;
			first += driverGroupSize[i];
		}
		panelBottom = y - REM;
	} else {
		/* "Category", the select (block, margin .5rem 0), the count, the cards */
		wsTextStyle_t system;
		int columns, j;
		float column;
		WS_BodyStyle( &system );
		system.size = 13.333333f;
		l->categoryY = top;
		l->filter.x = l->page.contentX;
		l->filter.y = top + WS_BODY_LINE + 0.5f * REM;
		/* Chrome draws this select in the system font: its width follows the
		 * widest option ("All achievements": 143.781px) */
		l->filter.w = 143.781f + WS_TextWidth( &system, l->filterText, strlen( l->filterText ) ) - WS_TextWidth( &system, "All achievements", 16 );
		l->filter.h = DRIVER_FILTER;
		l->countY = l->filter.y + l->filter.h + REM;
		y = l->countY + WS_BODY_LINE + REM;
		/* .driver-medals: repeat(auto-fit, minmax(min(100%, 16rem), 1fr)), gap 1rem */
		columns = (int)( ( l->page.contentW + REM ) / ( 16.0f * REM + REM ) );
		if ( columns < 1 ) columns = 1;
		if ( columns > l->medals && l->medals > 0 ) columns = l->medals;
		column = ( l->page.contentW - ( columns - 1 ) * REM ) / columns;
		for ( i = 0; i < l->medals; i += columns ) {
			float rowH = 0.0f;
			for ( j = i; j < l->medals && j < i + columns; j++ ) {
				float h = Driver_Medal( l, j, l->page.contentX + ( j - i ) * ( column + REM ), y, column );
				if ( h > rowH ) rowH = h;
			}
			for ( j = i; j < l->medals && j < i + columns; j++ ) l->medal[j].box.h = rowH;
			y += rowH + REM;
		}
		panelBottom = l->medals ? y - REM : y;
	}
	l->pageHeight = panelBottom + l->page.pad;

	/* the page's scroll */
	l->viewport.x = 0.0f; l->viewport.y = 0.0f; l->viewport.w = width; l->viewport.h = height;
	l->pageBar = l->pageHeight > height + 0.5f;
	l->pageScroll = l->pageBar ? WS_Clamp( 0.0f, floor( l->pageScroll + 0.5f ), floor( l->pageHeight - height + 0.5f ) ) : 0.0f;
	memset( &l->pageThumb, 0, sizeof( l->pageThumb ) );
	if ( l->pageBar ) {
		float dy = -l->pageScroll;
		WS_ScrollThumb( &l->viewport, l->pageHeight, l->pageScroll, &l->pageThumb );
		l->page.headerY += dy; l->page.h1Y += dy; l->back.y += dy;
		for ( i = 0; i < 3; i++ ) l->tabs[i].y += dy;
		l->panelY += dy;
		l->identity.y += dy; l->career.y += dy; l->avatarBox.y += dy;
		l->identityY += dy; l->identityLabelY += dy; l->nameY += dy; l->rankY += dy; l->countryY += dy;
		l->careerH2Y += dy; l->progressY += dy; l->progressTextY += dy;
		for ( i = 0; i < 4; i++ ) l->overviewCard[i].box.y += dy;
		for ( i = 0; i < 3; i++ ) l->actions[i].y += dy;
		for ( i = 0; i < DRIVER_GROUPS; i++ ) { l->group[i].y += dy; l->groupH2Y[i] += dy; }
		for ( i = 0; i < DRIVER_STATS; i++ ) l->statCard[i].box.y += dy;
		l->categoryY += dy; l->filter.y += dy; l->countY += dy;
		for ( i = 0; i < l->medals; i++ ) {
			driverMedal_t *m = &l->medal[i];
			m->box.y += dy; m->top += dy; m->medal.y += dy;
			m->labelY += dy; m->nameY += dy; m->textY += dy; m->progressY += dy; m->statusY += dy;
		}
	}
	if ( l->dialog != DRIVER_NO_DIALOG ) Driver_Dialog( l, width, height );
}
