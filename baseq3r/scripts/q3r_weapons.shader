// Q3Rally weapons (tools/carbuild/weapons/build_weapons.py): one shared atlas.

models/weapons2/q3r/parts
{
	qer_editorimage models/weapons2/q3r/parts.tga
	{
		map models/weapons2/q3r/chrome.tga
		tcGen environment
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/q3r/parts.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		depthWrite
		rgbGen lightingDiffuse
	}
}

// polished parts: the atlas plus a reflection
models/weapons2/q3r/chrome
{
	qer_editorimage models/weapons2/q3r/parts.tga
	{
		map models/weapons2/q3r/parts.tga
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/q3r/chrome.tga
		blendFunc GL_ONE GL_ONE
		tcGen environment
		rgbGen const ( 0.45 0.45 0.45 )
	}
}

// glowing parts (rail, sight windows, scoop core): fullbright from the atlas
models/weapons2/q3r/glow
{
	nopicmip
	{
		map models/weapons2/q3r/parts.tga
		rgbGen wave sin 0.9 0.1 0 1.5
	}
}

// Spark Gun: the spark between the plug tips
models/weapons2/q3r/spark
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/spark.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave noise 0.7 0.4 0 7
	}
}

// Nitro Flamer: the pilot flame under the nozzle
models/weapons2/q3r/pilot
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/pilot.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave noise 0.8 0.25 0 5
	}
}

// Harvester Saw: the chain runs round the bar
models/weapons2/q3r/chain
{
	{
		map models/weapons2/q3r/chain.tga
		tcMod scroll -1.6 0
		rgbGen lightingDiffuse
	}
}

// muzzle flashes: grey star / cone / ring textures, tinted per weapon,
// a white-hot core on top
models/weapons2/q3r/flash_yellow_star
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/flash_star.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 1.00 0.82 0.42 )
	}
	{
		clampmap models/weapons2/q3r/flash_star_core.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/weapons2/q3r/flash_yellow_cone
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/flash_cone.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 1.00 0.82 0.42 )
	}
	{
		clampmap models/weapons2/q3r/flash_cone_core.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/weapons2/q3r/flash_yellow_ring
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/lampflash.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 1.00 0.82 0.42 )
	}
}

models/weapons2/q3r/flash_orange_star
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/flash_star.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 1.00 0.55 0.16 )
	}
	{
		clampmap models/weapons2/q3r/flash_star_core.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/weapons2/q3r/flash_orange_cone
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/flash_cone.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 1.00 0.55 0.16 )
	}
	{
		clampmap models/weapons2/q3r/flash_cone_core.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/weapons2/q3r/flash_orange_ring
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/lampflash.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 1.00 0.55 0.16 )
	}
}

models/weapons2/q3r/flash_ice_star
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/flash_star.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.50 0.85 1.00 )
	}
	{
		clampmap models/weapons2/q3r/flash_star_core.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/weapons2/q3r/flash_ice_cone
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/flash_cone.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.50 0.85 1.00 )
	}
	{
		clampmap models/weapons2/q3r/flash_cone_core.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/weapons2/q3r/flash_ice_ring
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/lampflash.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.50 0.85 1.00 )
	}
}

models/weapons2/q3r/flash_blue_star
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/flash_star.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.30 0.72 1.00 )
	}
	{
		clampmap models/weapons2/q3r/flash_star_core.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/weapons2/q3r/flash_blue_cone
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/flash_cone.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.30 0.72 1.00 )
	}
	{
		clampmap models/weapons2/q3r/flash_cone_core.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/weapons2/q3r/flash_blue_ring
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/lampflash.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.30 0.72 1.00 )
	}
}

models/weapons2/q3r/flash_green_star
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/flash_star.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.55 1.00 0.38 )
	}
	{
		clampmap models/weapons2/q3r/flash_star_core.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/weapons2/q3r/flash_green_cone
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/flash_cone.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.55 1.00 0.38 )
	}
	{
		clampmap models/weapons2/q3r/flash_cone_core.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/weapons2/q3r/flash_green_ring
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/lampflash.tga
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.55 1.00 0.38 )
	}
}

models/weapons2/q3r/flash_entity_star
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/flash_star.tga
		blendFunc GL_ONE GL_ONE
		rgbGen entity
	}
	{
		clampmap models/weapons2/q3r/flash_star_core.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/weapons2/q3r/flash_entity_cone
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/flash_cone.tga
		blendFunc GL_ONE GL_ONE
		rgbGen entity
	}
	{
		clampmap models/weapons2/q3r/flash_cone_core.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/weapons2/q3r/flash_entity_ring
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/lampflash.tga
		blendFunc GL_ONE GL_ONE
		rgbGen entity
	}
}

// Spotlight MG: the lamp ring flashes with every shot (in machinegun_flash.md3)
models/weapons2/q3r/lampflash
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/lampflash.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}
