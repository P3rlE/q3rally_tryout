// Drag Race maps (tools/maptools/make_dragstrip.py). Generated: edit the script.

// skies: the community skyboxes with a sun, sky fill light and haze for these venues
textures/q3r_drag/sky_desert
{
	qer_editorimage env/ame_desert/desertsky_ft.tga
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_globaltexture
	q3map_sun 1 0.93 0.82 190 230 35
	q3map_skylight 90 4
	q3gl2_sun 1 0.93 0.82 140 230 35 0.5
	skyparms env/ame_desert/desertsky - -
	q3r_haze 0.70 0.64 0.56 14000	// Q3Rally: dust over the desert
	q3r_colorLut gfx/luts/warm
}

textures/q3r_drag/sky_night
{
	qer_editorimage env/ely_darkcity/darkcity_ft.tga
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_globaltexture
	q3map_sun 0.55 0.6 0.9 25 230 40
	q3map_skylight 20 3
	skyparms env/ely_darkcity/darkcity - -
	q3r_haze 0.07 0.07 0.13 9000	// Q3Rally: night over the city
	q3r_colorLut gfx/luts/neutral
}

textures/q3r_drag/sky_sunset
{
	qer_editorimage env/ely_sunset/sunset_ft.tga
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_globaltexture
	q3map_sun 1 0.68 0.45 170 350 12
	q3map_skylight 90 4
	q3gl2_sun 1 0.68 0.45 120 350 12 0.6
	skyparms env/ely_sunset/sunset - -
	q3r_haze 0.62 0.50 0.46 11000	// Q3Rally: evening haze over the airfield
	q3r_colorLut gfx/luts/dusk
}

// runway concrete of the airfield: the same asphalt surface (grip) as every drag lane
textures/q3r_drag/runway
{
	qer_editorimage textures/q3r_drag/runway.jpg
	surfaceparm SURF_ASPHALT
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/q3r_drag/runway.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

// rubber from burnouts and launches: painted on, no collision, same surface
textures/q3r_drag/rubber
{
	qer_editorimage textures/q3r_drag/rubber.jpg
	surfaceparm SURF_ASPHALT
	surfaceparm nonsolid
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/q3r_drag/rubber.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_drag/paint_yellow
{
	qer_editorimage textures/q3r_drag/paint_yellow.jpg
	surfaceparm SURF_ASPHALT
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/q3r_drag/paint_yellow.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_drag/paint_red
{
	qer_editorimage textures/q3r_drag/paint_red.jpg
	surfaceparm SURF_ASPHALT
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/q3r_drag/paint_red.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

// spectators standing at the barriers (cut-out rows of people)
textures/q3r_drag/crowdrow
{
	qer_editorimage textures/q3r_people/peeprow01.tga
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	{
		map textures/q3r_people/peeprow01.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

textures/q3r_drag/crowdrow2
{
	qer_editorimage textures/q3r_people/peeprow02.tga
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	{
		map textures/q3r_people/peeprow02.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

// timing board: lit from inside
textures/q3r_drag/board
{
	qer_editorimage textures/q3r_drag/board.jpg
	surfaceparm nomarks
	surfaceparm nolightmap
	q3map_surfacelight 40
	{
		map textures/q3r_drag/board.jpg
		rgbGen identity
	}
}

// lamp lenses (the light itself comes from light entities)
textures/q3r_drag/lamp
{
	qer_editorimage textures/q3r_drag/lamp.jpg
	surfaceparm nomarks
	surfaceparm nolightmap
	{
		map textures/q3r_drag/lamp.jpg
		rgbGen identity
	}
}

textures/q3r_drag/lamp_amber
{
	qer_editorimage textures/q3r_drag/lamp_amber.jpg
	surfaceparm nomarks
	surfaceparm nolightmap
	{
		map textures/q3r_drag/lamp_amber.jpg
		rgbGen identity
	}
}

textures/q3r_drag/lamp_blue
{
	qer_editorimage textures/q3r_drag/lamp_blue.jpg
	surfaceparm nomarks
	surfaceparm nolightmap
	{
		map textures/q3r_drag/lamp_blue.jpg
		rgbGen identity
	}
}

// invisible solid hull (the vehicle physics only takes solid brushes)
textures/q3r_drag/clip
{
	qer_trans 0.15
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm detail
}
