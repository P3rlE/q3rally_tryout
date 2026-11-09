// Cooling Tower - Autoball in a disused cooling tower.
// Written by tools/maptools/make_coolingtower.py. Compile with -custinfoparms.

textures/q3r_coolingtower/sky
{
	qer_editorimage env/arcsky/arcsky2_ft.jpg
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_globaltexture
	q3map_sun 1 0.95 0.86 220 90 65
	q3map_skylight 140 4
	q3gl2_sun 1 0.95 0.86 150 90 65 0.5
	skyparms env/arcsky/arcsky2 1024 -
	q3r_haze 0.78 0.76 0.72 9000	// Q3Rally: dusty late summer haze
}

textures/q3r_coolingtower/pitch
{
	qer_editorimage textures/q3r_coolingtower/pitch_lines.jpg
	{
		map textures/q3r_coolingtower/basin.jpg
		tcMod scale 9 9
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/q3r_coolingtower/pitch_lines.jpg
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/q3r_coolingtower/bank
{
	qer_editorimage textures/q3r_coolingtower/basin.jpg
	q3map_nonplanar
	q3map_shadeAngle 60
	{
		map textures/q3r_coolingtower/basin.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/concrete
{
	qer_editorimage textures/q3r_coolingtower/concrete.jpg
	{
		map textures/q3r_coolingtower/concrete.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/portal
{
	qer_editorimage textures/q3r_coolingtower/concrete.jpg
	{
		map textures/q3r_coolingtower/concrete.jpg
		rgbGen const ( 0.8 0.8 0.82 )
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/column
{
	qer_editorimage textures/q3r_coolingtower/concrete.jpg
	q3map_lightmapSampleSize 32
	{
		map textures/q3r_coolingtower/concrete.jpg
		rgbGen const ( 0.7 0.7 0.7 )
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/shell
{
	qer_editorimage textures/q3r_coolingtower/concrete.jpg
	q3map_nonplanar
	q3map_shadeAngle 60
	q3map_lightmapSampleSize 32
	{
		map textures/q3r_coolingtower/concrete.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/shell_top
{
	qer_editorimage textures/q3r_coolingtower/concrete.jpg
	q3map_nonplanar
	q3map_shadeAngle 60
	{
		map textures/q3r_coolingtower/concrete.jpg
		rgbGen const ( 0.85 0.85 0.85 )
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/shell_out
{
	qer_editorimage textures/q3r_coolingtower/concrete.jpg
	q3map_lightmapSampleSize 64
	{
		map textures/q3r_coolingtower/concrete.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/ground
{
	qer_editorimage textures/q3r_coolingtower/ground.jpg
	q3map_lightmapSampleSize 64
	{
		map textures/q3r_coolingtower/ground.jpg
		tcMod scale 0.25 0.25
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/portal_red
{
	qer_editorimage textures/q3r_coolingtower/concrete.jpg
	{
		map textures/q3r_coolingtower/concrete.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/q3r_coolingtower/concrete.jpg
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.25 0.04 0.03 )
	}
}

textures/q3r_coolingtower/portal_blue
{
	qer_editorimage textures/q3r_coolingtower/concrete.jpg
	{
		map textures/q3r_coolingtower/concrete.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/q3r_coolingtower/concrete.jpg
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.04 0.11 0.25 )
	}
}

textures/q3r_coolingtower/grate
{
	qer_editorimage textures/q3r_coolingtower/grate.tga
	qer_trans 0.5
	surfaceparm trans
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm SURF_METAL
	cull none
	nopicmip
	{
		map textures/q3r_coolingtower/grate.tga
		alphaFunc GE128
		depthWrite
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
		rgbGen identity
	}
}

textures/q3r_coolingtower/test25
{
	qer_editorimage textures/q3r_coolingtower/basin.jpg
	cull none
	q3map_nonplanar
	q3map_shadeAngle 60
	{
		map textures/q3r_coolingtower/basin.jpg
		rgbGen const ( 0.55 1.0 0.55 )
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/test35
{
	qer_editorimage textures/q3r_coolingtower/basin.jpg
	cull none
	q3map_nonplanar
	q3map_shadeAngle 60
	{
		map textures/q3r_coolingtower/basin.jpg
		rgbGen const ( 1.0 0.95 0.45 )
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/test42
{
	qer_editorimage textures/q3r_coolingtower/basin.jpg
	cull none
	q3map_nonplanar
	q3map_shadeAngle 60
	{
		map textures/q3r_coolingtower/basin.jpg
		rgbGen const ( 1.0 0.5 0.45 )
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}
