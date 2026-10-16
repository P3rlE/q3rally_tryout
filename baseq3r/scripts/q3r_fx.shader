// ------------------------------------------------------------
// Q3Rally client-side map effects (cgame/cg_rally_fx.c)
// Colour and opacity come from the particle vertices.
// ------------------------------------------------------------

// rally_fx_spray: soft mist puff
gfx/misc/q3r_mist
{
	nopicmip
	cull none
	{
		map gfx/misc/q3r_mist.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}

// rally_fx_spray: water droplet
gfx/misc/q3r_droplet
{
	nopicmip
	cull none
	{
		map gfx/misc/q3r_droplet.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}
