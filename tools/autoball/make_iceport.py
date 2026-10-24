#!/usr/bin/env python3
"""Generate the Iceport Autoball arena map.

The field keeps the established Autoball scale and team entities, while the
dressing turns the venue into a snowed-in cargo harbour with cranes, containers
and a marked ice surface. Compile from baseq3r with:

    q3map2 -meta maps/q3r_iceport.map
    q3map2 -vis -saveprt maps/q3r_iceport.map
    q3map2 -light -fast -samples 2 -bounce 1 maps/q3r_iceport.map
"""
import os
import sys
from itertools import combinations
from math import cos, pi, sin

HALF_X = 2560
HALF_Y = 1792
WALL_HEIGHT = 512
CEILING_HEIGHT = 1600
THICK = 64
GOAL_HALF_W = 512
GOAL_H = 320
GOAL_D = 512
RAMP = 192
BALL_RADIUS = 75

ICE = "q3r_iceport/ice"
CONCRETE = "arctic/concrete4_1"
STEEL = "q3r_iceport/steel"
TRIM = "arctic/trim1_2"
RED = "q3r_iceport/container_red"
BLUE = "q3r_iceport/container_blue"
CYAN = "q3r_iceport/cyan_light"
RED_LIGHT = "q3r_iceport/red_light"
BLUE_LIGHT = "q3r_iceport/blue_light"
PAINT = "q3r_iceport/ice_paint"
ICE_MARK = "q3r_iceport/ice_mark"
TRIGGER = "common/trigger"
SKY = "arctic/arc_skybox3"


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def add(a, b, scale=1):
    return (a[0] + scale * b[0], a[1] + scale * b[1], a[2] + scale * b[2])


def face(normal, point, texture):
    return (normal, point, texture)


def box(mins, maxs, texture, side_texture=None):
    x1, y1, z1 = mins
    x2, y2, z2 = maxs
    sides = side_texture or texture
    return [
        face((0, 0, -1), (x1, y1, z1), texture),
        face((0, 0, 1), (x1, y1, z2), texture),
        face((-1, 0, 0), (x1, y1, z1), sides),
        face((1, 0, 0), (x2, y1, z1), sides),
        face((0, -1, 0), (x1, y1, z1), sides),
        face((0, 1, 0), (x1, y2, z1), sides),
    ]


def ramp_along_y(x1, x2, wall_y, sign):
    toe_y = wall_y - sign * RAMP
    return [
        face((0, 0, -1), (x1, toe_y, 0), CONCRETE),
        face((0, sign, 0), (x1, wall_y, 0), CONCRETE),
        face((0, -sign, 1), (x1, toe_y, 0), CONCRETE),
        face((-1, 0, 0), (x1, toe_y, 0), CONCRETE),
        face((1, 0, 0), (x2, toe_y, 0), CONCRETE),
    ]


def ramp_along_x(y1, y2, wall_x, sign):
    toe_x = wall_x - sign * RAMP
    return [
        face((0, 0, -1), (toe_x, y1, 0), CONCRETE),
        face((sign, 0, 0), (wall_x, y1, 0), CONCRETE),
        face((-sign, 0, 1), (toe_x, y1, 0), CONCRETE),
        face((0, -1, 0), (toe_x, y1, 0), CONCRETE),
        face((0, 1, 0), (toe_x, y2, 0), CONCRETE),
    ]


def cylinder(radius, z1, z2, texture, sides=32, center=(0, 0)):
    """A flat-topped convex disk, used for harmless ice-paint markings."""
    faces = [face((0, 0, -1), (0, 0, z1), texture),
             face((0, 0, 1), (0, 0, z2), texture)]
    for i in range(sides):
        angle = (2 * i + 1) * pi / sides
        normal = (cos(angle), sin(angle), 0)
        point = (center[0] + radius * cos(angle), center[1] + radius * sin(angle), z1)
        faces.append(face(normal, point, texture))
    return faces


