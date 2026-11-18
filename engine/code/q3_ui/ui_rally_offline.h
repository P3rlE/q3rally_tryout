/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

#ifndef UI_RALLY_OFFLINE_H
#define UI_RALLY_OFFLINE_H

void Offline_Text( int kind, int index, int choice, char *out, int size );
void Offline_Dump( void );
/* width / height of levelshots/<name> from its file header (1 when unknown) */
float Offline_ImageAspect( const char *name );
/* takes over drawing and input of the track step (s_startserver.menu) */
void Offline_InstallTrack( menuframework_s *menu );
/* and of the rules and drivers step (s_serveroptions.menu), ui_rally_offlinerules.c */
void Offline_InstallRules( menuframework_s *menu );

#endif
