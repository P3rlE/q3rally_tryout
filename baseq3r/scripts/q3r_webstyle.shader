// Backgrounds of the native screens that look like the WebUI
// (tools/ui_webstyle/build_ui_art.py, q3_ui/ui_rally_webstyle.c).

// The UI passes the pixel position / 13 as texture coordinates; the transform
// projects it on the 132deg gradient line, s counts 13px periods.
menu/ui/web_stripes
{
	nopicmip
	nomipmaps
	{
		map menu/ui/web_stripes.png
		tcMod transform 0.7431448 0 0.6691306 0 0 0
		rgbGen vertex
	}
}

menu/ui/web_showroom
{
	nopicmip
	nomipmaps
	{
		clampmap menu/ui/web_showroom.png
		rgbGen vertex
	}
}

menu/ui/web_progress_track
{
	nopicmip
	nomipmaps
	{
		clampmap menu/ui/web_progress_track.png
		rgbGen vertex
	}
}

menu/ui/web_progress_fill
{
	nopicmip
	nomipmaps
	{
		clampmap menu/ui/web_progress_fill.png
		rgbGen vertex
	}
}

menu/ui/web_scrollbar
{
	nopicmip
	nomipmaps
	{
		clampmap menu/ui/web_scrollbar.png
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}
