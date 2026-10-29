// Lantern models for the maps (misc_model): models/mapobjects/q3r_lanterns/.
// Written for tools/mapobjects/make_lanterns.py. The models are baked into the
// BSP and vertex lit by q3map2; glass, flame and lens glow by themselves (the
// light itself comes from a light entity next to the model).

models/mapobjects/q3r_lanterns/iron
{
	qer_editorimage models/mapobjects/q3r_lanterns/iron.tga
	surfaceparm nomarks
	{
		map models/mapobjects/q3r_lanterns/iron.tga
		rgbGen vertex
	}
}

models/mapobjects/q3r_lanterns/pole
{
	qer_editorimage models/mapobjects/q3r_lanterns/pole.tga
	surfaceparm nomarks
	{
		map models/mapobjects/q3r_lanterns/pole.tga
		rgbGen vertex
	}
}

models/mapobjects/q3r_lanterns/glass
{
	qer_editorimage models/mapobjects/q3r_lanterns/glass.tga
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm trans
	surfaceparm nonsolid
	cull none
	{
		map models/mapobjects/q3r_lanterns/glass.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave noise 0.88 0.12 0 5
	}
}

models/mapobjects/q3r_lanterns/flame
{
	qer_editorimage models/mapobjects/q3r_lanterns/flame.tga
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nonsolid
	{
		map models/mapobjects/q3r_lanterns/flame.tga
		rgbGen wave noise 0.9 0.1 0 7
	}
}

models/mapobjects/q3r_lanterns/lens
{
	qer_editorimage models/mapobjects/q3r_lanterns/lens.tga
	surfaceparm nolightmap
	surfaceparm nomarks
	{
		map models/mapobjects/q3r_lanterns/lens.tga
		rgbGen identity
	}
}
