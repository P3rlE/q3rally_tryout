// Start light (rally_startlight). Written by tools/maptools/make_startlight.py.

models/mapobjects/startlight/housing
{
	qer_editorimage models/mapobjects/startlight/housing.jpg
	surfaceparm nolightmap
	surfaceparm SURF_METAL
	cull disable
	{
		map models/mapobjects/startlight/housing.jpg
		rgbGen vertex
	}
}

models/mapobjects/startlight/lens_red
{
	qer_editorimage models/mapobjects/startlight/lens_red.jpg
	surfaceparm nolightmap
	{
		map models/mapobjects/startlight/lens_red.jpg
		rgbGen vertex
	}
	{
		map models/mapobjects/startlight/lens_red.jpg
		blendFunc GL_ONE GL_ONE
		tcGen environment
		rgbGen const ( 0.25 0.25 0.25 )
	}
}

models/mapobjects/startlight/lens_green
{
	qer_editorimage models/mapobjects/startlight/lens_green.jpg
	surfaceparm nolightmap
	{
		map models/mapobjects/startlight/lens_green.jpg
		rgbGen vertex
	}
	{
		map models/mapobjects/startlight/lens_green.jpg
		blendFunc GL_ONE GL_ONE
		tcGen environment
		rgbGen const ( 0.25 0.25 0.25 )
	}
}

// the light of a lit lamp, drawn by cgame (additive, coloured per vertex)
models/mapobjects/startlight/glow
{
	nopicmip
	nomipmaps
	cull disable
	{
		clampmap models/mapobjects/startlight/glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}
