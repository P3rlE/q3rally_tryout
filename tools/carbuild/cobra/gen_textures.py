"""Paint the Cobra textures and write its skins.

usage: gen_textures.py <outdir>

Run after build_model.py: it writes body.md3 and the part textures (trim,
lights from the source model, black) into the same directory. The paint
atlases follow layout.py.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, HERE)            # cobra/layout.py before ../layout.py
import layout
import md3
import paint

OUT = sys.argv[1]
ALPHA = 238                         # satin paint like the other new cars
DECALS = {
    'flank': (-14.0, 16.0, 0.5, 3.5),
    'ends': (-12.0, 14.0),
    # twin stripes over bonnet, cockpit and boot
    'stripes': [(1.2, 5.6, 'stripe'), (-5.6, -1.2, 'stripe')],
    'stripe_front': -9.0,
    'stripe_back': -7.0,
    'side_stripes': [],
    # band along the sills, only for the secret paints (secret_paints.py)
    'secret_side_stripes': [([(46.0, -8.4), (-46.0, -8.4), (-46.0, -10.2), (46.0, -10.2)], 'stripe'),
                            ([(46.0, -10.8), (-46.0, -10.8), (-46.0, -11.4), (46.0, -11.4)], 'edge')],
    'side_text': (-41.0, -3.0, 1.8),         # rear wing, behind the wheel arch
    'door_roundel': (2.0, -4.0, 4.6),         # door
    'roof_roundel': (-41.0, 0.0, 5.0),        # no roof: on the boot lid, readable from the chase cam
    'hood_text': (30.0, 2.4),
}

paint.Painter(layout, DECALS, ALPHA).paint_all(OUT)
paint.shared_parts(OUT, ('glass', 'chrome'))
# skins list the surfaces of body.md3 (build_model.py splits big groups into group2, group3, ...)
TEX = {'glass': 'glass', 'chrome': 'chrome', 'parts': 'parts', 'hlights': 'parts', 'rlights': 'parts'}
names = [s.name for s in md3.load(os.path.join(OUT, 'body.md3')).surfs]
paint.write_skins(OUT, 'models/players/cobra/', [],
                  [n for n in names if n.startswith('body_')],
                  [(n, TEX[n.rstrip('0123456789')]) for n in names if not n.startswith('body_')])
print('ok')
