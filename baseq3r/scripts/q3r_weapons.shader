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
