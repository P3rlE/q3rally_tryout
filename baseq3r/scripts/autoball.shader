// Autoball ball (tools/autoball/make_ball.py)
// Stage 2 adds the seam/pentagon mask in the colour cgame sets on the
// entity: the team of the last car that touched the ball.
models/autoball/ball
{
	{
		map models/autoball/ball.tga
		rgbGen lightingDiffuse
	}
	{
		map models/autoball/ball_glow.tga
		blendFunc add
		rgbGen entity
	}
}

// Ball trail puffs (cg_autoball.c). Additive; cgame sets colour and fade.
autoballTrail
{
	nopicmip
	cull none
	entityMergable
	{
		map models/autoball/trail.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
		alphaGen vertex
	}
}
