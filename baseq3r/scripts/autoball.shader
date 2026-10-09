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
