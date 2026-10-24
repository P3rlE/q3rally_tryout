// Cold industrial materials for the snowbound Iceport Autoball arena.

textures/q3r_iceport/ice
{
	qer_editorimage textures/arctic/snow1_1.jpg
	{
		map textures/arctic/snow1_1.jpg
		tcMod scale 0.125 0.125
		rgbGen const ( 0.78 0.86 0.94 )
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_iceport/steel
{
	qer_editorimage textures/q3r_metals/cormetal02.tga
	{
		map textures/q3r_metals/cormetal02.tga
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_iceport/ice_mark
{
	qer_editorimage textures/arctic/snow1_1.jpg
	surfaceparm nonsolid
	surfaceparm nolightmap
	surfaceparm nomarks
	{
		map textures/arctic/snow1_1.jpg
		rgbGen const ( 0.68 0.80 0.91 )
	}
}

textures/q3r_iceport/container_red
{
	qer_editorimage textures/q3r_metals/cormetal04.tga
	{
		map textures/q3r_metals/cormetal04.tga
		rgbGen const ( 0.62 0.16 0.13 )
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_iceport/container_blue
{
	qer_editorimage textures/q3r_metals/cormetal03.tga
	{
		map textures/q3r_metals/cormetal03.tga
		rgbGen const ( 0.12 0.38 0.68 )
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_iceport/ice_paint
{
	qer_editorimage textures/q3r_lights/proto_light.jpg
	surfaceparm nonsolid
	surfaceparm nolightmap
	surfaceparm nomarks
	{
		map $whiteimage
		rgbGen const ( 0.16 0.54 0.76 )
	}
}

textures/q3r_iceport/cyan_light
{
	qer_editorimage textures/q3r_lights/proto_light.jpg
	surfaceparm nonsolid
	surfaceparm nolightmap
	surfaceparm nomarks
	q3map_surfacelight 110
	{
		map $whiteimage
		rgbGen const ( 0.12 0.76 1.0 )
	}
}

textures/q3r_iceport/red_light
{
	qer_editorimage textures/q3r_lights/proto_light.jpg
	surfaceparm nonsolid
	surfaceparm nolightmap
	surfaceparm nomarks
	q3map_surfacelight 85
	{
		map $whiteimage
		rgbGen const ( 1.0 0.12 0.08 )
	}
}

textures/q3r_iceport/blue_light
{
	qer_editorimage textures/q3r_lights/proto_light.jpg
	surfaceparm nonsolid
	surfaceparm nolightmap
	surfaceparm nomarks
	q3map_surfacelight 85
	{
		map $whiteimage
		rgbGen const ( 0.10 0.42 1.0 )
	}
}
