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
/* takes over drawing and input of the track step (s_startserver.menu) */
void Offline_InstallTrack( menuframework_s *menu );

#endif
