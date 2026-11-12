// Timber Run - A to B gravel sprint through an autumn forest, early in the morning.
// Written by tools/maptools/make_timberrun.py. Compile with -custinfoparms.

textures/q3r_timberrun/sky
{
	qer_editorimage env/q3r_timberrun/q3r_timberrun_ft.jpg
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_globaltexture
	q3map_sun 1 0.84 0.64 230 300 20
	q3map_skylight 240 4
	q3gl2_sun 1 0.84 0.64 150 300 20 0.6
	skyparms env/q3r_timberrun/q3r_timberrun - -
	q3r_haze 0.80 0.80 0.78 3000 1300 701 320 0.8	// Q3Rally: morning mist, thick in the creek valley
}

textures/q3r_timberrun/creek
{
	qer_editorimage textures/q3r_liquids/water.tga
	qer_trans 0.35
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nolightmap
	q3map_globaltexture
	cull disable
	q3r_water textures/q3r_liquids/water_normal.tga 160 0.03 0.014
	q3r_waterColor 0.10 0.13 0.10 0.55
	q3r_waterFresnel 0.10 4
	q3r_waterSpecular 0.8
	{
		map textures/q3r_liquids/water.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen const ( 0.22 0.27 0.22 )
		alphaGen const 0.62
		tcMod scale 0.5 0.5
		tcMod scroll 0.02 0.004
	}
}