def brush_vertices(faces):
    planes = [(f[0], dot(f[0], f[1])) for f in faces]
    verts = []
    for (n1, d1), (n2, d2), (n3, d3) in combinations(planes, 3):
        det = dot(n1, cross(n2, n3))
        if abs(det) < 1e-9:
            continue
        p = add(add(tuple(d1 * c for c in cross(n2, n3)),
                    tuple(d2 * c for c in cross(n3, n1))),
                tuple(d3 * c for c in cross(n1, n2)))
        p = tuple(c / det for c in p)
        if all(dot(n, p) <= d + 1e-6 for n, d in planes):
            verts.append(tuple(round(c, 3) for c in p))
    return set(verts)


def plane_points(normal, point):
    up = (0, 0, 1) if abs(normal[2]) < max(abs(normal[0]), abs(normal[1]), 1e-9) * 2 else (1, 0, 0)
    if normal[0] == 0 and normal[1] == 0:
        up = (1, 0, 0)
    t1 = cross(up, normal)
    t2 = cross(normal, t1)
    assert dot(cross(t1, t2), normal) > 0
    a = point
    return a, add(a, t2, 64), add(a, t1, 64)


def format_brush(faces, index):
    verts = brush_vertices(faces)
    if len(verts) < 4:
        raise ValueError(f"brush {index} is degenerate")
    lines = [f"// brush {index}", "{"]
    for normal, point, texture in faces:
        points = " ".join("( %d %d %d )" % tuple(int(round(v)) for v in p)
                           for p in plane_points(normal, point))
        lines.append(f"{points} {texture} 0 0 0 0.5 0.5 0 0 0")
    lines.append("}")
    return "\n".join(lines)


def entity(fields, brushes=None):
    body = "\n".join(f'"{key}" "{value}"' for key, value in fields)
    if brushes:
        body += "\n" + "\n".join(format_brush(b, i) for i, b in enumerate(brushes))
    return "{\n" + body + "\n}"


def arena_brushes():
    x, y, h, t = HALF_X, HALF_Y, WALL_HEIGHT, THICK
    g, gh, d = GOAL_HALF_W, GOAL_H, GOAL_D
    brushes = [
        box((-x - d - t, -y - t, -t), (x + d + t, y + t, 0), ICE),
        # A lower dock apron gives the venue a visible industrial foreground.
        box((-3904, -2592, -192), (3904, 2592, -64), CONCRETE),
        box((-6208, -4704, h + 1024), (6208, 4704, h + 1088), SKY),
        box((-x - t, y, 0), (x + t, y + t, h - 128), STEEL),
        box((-x - t, y, h - 128), (x + t, y + t, h), "arctic/glass1"),
        box((-x - t, -y - t, 0), (x + t, -y, h - 128), STEEL),
        box((-x - t, -y - t, h - 128), (x + t, -y, h), "arctic/glass1"),
    ]
    for sign in (1, -1):
        inner, outer = (x, x + t) if sign > 0 else (-x - t, -x)
        brushes += [
            box((inner, -y, 0), (outer, -g, h - 128), STEEL),
            box((inner, -y, h - 128), (outer, -g, h), "arctic/glass1"),
            box((inner, g, 0), (outer, y, h - 128), STEEL),
            box((inner, g, h - 128), (outer, y, h), "arctic/glass1"),
            box((inner, -g, gh), (outer, g, h), "arctic/glass1"),
        ]
        back = (x + d, x + d + t) if sign > 0 else (-x - d - t, -x - d)
        side = (x + t, x + d) if sign > 0 else (-x - d, -x - t)
        roof = (x + t, x + d + t) if sign > 0 else (-x - d - t, -x - t)
        brushes += [
            box((back[0], -g - t, 0), (back[1], g + t, gh + t), STEEL),
            box((side[0], g, 0), (side[1], g + t, gh), STEEL),
            box((side[0], -g - t, 0), (side[1], -g, gh), STEEL),
            box((roof[0], -g - t, gh), (roof[1], g + t, gh + t), STEEL),
        ]
        brushes += [ramp_along_y(-x, x, sign * y, sign),
                    ramp_along_x(-y, -g, sign * x, sign),
                    ramp_along_x(g, y, sign * x, sign)]

    # Ice paint is render-only. The three-unit decals leave the drivable floor flat.
    brushes += [
        box((-12, -HALF_Y + 24, 1), (12, HALF_Y - 24, 4), PAINT),
        box((-HALF_X + 24, -HALF_Y + 24, 1), (HALF_X - 24, -HALF_Y + 40, 4), PAINT),
        box((-HALF_X + 24, HALF_Y - 40, 1), (HALF_X - 24, HALF_Y - 24, 4), PAINT),
        cylinder(222, 1, 4, CYAN),
        cylinder(204, 4, 7, ICE_MARK),
        cylinder(38, 7, 10, CYAN, sides=20),
        cylinder(196, 1, 4, RED_LIGHT, center=(-2120, 0)),
        cylinder(178, 4, 7, ICE_MARK, center=(-2120, 0)),
        cylinder(196, 1, 4, BLUE_LIGHT, center=(2120, 0)),
        cylinder(178, 4, 7, ICE_MARK, center=(2120, 0)),
    ]
    # The centre and goal creases are all recessed visual layers, never collision lips.
    # Team goal rails and top lintels make the scoring ends readable across the ice.
    for sign, material in ((-1, RED_LIGHT), (1, BLUE_LIGHT)):
        gx = sign * HALF_X
        x1, x2 = sorted((gx - sign * 8, gx + sign * 8))
        brushes.extend([
            box((x1, -GOAL_HALF_W - 12, 2), (x2, -GOAL_HALF_W + 8, GOAL_H + 12), material),
            box((x1, GOAL_HALF_W - 8, 2), (x2, GOAL_HALF_W + 12, GOAL_H + 12), material),
            box((x1, -GOAL_HALF_W - 12, GOAL_H - 10), (x2, GOAL_HALF_W + 12, GOAL_H + 12), material),
        ])
    return brushes


