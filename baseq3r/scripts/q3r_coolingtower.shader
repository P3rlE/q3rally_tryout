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
	q3r_colorLut gfx/luts/warm	// Q3Rally: colour grading
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
	surfaceparm nonsolid
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
		tcMod scale 0.5 0.5
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

textures/q3r_coolingtower/steel
{
	qer_editorimage textures/q3r_metals/plate01.tga
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm SURF_METAL
	q3map_lightmapSampleSize 8
	{
		map textures/q3r_metals/plate01.tga
		rgbGen const ( 0.45 0.45 0.48 )
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
	surfaceparm nonsolid
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

textures/q3r_coolingtower/asphalt
{
	qer_editorimage textures/q3r_road/asphalt.jpg
	surfaceparm nonsolid
	q3map_lightmapSampleSize 64
	{
		map textures/q3r_road/asphalt.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/brick
{
	qer_editorimage textures/q3r_building/brick01.jpg
	surfaceparm nonsolid
	q3map_lightmapSampleSize 64
	{
		map textures/q3r_building/brick01.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/hall_windows
{
	qer_editorimage textures/q3r_building/ind-windows-1.jpg
	surfaceparm nonsolid
	q3map_lightmapSampleSize 64
	{
		map textures/q3r_building/ind-windows-1.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/far_concrete
{
	qer_editorimage textures/q3r_coolingtower/concrete.jpg
	surfaceparm nonsolid
	q3map_lightmapSampleSize 64
	q3map_nonplanar
	q3map_shadeAngle 60
	{
		map textures/q3r_coolingtower/concrete.jpg
		tcMod scale 0.5 0.5
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/chimney_red
{
	qer_editorimage textures/q3r_coolingtower/concrete.jpg
	surfaceparm nonsolid
	q3map_lightmapSampleSize 64
	{
		map textures/q3r_coolingtower/concrete.jpg
		rgbGen const ( 0.75 0.18 0.14 )
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/wire
{
	qer_editorimage textures/q3r_metals/plate01.tga
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm pointlight
	{
		map textures/q3r_metals/plate01.tga
		rgbGen const ( 0.12 0.12 0.13 )
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_coolingtower/nodraw_ns
{
	qer_editorimage textures/common/nodraw.tga
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nolightmap
}

textures/q3r_coolingtower/drain
{
	qer_editorimage textures/q3r_coolingtower/drain.tga
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	polygonOffset
	{
		map textures/q3r_coolingtower/drain.tga
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

textures/q3r_coolingtower/beacon
{
	qer_editorimage textures/q3r_coolingtower/concrete.jpg
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	q3map_surfacelight 120
	q3map_lightRGB 1 0.55 0.1
	{
		map textures/q3r_signage/white.jpg
		rgbGen wave square 0.2 0.8 0 1.2
		alphaGen const 1
	}
	{
		map textures/q3r_signage/white.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen const ( 1 0.55 0.1 )
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
