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

#ifndef UI_RALLY_CONFIG_H
#define UI_RALLY_CONFIG_H

/* the settings and race control pages (WebUI config pages 0-15) are drawn
 * natively: UI_Refresh and UI_KeyEvent hand them to these */
qboolean Config_Native( void );
void Config_Draw( void );
sfxHandle_t Config_Key( int key );
void Config_Dump( void );

#endif
