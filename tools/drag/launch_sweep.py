#!/usr/bin/env python3
"""Build and run the Drag launch sweep against the current shared movement code.

Usage: python tools/drag/launch_sweep.py [rpm|all] [command ms] [shift rpm]

Compiles tools/drag/launch_sweep.c together with the bg_* movement sources
(as the cgame does), so the numbers always match the checked-out physics.
Needs gcc (MSYS2 MinGW on Windows).
"""
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
CODE = ROOT / "engine/code"
SOURCES = ["game/bg_pmove.c", "game/bg_physics.c", "game/bg_wheel_forces.c", "game/bg_misc.c",
           "game/bg_slidemove.c", "game/bg_weather.c", "qcommon/q_math.c", "qcommon/q_shared.c"]


def build(directory: pathlib.Path) -> pathlib.Path:
    compiler = shutil.which("gcc") or "C:/msys64/mingw64/bin/gcc.exe"
    binary = directory / ("launch_sweep.exe" if sys.platform == "win32" else "launch_sweep")
    command = [compiler, "-O2", '-DARCH_STRING="x86_64"', "-DCGAME", "-I", str(CODE),
               str(ROOT / "tools/drag/launch_sweep.c")]
    command += [str(CODE / source) for source in SOURCES]
    command += ["-o", str(binary), "-lm"]
    subprocess.run(command, check=True)
    return binary


def main() -> int:
    with tempfile.TemporaryDirectory() as directory:
        binary = build(pathlib.Path(directory))
        return subprocess.run([str(binary), *sys.argv[1:]]).returncode


if __name__ == "__main__":
    sys.exit(main())
