/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The garage's catalog, found in the game files the way
engine/webui/build_showroom.py builds the WebUI's catalog.json: the vehicles
(models/players/<car>/body.md3), their paints and the wheels and drivers
(<key>.skin with an icon_<key>), the license plates (models/players/plates)
and what each one is called and unlocks at.
*/

#ifndef UI_RALLY_GARAGEDATA_H
#define UI_RALLY_GARAGEDATA_H

#define GARAGE_KEY			32
#define GARAGE_NAME			40
#define GARAGE_MAX_VEHICLES	40
#define GARAGE_MAX_PAINTS	40
#define GARAGE_MAX_WHEELS	192
#define GARAGE_MAX_HEADS	48
#define GARAGE_MAX_PLATES	96

typedef struct {
	char	key[GARAGE_KEY];
	char	name[GARAGE_NAME];
	int		rank;
	char	ext[5];			/* the plate's file type */
} garageOption_t;

typedef struct {
	char			key[GARAGE_KEY];
	char			name[GARAGE_NAME];
	int				rank;
	int				paints;
	garageOption_t	paint[GARAGE_MAX_PAINTS];
} garageVehicle_t;

typedef struct {
	qboolean		loaded;
	int				vehicles, wheels, heads, plates;
	garageVehicle_t	vehicle[GARAGE_MAX_VEHICLES];
	garageOption_t	wheel[GARAGE_MAX_WHEELS];
	garageOption_t	head[GARAGE_MAX_HEADS];
	garageOption_t	plate[GARAGE_MAX_PLATES];
} garageCatalog_t;

extern garageCatalog_t garageCatalog;

void		Garage_LoadCatalog( void );
/* "Rookie Driver" ... (game/profile_shared.h) */
const char	*Garage_RankName( int rank );
void		Garage_TitleCase( const char *key, char *out, int size );
int			Garage_FindVehicle( const char *key );
int			Garage_FindOption( const garageOption_t *options, int count, const char *key );
/* the picture of an option: icon_<key> beside its skin, the plate itself */
void		Garage_PaintIcon( int vehicle, int paint, char *out, int size );
void		Garage_OptionIcon( const char *folder, const garageOption_t *option, char *out, int size );

#endif
