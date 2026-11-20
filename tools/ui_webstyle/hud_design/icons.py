"""The stroke icons of the HUD design (24x24, round caps), shared by gen.py and
build_hud_icons.py."""

ICON_PATHS = {
    'gun': '<circle cx="12" cy="12" r="7"/><path d="M12 2v5M12 17v5M2 12h5M17 12h5"/>',
    'mine': '<circle cx="12" cy="13" r="6"/><path d="M12 3v4M5 6l2.5 2.5M19 6l-2.5 2.5"/>',
    'smoke': '<path d="M4 16c0-3 3-4 5-3 0-4 6-5 8-1 3 0 4 3 3 5H4z"/>',
    'box': '<rect x="4" y="6" width="16" height="13"/><path d="M4 10h16M12 6v13"/>',
    'note': '<path d="M9 18V5l11-2v13"/><circle cx="6.5" cy="18" r="2.5"/><circle cx="17.5" cy="16" r="2.5"/>',
    'flag': '<path d="M5 21V4M5 4h12l-3 4 3 4H5"/>',
    'trophy': '<path d="M7 4h10v4a5 5 0 0 1-10 0z"/><path d="M12 13v4M8 21h8M7 6H4a3 3 0 0 0 3 4M17 6h3a3 3 0 0 1-3 4"/>',
    'medal': '<circle cx="12" cy="15" r="5"/><path d="M8 3l4 7 4-7"/>',
    'chevron': '<path d="M6 15l6-6 6 6"/>',
    'nos': '<rect x="8" y="5" width="8" height="15" rx="2"/><path d="M10 2h4M12 2v3"/>',
    'msg': '<path d="M4 5h16v11H9l-5 4z"/>',
    'prev': '<path d="M6 5v14M18 5l-9 7 9 7z"/>',
    'play': '<path d="M7 5l12 7-12 7z"/>',
    'next': '<path d="M18 5v14M6 5l9 7-9 7z"/>',
    'hill': '<path d="M3 19l6-9 4 5 3-4 5 8z"/>',
    'ball': '<circle cx="12" cy="12" r="8"/><path d="M12 7l4 3-1.5 5h-5L8 10z"/>',
    'zone': '<path d="M12 3l8 4.5v9L12 21l-8-4.5v-9z"/>',
}
