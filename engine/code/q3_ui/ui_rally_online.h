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

#ifndef UI_RALLY_ONLINE_H
#define UI_RALLY_ONLINE_H

void Online_Text( int kind, int index, char *out, int size );
void Online_Dump( void );
/* takes over drawing and input of the server browser (g_arenaservers.menu) */
void Online_Install( menuframework_s *menu );

#endif