def cargo_brushes():
    brushes = []
    # Colour-coded freight stacks appear above the boards on both harbour sides.
    for side in (-1, 1):
        y1, y2 = sorted((side * 1900, side * 2360))
        for i, x in enumerate((-2240, -1460, -680, 100, 880, 1660, 2440)):
            material = RED if (i + (side > 0)) % 2 == 0 else BLUE
            brushes.append(box((x - 352, y1, -64), (x + 352, y2, 480), material))
            # Door end, corner posts and a pair of pale reefer bars break up each box.
            door_x = x + (304 if side < 0 else -336)
            brushes.append(box((door_x, y1, -48), (door_x + 32, y2, 464), STEEL))
            for rib_x in (x - 272, x + 272):
                brushes.append(box((rib_x, y1 - 4, -48), (rib_x + 20, y1 + 4, 464), TRIM))
            brushes.append(box((x - 300, y1 - 5, 370), (x + 300, y1 + 5, 390), CYAN))

        # A second tier and insulated service pods frame the centre and corners.
        for x in (-1850, 1850):
            material = BLUE if x < 0 else RED
            brushes.append(box((x - 384, y1, 480), (x + 384, y2, 880), material))
            brushes.append(box((x - 300, y1 - 5, 760), (x + 300, y1 + 5, 778), TRIM))

    # Two portal gantries: posts, bridge rails, and luminous rails over the dock.
    for x in (-2944, 2944):
        for y in (-2352, 2352):
            brushes.append(box((x - 80, y - 80, -64), (x + 80, y + 80, 1320), STEEL))
        brushes.append(box((x - 112, -2432, 1280), (x + 112, 2432, 1360), STEEL))
        brushes.append(box((x - 16, -2392, 1360), (x + 16, 2392, 1380), CYAN))
    for y in (-2352, 2352):
        brushes.append(box((-2944, y - 96, 1184), (2944, y + 96, 1264), STEEL))
        brushes.append(box((-2800, y - 12, 1264), (2800, y + 12, 1284), BLUE_LIGHT))

    # Container yard markers along the apron.
    for x in (-3000, -1200, 1200, 3000):
        brushes.append(box((x - 48, -2520, -64), (x + 48, 2520, -48), TRIM))
    return brushes