// trees: leaves alpha tested, cast alpha shadows; vertex lit by q3map2
models/mapobjects/q3r_timberrun/spruce
{
	qer_editorimage models/mapobjects/q3r_timberrun/spruce.tga
	surfaceparm nonsolid
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	cull disable
	{
		map models/mapobjects/q3r_timberrun/spruce.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/beech_y
{
	qer_editorimage models/mapobjects/q3r_timberrun/beech_y.tga
	surfaceparm nonsolid
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	cull disable
	{
		map models/mapobjects/q3r_timberrun/beech_y.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/beech_r
{
	qer_editorimage models/mapobjects/q3r_timberrun/beech_r.tga
	surfaceparm nonsolid
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	cull disable
	{
		map models/mapobjects/q3r_timberrun/beech_r.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/beech_g
{
	qer_editorimage models/mapobjects/q3r_timberrun/beech_g.tga
	surfaceparm nonsolid
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	cull disable
	{
		map models/mapobjects/q3r_timberrun/beech_g.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/oak
{
	qer_editorimage models/mapobjects/q3r_timberrun/oak.tga
	surfaceparm nonsolid
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	cull disable
	{
		map models/mapobjects/q3r_timberrun/oak.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/sprite_spruce
{
	qer_editorimage models/mapobjects/q3r_timberrun/sprite_spruce.tga
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	surfaceparm noimpact
	cull disable
	{
		map models/mapobjects/q3r_timberrun/sprite_spruce.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/sprite_beech_y
{
	qer_editorimage models/mapobjects/q3r_timberrun/sprite_beech_y.tga
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	surfaceparm noimpact
	cull disable
	{
		map models/mapobjects/q3r_timberrun/sprite_beech_y.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/sprite_beech_r
{
	qer_editorimage models/mapobjects/q3r_timberrun/sprite_beech_r.tga
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	surfaceparm noimpact
	cull disable
	{
		map models/mapobjects/q3r_timberrun/sprite_beech_r.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/sprite_beech_g
{
	qer_editorimage models/mapobjects/q3r_timberrun/sprite_beech_g.tga
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	surfaceparm noimpact
	cull disable
	{
		map models/mapobjects/q3r_timberrun/sprite_beech_g.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/sprite_oak
{
	qer_editorimage models/mapobjects/q3r_timberrun/sprite_oak.tga
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	surfaceparm noimpact
	cull disable
	{
		map models/mapobjects/q3r_timberrun/sprite_oak.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/bark
{
	qer_editorimage models/mapobjects/GR_trees/stamm.jpg
	surfaceparm nonsolid
	surfaceparm nolightmap
	{
		map models/mapobjects/GR_trees/stamm.jpg
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/oakbark
{
	qer_editorimage models/mapobjects/oak/oakstamm.jpg
	surfaceparm nonsolid
	surfaceparm nolightmap
	{
		map models/mapobjects/oak/oakstamm.jpg
		rgbGen vertex
	}
}

// curve signs (models, vertex lit by q3map2)
models/mapobjects/q3r_timberrun/sign_l
{
	qer_editorimage models/mapobjects/q3r_timberrun/sign_l.jpg
	surfaceparm nolightmap
	surfaceparm SURF_METAL
	cull disable
	{
		map models/mapobjects/q3r_timberrun/sign_l.jpg
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/sign_r
{
	qer_editorimage models/mapobjects/q3r_timberrun/sign_r.jpg
	surfaceparm nolightmap
	surfaceparm SURF_METAL
	cull disable
	{
		map models/mapobjects/q3r_timberrun/sign_r.jpg
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/sign_metal
{
	qer_editorimage textures/base_trim/tin.jpg
	surfaceparm nolightmap
	surfaceparm SURF_METAL
	cull disable
	{
		map textures/base_trim/tin.jpg
		rgbGen vertex
	}
}

// start and finish arches (models)
models/mapobjects/q3r_timberrun/truss
{
	qer_editorimage textures/base_trim/tin.jpg
	surfaceparm nolightmap
	surfaceparm SURF_METAL
	{
		map textures/base_trim/tin.jpg
		rgbGen vertex
	}
	{
		map textures/base_trim/tin.jpg
		blendFunc GL_ONE GL_ONE
		tcGen environment
		rgbGen const ( 0.18 0.18 0.18 )
	}
}

models/mapobjects/q3r_timberrun/banner_start
{
	qer_editorimage models/mapobjects/q3r_timberrun/banner_start.jpg
	surfaceparm nolightmap
	{
		map models/mapobjects/q3r_timberrun/banner_start.jpg
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/banner_finish
{
	qer_editorimage models/mapobjects/q3r_timberrun/banner_finish.jpg
	surfaceparm nolightmap
	{
		map models/mapobjects/q3r_timberrun/banner_finish.jpg
		rgbGen vertex
	}
}

models/mapobjects/q3r_timberrun/concrete
{
	qer_editorimage models/mapobjects/q3r_timberrun/concrete.jpg
	surfaceparm nolightmap
	{
		map models/mapobjects/q3r_timberrun/concrete.jpg
		rgbGen vertex
	}
}

textures/q3r_timberrun/logbark
{
	qer_editorimage textures/q3r_timberrun/logbark.jpg
	surfaceparm SURF_DIRT
	{
		map textures/q3r_timberrun/logbark.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/logend
{
	qer_editorimage textures/q3r_timberrun/logend.jpg
	surfaceparm SURF_DIRT
	{
		map textures/q3r_timberrun/logend.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/logbark_ns
{
	qer_editorimage textures/q3r_timberrun/logbark.jpg
	surfaceparm nonsolid
	surfaceparm nomarks
	{
		map textures/q3r_timberrun/logbark.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/logend_ns
{
	qer_editorimage textures/q3r_timberrun/logend.jpg
	surfaceparm nonsolid
	surfaceparm nomarks
	{
		map textures/q3r_timberrun/logend.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/logend_mark_ns
{
	qer_editorimage textures/q3r_timberrun/logend_mark.jpg
	surfaceparm nonsolid
	surfaceparm nomarks
	{
		map textures/q3r_timberrun/logend_mark.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/bale
{
	qer_editorimage textures/q3r_timberrun/bale.jpg
	surfaceparm nonsolid
	surfaceparm nomarks
	{
		map textures/q3r_timberrun/bale.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/bale_end
{
	qer_editorimage textures/q3r_timberrun/bale_end.jpg
	surfaceparm nonsolid
	surfaceparm nomarks
	{
		map textures/q3r_timberrun/bale_end.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/metal
{
	qer_editorimage textures/base_trim/tin.jpg
	surfaceparm SURF_METAL
	{
		map textures/base_trim/tin.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/trunk
{
	qer_editorimage textures/base_floor/concrete.jpg
	qer_trans 0.15
	// solid core of a tree trunk, invisible
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm detail
}

textures/q3r_timberrun/boundary
{
	qer_editorimage textures/base_floor/concrete.jpg
	qer_trans 0.15
	// invisible but SOLID; trans lets light through
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm noimpact
	surfaceparm detail
}

// ground: t_* parents on the brush sides (collision, friction); q3map2 draws
// ter_<layer> / ter_<a>to<b> from maps/q3r_timberrun_index.pcx. Layers: 0 forest, 1 gravel, 2 grass, 3 rock, 4 cliff, 5 mud

textures/q3r_timberrun/t_dirt
{
	qer_editorimage textures/q3r_timberrun/forestfloor.jpg
	q3map_indexed
	q3map_shadeAngle 120
	q3map_nonplanar
	surfaceparm SURF_DIRT
}

textures/q3r_timberrun/t_gravel
{
	qer_editorimage textures/q3r_dirt_grass/stones01.tga
	q3map_indexed
	q3map_shadeAngle 120
	q3map_nonplanar
	surfaceparm SURF_GRAVEL
}

textures/q3r_timberrun/t_grass
{
	qer_editorimage textures/q3r_dirt_grass/grass07.tga
	q3map_indexed
	q3map_shadeAngle 120
	q3map_nonplanar
	surfaceparm SURF_GRASS
}

textures/q3r_timberrun/t_rock
{
	qer_editorimage textures/q3r_dirt_grass/rock03.tga
	q3map_indexed
	q3map_shadeAngle 120
	q3map_nonplanar
}

textures/q3r_timberrun/t_wet
{
	qer_editorimage textures/q3r_dirt_grass/darkdirt02.tga
	q3map_indexed
	q3map_shadeAngle 120
	q3map_nonplanar
	surfaceparm SURF_GRAVEL
	surfaceparm SURF_WET
}

textures/q3r_timberrun/ter_0
{
	qer_editorimage textures/q3r_timberrun/forestfloor.jpg
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_timberrun/forestfloor.jpg
		tcGen vector ( 0.0039062 0 0 ) ( 0 0.0039062 0 )
		rgbGen const ( 0.95 0.90 0.84 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_1
{
	qer_editorimage textures/q3r_dirt_grass/stones01.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/stones01.tga
		tcGen vector ( 0.0062500 0 0 ) ( 0 0.0062500 0 )
		rgbGen const ( 0.92 0.88 0.80 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_2
{
	qer_editorimage textures/q3r_dirt_grass/grass07.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/grass07.tga
		tcGen vector ( 0.0031250 0 0 ) ( 0 0.0031250 0 )
		rgbGen const ( 0.86 0.82 0.62 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_3
{
	qer_editorimage textures/q3r_dirt_grass/rock03.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/rock03.tga
		tcGen vector ( 0.0034722 0 0 ) ( 0 0.0034722 0 )
		rgbGen const ( 0.80 0.78 0.74 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_4
{
	qer_editorimage textures/q3r_dirt_grass/rock03.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/rock03.tga
		tcGen vector ( 0.0022187 0.0022187 0 ) ( 0 0 0.0031250 )
		rgbGen const ( 0.76 0.74 0.70 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_5
{
	qer_editorimage textures/q3r_dirt_grass/darkdirt02.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/darkdirt02.tga
		tcGen vector ( 0.0044643 0 0 ) ( 0 0.0044643 0 )
		rgbGen const ( 0.62 0.56 0.48 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_0to1
{
	qer_editorimage textures/q3r_timberrun/forestfloor.jpg
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_timberrun/forestfloor.jpg
		tcGen vector ( 0.0039062 0 0 ) ( 0 0.0039062 0 )
		rgbGen const ( 0.95 0.90 0.84 )
	}
	{
		map textures/q3r_dirt_grass/stones01.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0062500 0 0 ) ( 0 0.0062500 0 )
		rgbGen const ( 0.92 0.88 0.80 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_0to2
{
	qer_editorimage textures/q3r_timberrun/forestfloor.jpg
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_timberrun/forestfloor.jpg
		tcGen vector ( 0.0039062 0 0 ) ( 0 0.0039062 0 )
		rgbGen const ( 0.95 0.90 0.84 )
	}
	{
		map textures/q3r_dirt_grass/grass07.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0031250 0 0 ) ( 0 0.0031250 0 )
		rgbGen const ( 0.86 0.82 0.62 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_0to3
{
	qer_editorimage textures/q3r_timberrun/forestfloor.jpg
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_timberrun/forestfloor.jpg
		tcGen vector ( 0.0039062 0 0 ) ( 0 0.0039062 0 )
		rgbGen const ( 0.95 0.90 0.84 )
	}
	{
		map textures/q3r_dirt_grass/rock03.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0034722 0 0 ) ( 0 0.0034722 0 )
		rgbGen const ( 0.80 0.78 0.74 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_0to4
{
	qer_editorimage textures/q3r_timberrun/forestfloor.jpg
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_timberrun/forestfloor.jpg
		tcGen vector ( 0.0039062 0 0 ) ( 0 0.0039062 0 )
		rgbGen const ( 0.95 0.90 0.84 )
	}
	{
		map textures/q3r_dirt_grass/rock03.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0022187 0.0022187 0 ) ( 0 0 0.0031250 )
		rgbGen const ( 0.76 0.74 0.70 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_0to5
{
	qer_editorimage textures/q3r_timberrun/forestfloor.jpg
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_timberrun/forestfloor.jpg
		tcGen vector ( 0.0039062 0 0 ) ( 0 0.0039062 0 )
		rgbGen const ( 0.95 0.90 0.84 )
	}
	{
		map textures/q3r_dirt_grass/darkdirt02.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0044643 0 0 ) ( 0 0.0044643 0 )
		rgbGen const ( 0.62 0.56 0.48 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_1to2
{
	qer_editorimage textures/q3r_dirt_grass/stones01.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/stones01.tga
		tcGen vector ( 0.0062500 0 0 ) ( 0 0.0062500 0 )
		rgbGen const ( 0.92 0.88 0.80 )
	}
	{
		map textures/q3r_dirt_grass/grass07.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0031250 0 0 ) ( 0 0.0031250 0 )
		rgbGen const ( 0.86 0.82 0.62 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_1to3
{
	qer_editorimage textures/q3r_dirt_grass/stones01.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/stones01.tga
		tcGen vector ( 0.0062500 0 0 ) ( 0 0.0062500 0 )
		rgbGen const ( 0.92 0.88 0.80 )
	}
	{
		map textures/q3r_dirt_grass/rock03.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0034722 0 0 ) ( 0 0.0034722 0 )
		rgbGen const ( 0.80 0.78 0.74 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_1to4
{
	qer_editorimage textures/q3r_dirt_grass/stones01.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/stones01.tga
		tcGen vector ( 0.0062500 0 0 ) ( 0 0.0062500 0 )
		rgbGen const ( 0.92 0.88 0.80 )
	}
	{
		map textures/q3r_dirt_grass/rock03.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0022187 0.0022187 0 ) ( 0 0 0.0031250 )
		rgbGen const ( 0.76 0.74 0.70 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_1to5
{
	qer_editorimage textures/q3r_dirt_grass/stones01.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/stones01.tga
		tcGen vector ( 0.0062500 0 0 ) ( 0 0.0062500 0 )
		rgbGen const ( 0.92 0.88 0.80 )
	}
	{
		map textures/q3r_dirt_grass/darkdirt02.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0044643 0 0 ) ( 0 0.0044643 0 )
		rgbGen const ( 0.62 0.56 0.48 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_2to3
{
	qer_editorimage textures/q3r_dirt_grass/grass07.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/grass07.tga
		tcGen vector ( 0.0031250 0 0 ) ( 0 0.0031250 0 )
		rgbGen const ( 0.86 0.82 0.62 )
	}
	{
		map textures/q3r_dirt_grass/rock03.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0034722 0 0 ) ( 0 0.0034722 0 )
		rgbGen const ( 0.80 0.78 0.74 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_2to4
{
	qer_editorimage textures/q3r_dirt_grass/grass07.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/grass07.tga
		tcGen vector ( 0.0031250 0 0 ) ( 0 0.0031250 0 )
		rgbGen const ( 0.86 0.82 0.62 )
	}
	{
		map textures/q3r_dirt_grass/rock03.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0022187 0.0022187 0 ) ( 0 0 0.0031250 )
		rgbGen const ( 0.76 0.74 0.70 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_2to5
{
	qer_editorimage textures/q3r_dirt_grass/grass07.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/grass07.tga
		tcGen vector ( 0.0031250 0 0 ) ( 0 0.0031250 0 )
		rgbGen const ( 0.86 0.82 0.62 )
	}
	{
		map textures/q3r_dirt_grass/darkdirt02.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0044643 0 0 ) ( 0 0.0044643 0 )
		rgbGen const ( 0.62 0.56 0.48 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_3to4
{
	qer_editorimage textures/q3r_dirt_grass/rock03.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/rock03.tga
		tcGen vector ( 0.0034722 0 0 ) ( 0 0.0034722 0 )
		rgbGen const ( 0.80 0.78 0.74 )
	}
	{
		map textures/q3r_dirt_grass/rock03.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0022187 0.0022187 0 ) ( 0 0 0.0031250 )
		rgbGen const ( 0.76 0.74 0.70 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_3to5
{
	qer_editorimage textures/q3r_dirt_grass/rock03.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/rock03.tga
		tcGen vector ( 0.0034722 0 0 ) ( 0 0.0034722 0 )
		rgbGen const ( 0.80 0.78 0.74 )
	}
	{
		map textures/q3r_dirt_grass/darkdirt02.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0044643 0 0 ) ( 0 0.0044643 0 )
		rgbGen const ( 0.62 0.56 0.48 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_timberrun/ter_4to5
{
	qer_editorimage textures/q3r_dirt_grass/rock03.tga
	q3map_shadeAngle 120
	q3map_lightmapSampleSize 24
	q3map_nonplanar
	{
		map textures/q3r_dirt_grass/rock03.tga
		tcGen vector ( 0.0022187 0.0022187 0 ) ( 0 0 0.0031250 )
		rgbGen const ( 0.76 0.74 0.70 )
	}
	{
		map textures/q3r_dirt_grass/darkdirt02.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcGen vector ( 0.0044643 0 0 ) ( 0 0.0044643 0 )
		rgbGen const ( 0.62 0.56 0.48 )
	}
	{
		map textures/q3r_timberrun/macro.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}
