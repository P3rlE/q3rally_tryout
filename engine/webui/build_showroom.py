"""Stage the shared website showroom for the embedded garage pilot."""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import zipfile

import showroom_export as exporter


ENGINE = Path(__file__).resolve().parents[1]
ASSET_ID = re.compile(r"[a-z0-9_-]{1,48}")


def label(key):
    return key.replace("_", " ").title()


def export_catalog(game, destination):
    from PIL import Image
    reference = json.loads((ENGINE / "webui/catalog_labels.json").read_text(encoding="utf-8"))
    unlocks = (ENGINE / "code/game/garage_unlocks.h").read_text(encoding="utf-8")
    cars, skins = (
        {key: (name, int(rank)) for key, name, rank in
         re.findall(r'ENTRY\( "([^"]+)", "([^"]+)", (\d+) \)', section)}
        for section in (unlocks.split("GARAGE_CAR_UNLOCKS")[1], unlocks.split("GARAGE_CAR_UNLOCKS")[0])
    )
    rank_table = (ENGINE / "code/game/profile_shared.h").read_text(encoding="utf-8")
    ranks = re.findall(r'ENTRY\(\s*"([^"]+)"', rank_table)
    models = game / "models/players"
    showroom = destination / "showroom"
    previews = destination / "previews"
    showroom.mkdir(parents=True, exist_ok=True)
    previews.mkdir(exist_ok=True)
    catalog = {"vehicles": {}, "wheels": {}, "ranks": ranks}
    materials = {"vehicles": {}, "wheels": {}}
    shaders = exporter.shader_maps(game / "scripts/cars.shader")
    wheel_surfaces = set()

    def options(folder, prefix, labels, locks):
        result = {}
        for skin in sorted(folder.glob("*.skin")):
            key = skin.stem
            if not ASSET_ID.fullmatch(key):
                continue
            icon = exporter.find_image(folder / f"icon_{key}.tga")
            if icon is None:
                continue  # The native selector also discovers skins by their icons.
            name, rank = locks.get(key, (labels.get(key, label(key)), 0))
            filename = f"{prefix}-{key}.png"
            with Image.open(icon) as image:
                image.convert("RGBA").save(previews / filename)
            result[key] = {"name": name, "image": f"previews/{filename}", "rank": rank}
        return result

    for car in sorted(models.iterdir()):
        key = car.name
        if not ASSET_ID.fullmatch(key) or not (car / "body.md3").is_file():
            continue
        vehicle = reference["vehicles"].get(key, {})
        paint_labels = {key: value["name"] for key, value in vehicle.get("paints", {}).items()}
        name, rank = cars.get(key, (vehicle.get("name", label(key)), 0))
        paints = options(car, key, paint_labels, skins)
        body, wheels = exporter.export_geometry(car / "body.md3", car / "wheel.md3", showroom / f"{key}.glb")
        wheel_surfaces.update(wheels)
        variants = exporter.export_textures(game, showroom, car, paints, body, shaders)
        if not paints or any(not variant for variant in variants.values()):
            raise ValueError(f"Missing paint textures for {key}")
        catalog["vehicles"][key] = {"name": name, "rank": rank, "paints": paints}
        materials["vehicles"][key] = variants

    catalog["wheels"] = options(models / "wheels", "wheels", reference["wheels"], {})
    materials["wheels"] = exporter.export_textures(game, showroom, models / "wheels",
                                                    catalog["wheels"], wheel_surfaces, shaders)
    if any("w_rim" not in rim for rim in materials["wheels"].values()):
        raise ValueError("Missing required wheel texture")
    catalog["heads"] = options(models / "heads", "driver", {}, {})
    catalog["plates"] = {}
    for source in sorted((models / "plates").iterdir()):
        key = source.stem.lower()
        if source.suffix.lower() not in {".tga", ".jpg", ".png"} or not ASSET_ID.fullmatch(key):
            continue
        if key.startswith("player"):
            continue
        filename = f"plate-{key}.png"
        with Image.open(source) as image:
            image.convert("RGBA").save(previews / filename)
        catalog["plates"][key] = {"name": label(key), "image": f"previews/{filename}", "rank": 0}

    for key, folder in (("avatars", "gfx/avatars"), ("achievementIcons", "menu/achievements"), ("mapShots", "levelshots")):
        catalog[key] = {}
        for source in sorted((game / folder).rglob("*")):
            if source.suffix.lower() not in {".png", ".tga", ".jpg"}:
                continue
            shader = source.relative_to(game).with_suffix("").as_posix()
            filename = key + "-" + "-".join(source.relative_to(game).with_suffix("").parts) + ".png"
            with Image.open(source) as image:
                image.convert("RGBA").save(previews / filename)
            catalog[key][shader] = "previews/" + filename

    for path, data in ((destination / "catalog.json", catalog), (showroom / "materials.json", materials)):
        path.write_text(json.dumps(data, separators=(",", ":")), encoding="utf-8")
    print(f"Exported {len(catalog['vehicles'])} vehicles, "
          f"{sum(len(car['paints']) for car in catalog['vehicles'].values())} paints, "
          f"and {len(catalog['wheels'])} wheel sets.")


def extract_assets(package, destination):
    """Only unpack VFS assets needed by the exporter; reject path traversal."""
    with zipfile.ZipFile(package) as archive:
        for member in archive.infolist():
            if member.is_dir():
                continue
            name = member.filename
            if name.startswith(("models/players/", "textures/", "scripts/", "gfx/avatars/", "menu/achievements/", "levelshots/")):
                if "\\" in name or any(part in ("", ".", "..") or ":" in part for part in name.split("/")):
                    raise ValueError(f"Unsafe asset path: {name}")
                if (member.external_attr >> 16) & 0o170000 == 0o120000:
                    raise ValueError(f"Asset symlink: {name}")
                archive.extract(member, destination)


def build(destination, game_assets):
    webui = ENGINE / "webui"
    destination.mkdir(parents=True, exist_ok=False)
    for source in (webui / "garage").iterdir():
        if source.is_file():
            shutil.copyfile(source, destination / source.name)
    fonts = destination / "fonts"
    fonts.mkdir()
    for source in (ENGINE.parent / "launcher/assets/fonts").iterdir():
        if source.suffix.lower() in {".ttf", ".txt"}:
            shutil.copyfile(source, fonts / source.name)
    with tempfile.TemporaryDirectory(prefix="q3rally-showroom-") as directory:
        game = Path(directory)
        extract_assets(game_assets, game)
        export_catalog(game, destination)
    subprocess.run([
        "node", "-e", "require('esbuild').buildSync(JSON.parse(process.argv[1]))",
        json.dumps({
            "entryPoints": [str(webui / "vehicleShowroom.ts")],
            "outfile": str(destination / "vehicleShowroom.bundle.js"),
            "bundle": True, "format": "iife", "globalName": "Q3RallyShowroom",
            "target": "chrome120", "minify": True, "legalComments": "eof",
        }),
    ], cwd=webui, check=True)
    shutil.copyfile(webui / "node_modules/three/LICENSE", destination / "THREE-LICENSE.txt")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path, help="Pilot runtime webui/garage directory")
    parser.add_argument("game_assets", type=Path, help="Built pak0.pk3 including secret skins")
    args = parser.parse_args()

    build(args.destination.resolve(), args.game_assets.resolve())


if __name__ == "__main__":
    main()