def team_spawns():
    kickoff = [(-730, 640), (-730, -640), (-1550, 0), (-2150, 300)]
    respawn = [(-2200, 1100), (-2200, -1100), (-2200, 600), (-2200, -600)]
    ents = []
    for team, sign, angle in (("red", 1, 0), ("blue", -1, 180)):
        for x, y in kickoff:
            ents.append(entity([("classname", f"team_CTF_{team}player"),
                                ("origin", f"{sign * x} {sign * y} 48"), ("angle", str(angle))]))
        for x, y in respawn:
            ents.append(entity([("classname", f"team_CTF_{team}spawn"),
                                ("origin", f"{sign * x} {sign * y} 48"), ("angle", str(angle))]))
    return ents


def point_entities():
    ents = [entity([("classname", "autoball_ball"), ("origin", "0 0 100")]),
            entity([("classname", "rally_scoreboard"), ("origin", "0 0 688")])]
    for sx, angle in ((-1, 0), (1, 180)):
        for y in (-900, -300, 300, 900):
            ents.append(entity([("classname", "info_player_deathmatch"),
                                ("origin", f"{sx * 1800} {y} 48"), ("angle", str(angle))]))
    for sx in (-1, 1):
        for sy in (-1, 1):
            ents.append(entity([("classname", "rally_item_turbo"),
                                ("origin", f"{sx * (HALF_X - 360)} {sy * (HALF_Y - 360)} 48"),
                                ("count", "5000"), ("wait", "10")]))
    for x, y in ((0, 1300), (0, -1300), (-1280, 1240), (1280, 1240),
                 (-1280, -1240), (1280, -1240)):
        ents.append(entity([("classname", "rally_item_turbo"), ("origin", f"{x} {y} 48"),
                            ("count", "1500"), ("wait", "4")]))
    for x in (-1800, 0, 1800):
        for y in (-1400, 0, 1400):
            color = "0.52 0.72 1.0" if y <= 0 else "0.72 0.88 1.0"
            ents.append(entity([("classname", "light"), ("origin", f"{x} {y} 620"),
                                ("light", "1150"), ("_color", color)]))
    for sign, material in ((-1, RED_LIGHT), (1, BLUE_LIGHT)):
        ents.append(entity([("classname", "light"),
                            ("origin", f"{sign * 2700} 0 360"),
                            ("light", "850"),
                            ("_color", "1.0 0.24 0.18" if sign < 0 else "0.24 0.68 1.0")]))
    for team, sign in (("red", -1), ("blue", 1)):
        x1, x2 = sorted((sign * (HALF_X + BALL_RADIUS), sign * (HALF_X + GOAL_D)))
        trigger = box((x1, -GOAL_HALF_W, 0), (x2, GOAL_HALF_W, GOAL_H), TRIGGER)
        ents.append(entity([("classname", "autoball_goal"), ("team", team)], [trigger]))
    return ents + team_spawns()


def main():
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "baseq3r", "maps", "q3r_iceport.map")
    brushes = arena_brushes() + cargo_brushes()
    parts = ["// Generated by tools/autoball/make_iceport.py - edit the script, not this file",
             "// entity 0", "{", '"classname" "worldspawn"',
             '"message" "Iceport Arena"', '"_ambient" "18"',
             '"colorlut" "gfx/luts/cold"', '"weather" "snow"',
             '"weatherChance" "65"', '"weatherCycle" "80 150"',
             '"weatherStrength" "0.3 0.65"', '"weatherWind" "20 95"',
             '"weatherWindDir" "0"']
    for i, brush in enumerate(brushes):
        parts.append(format_brush(brush, i))
    parts.append("}")
    for i, item in enumerate(point_entities(), start=1):
        parts.extend((f"// entity {i}", item))
    with open(out, "w", newline="\n") as stream:
        stream.write("\n".join(parts) + "\n")
    print(f"Wrote {out} ({len(brushes)} world brushes)")


if __name__ == "__main__":
    main()
