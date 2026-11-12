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
