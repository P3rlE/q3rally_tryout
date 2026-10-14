"""Paint the GT-R textures and write its skins.

usage: gen_textures.py <outdir>

The paint atlases follow layout.py; parts.tga (the palette of the other
materials) is written by build_model.py.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, HERE)            # gtr/layout.py before ../layout.py
import layout
import paint

OUT = sys.argv[1]
ALPHA = 238                         # satin paint like the Skyline
DECALS = {
    'flank': (-14.0, 17.0, 3.0, 6.5),
    'ends': (-11.0, 15.0),
    'stripes': [(-1.4, 7.6, 'stripe'), (-4.0, -2.6, 'edge')],
    'stripe_front': -9.0,
    'stripe_back': -7.0,
    # side stripe along the sills between the wheel arches, rising to the back
    'side_stripes': [([(24.0, -6.4), (-22.0, -5.4), (-22.0, -7.8), (24.0, -8.8)], 'stripe'),
                     ([(24.0, -9.4), (-22.0, -8.4), (-22.0, -9.1), (24.0, -10.1)], 'edge')],
    'side_text': (-33.0, 1.6, 2.0),          # rear wing, above the wheel arch
    'door_roundel': (3.0, -0.8, 5.6),
    'roof_roundel': (-6.0, 0.0, 7.0),
    'hood_text': (37.0, 2.6),
}

paint.Painter(layout, DECALS, ALPHA).paint_all(OUT)
paint.shared_parts(OUT, ('glass', 'chrome'))
paint.write_skins(OUT, 'models/players/gtr/',
                  [('glass', 'glass'), ('chrome', 'chrome'), ('parts', 'parts')],
                  ('body_fl', 'body_fr', 'body_rl', 'body_rr'),
                  [('hlights', 'parts'), ('rlights', 'parts')])
print('ok')
