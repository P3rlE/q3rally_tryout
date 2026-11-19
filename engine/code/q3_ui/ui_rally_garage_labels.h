/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The names the WebUI's garage catalog gives paints and wheels where they are
not their file name in title case (engine/webui/catalog_labels.json, read by
engine/webui/build_showroom.py). Paints unlocked by rank are named in
game/garage_unlocks.h. tests/test_native_garage.py keeps the lists in step.
*/

#ifndef UI_RALLY_GARAGE_LABELS_H
#define UI_RALLY_GARAGE_LABELS_H

/* vehicle, paint, name */
#define UI_PAINT_LABELS( ENTRY ) \
	ENTRY( "alpine", "rally01", "Rally 01" ) \
	ENTRY( "alpine", "rally07", "Rally 07" ) \
	ENTRY( "alpine", "rally13", "Rally 13" ) \
	ENTRY( "alpine", "rally27", "Rally 27" ) \
	ENTRY( "alpine", "rally44", "Rally 44" ) \
	ENTRY( "alpine", "rally88", "Rally 88" ) \
	ENTRY( "camaro", "rally01", "Rally 01" ) \
	ENTRY( "camaro", "rally07", "Rally 07" ) \
	ENTRY( "camaro", "rally13", "Rally 13" ) \
	ENTRY( "camaro", "rally27", "Rally 27" ) \
	ENTRY( "camaro", "rally44", "Rally 44" ) \
	ENTRY( "camaro", "rally88", "Rally 88" ) \
	ENTRY( "db12", "rally01", "Rally 01" ) \
	ENTRY( "db12", "rally07", "Rally 07" ) \
	ENTRY( "db12", "rally13", "Rally 13" ) \
	ENTRY( "db12", "rally27", "Rally 27" ) \
	ENTRY( "db12", "rally44", "Rally 44" ) \
	ENTRY( "db12", "rally88", "Rally 88" ) \
	ENTRY( "gravelord", "rally01", "Rally 01" ) \
	ENTRY( "gravelord", "rally07", "Rally 07" ) \
	ENTRY( "gravelord", "rally13", "Rally 13" ) \
	ENTRY( "gravelord", "rally27", "Rally 27" ) \
	ENTRY( "gravelord", "rally44", "Rally 44" ) \
	ENTRY( "gravelord", "rally88", "Rally 88" ) \
	ENTRY( "gremlin", "rally01", "Rally 01" ) \
	ENTRY( "gremlin", "rally07", "Rally 07" ) \
	ENTRY( "gremlin", "rally13", "Rally 13" ) \
	ENTRY( "gremlin", "rally27", "Rally 27" ) \
	ENTRY( "gremlin", "rally44", "Rally 44" ) \
	ENTRY( "gremlin", "rally88", "Rally 88" ) \
	ENTRY( "macdaddy", "rally01", "Rally 01" ) \
	ENTRY( "macdaddy", "rally07", "Rally 07" ) \
	ENTRY( "macdaddy", "rally13", "Rally 13" ) \
	ENTRY( "macdaddy", "rally27", "Rally 27" ) \
	ENTRY( "macdaddy", "rally44", "Rally 44" ) \
	ENTRY( "macdaddy", "rally88", "Rally 88" ) \
	ENTRY( "mini", "rally01", "Rally 01" ) \
	ENTRY( "mini", "rally07", "Rally 07" ) \
	ENTRY( "mini", "rally13", "Rally 13" ) \
	ENTRY( "mini", "rally27", "Rally 27" ) \
	ENTRY( "mini", "rally44", "Rally 44" ) \
	ENTRY( "mini", "rally88", "Rally 88" ) \
	ENTRY( "murcielago", "rally01", "Rally 01" ) \
	ENTRY( "murcielago", "rally07", "Rally 07" ) \
	ENTRY( "murcielago", "rally13", "Rally 13" ) \
	ENTRY( "murcielago", "rally27", "Rally 27" ) \
	ENTRY( "murcielago", "rally44", "Rally 44" ) \
	ENTRY( "murcielago", "rally88", "Rally 88" ) \
	ENTRY( "raptor", "rally01", "Rally 01" ) \
	ENTRY( "raptor", "rally07", "Rally 07" ) \
	ENTRY( "raptor", "rally13", "Rally 13" ) \
	ENTRY( "raptor", "rally27", "Rally 27" ) \
	ENTRY( "raptor", "rally44", "Rally 44" ) \
	ENTRY( "raptor", "rally88", "Rally 88" ) \
	ENTRY( "reaper", "rally01", "Rally 01" ) \
	ENTRY( "reaper", "rally07", "Rally 07" ) \
	ENTRY( "reaper", "rally13", "Rally 13" ) \
	ENTRY( "reaper", "rally27", "Rally 27" ) \
	ENTRY( "reaper", "rally44", "Rally 44" ) \
	ENTRY( "reaper", "rally88", "Rally 88" ) \
	ENTRY( "roadster", "ps2", "Ps 2" ) \
	ENTRY( "roadster", "rally01", "Rally 01" ) \
	ENTRY( "roadster", "rally07", "Rally 07" ) \
	ENTRY( "roadster", "rally13", "Rally 13" ) \
	ENTRY( "roadster", "rally27", "Rally 27" ) \
	ENTRY( "roadster", "rally44", "Rally 44" ) \
	ENTRY( "roadster", "rally88", "Rally 88" ) \
	ENTRY( "sidepipe", "ra3", "Ra 3" ) \
	ENTRY( "sidepipe", "rally01", "Rally 01" ) \
	ENTRY( "sidepipe", "rally07", "Rally 07" ) \
	ENTRY( "sidepipe", "rally13", "Rally 13" ) \
	ENTRY( "sidepipe", "rally13_navy", "Rally 13 Navy" ) \
	ENTRY( "sidepipe", "rally27", "Rally 27" ) \
	ENTRY( "sidepipe", "rally44", "Rally 44" ) \
	ENTRY( "sidepipe", "rally88", "Rally 88" ) \
	ENTRY( "slingshot", "ps2", "Ps 2" ) \
	ENTRY( "slingshot", "rally01", "Rally 01" ) \
	ENTRY( "slingshot", "rally07", "Rally 07" ) \
	ENTRY( "slingshot", "rally13", "Rally 13" ) \
	ENTRY( "slingshot", "rally27", "Rally 27" ) \
	ENTRY( "slingshot", "rally44", "Rally 44" ) \
	ENTRY( "slingshot", "rally88", "Rally 88" ) \
	ENTRY( "viper", "rally01", "Rally 01" ) \
	ENTRY( "viper", "rally07", "Rally 07" ) \
	ENTRY( "viper", "rally13", "Rally 13" ) \
	ENTRY( "viper", "rally27", "Rally 27" ) \
	ENTRY( "viper", "rally44", "Rally 44" ) \
	ENTRY( "viper", "rally88", "Rally 88" )

/* wheel set, name */
#define UI_WHEEL_LABELS( ENTRY ) \
	ENTRY( "q3r_bastion", "Q3Rally Bastion" ) \
	ENTRY( "q3r_crown", "Q3Rally Crown" ) \
	ENTRY( "q3r_rally13", "Rally 13" ) \
	ENTRY( "q3r_rally13_dark", "Rally 13 Dark" ) \
	ENTRY( "q3r_stage6", "Q3Rally Stage6" ) \
	ENTRY( "q3r_talon", "Q3Rally Talon" ) \
	ENTRY( "svt_cobra", "SVT Cobra" ) \
	ENTRY( "svt_cobra_99", "SVT Cobra 99" ) \
	ENTRY( "svt_cobra_contour", "SVT Cobra Contour" ) \
	ENTRY( "svt_cobra_r", "SVT Cobra R" ) \
	ENTRY( "svt_lightning", "SVT Lightning" ) \
	ENTRY( "svt_tbird", "SVT Tbird" )

#endif
