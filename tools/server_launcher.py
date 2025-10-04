"""Interactive Q3Rally dedicated server starter.

This helper provides a tiny Tkinter GUI that is able to:

* Detect the current operating system and suggest a matching ``fs_homepath``.
* Collect a couple of frequently used server settings by means of text
  entries, checkboxes and drop-down lists.
* Write a configuration file under the selected homepath.
* Determine a plausible dedicated server binary and launch it with the
  generated configuration.

The script intentionally sticks to the Python standard library in order to
keep its dependencies small.  It is therefore well suited for quick testing
on Windows, macOS and Linux.
"""

from __future__ import annotations

import os
import platform
import subprocess
import sys
import threading
from pathlib import Path
from typing import Iterable, List, Optional, Tuple

import tkinter as tk
from tkinter import messagebox
from tkinter import scrolledtext
from tkinter import ttk


GAMETYPES: List[Tuple[str, str]] = [
    ("0", "Racing"),
    ("1", "Racing Deathmatch"),
    ("2", "Single Player"),
    ("3", "Demolition Derby"),
    ("4", "Last Car Standing"),
    ("5", "Elimination"),
    ("6", "Deathmatch"),
    ("7", "Team Deathmatch"),
    ("8", "Team Racing"),
    ("9", "Team Racing Deathmatch"),
    ("10", "Capture the Flag"),
    ("11", "Four-Team Capture the Flag"),
    ("12", "Domination"),
]


def default_homepath(system_name: str) -> Path:
    """Return a sensible default ``fs_homepath`` for *system_name*."""

    home = Path.home()
    if system_name == "windows":
        appdata = os.environ.get("APPDATA")
        if appdata:
            return Path(appdata) / "Q3Rally"
        return home / "AppData" / "Roaming" / "Q3Rally"
    if system_name == "darwin":
        return home / "Library" / "Application Support" / "Q3Rally"
    return home / ".q3rally"


def find_binary(base_path: Path, system_name: str) -> Optional[Path]:
    """Try to locate a dedicated server binary within *base_path*.

    The function checks a set of known filenames as well as the typical build
    folders created by the ioquake3/q3rally build system.
    """

    if system_name == "windows":
        candidates = [
            "q3rally-server.x86_64.exe",
            "q3rally-server.exe",
            "q3rally.x86_64.exe",
            "q3rally.exe",
        ]
    elif system_name == "darwin":
        candidates = [
            "q3rally-server.arm64",
            "q3rally-server.x86_64",
            "q3rally.x86_64",
            "q3rally.arm64",
        ]
    else:
        candidates = [
            "q3rally-server.x86_64",
            "q3rally-server.x86",
            "q3rally-server",
            "q3rally.x86_64",
        ]

    search_roots: Iterable[Path] = [
        base_path,
        base_path / "engine" / "build",
        base_path / "engine",
    ]

    for root in search_roots:
        if not root.exists():
            continue
        for candidate in candidates:
            match = next(root.glob(f"**/{candidate}"), None)
            if match is not None:
                return match
    return None


class ServerLauncher(tk.Tk):
    """Small Tkinter window that collects server options and starts it."""

    def __init__(self) -> None:
        super().__init__()
        self.title("Q3Rally Dedicated Server")
        self.resizable(width=False, height=False)

        self.system_name = platform.system().lower()
        self.base_path = tk.StringVar(
            value=str(Path(__file__).resolve().parents[1])
        )
        self.home_path = tk.StringVar(
            value=str(default_homepath(self.system_name))
        )
        self.config_name = tk.StringVar(value="server_auto.cfg")
        self.hostname = tk.StringVar(value="My Q3Rally Server")
        self.motd = tk.StringVar(value="Welcome to Q3Rally!")
        self.rcon_password = tk.StringVar()
        self.selected_gametype = tk.StringVar(value=GAMETYPES[0][0])
        self.selected_map = tk.StringVar()
        self.max_clients = tk.StringVar(value="16")
        self.time_limit = tk.StringVar(value="0")
        self.frag_limit = tk.StringVar(value="0")
        self.bot_min_players = tk.StringVar(value="0")

        self.enable_bots = tk.BooleanVar(value=False)
        self.allow_downloads = tk.BooleanVar(value=False)
        self.enable_ladder = tk.BooleanVar(value=True)
        self.pure_server = tk.BooleanVar(value=False)

        self.process: Optional[subprocess.Popen[str]] = None

        self._build_ui()
        self._populate_maps()

    # ------------------------------------------------------------------ UI --
    def _build_ui(self) -> None:
        frame = ttk.Frame(self, padding=10)
        frame.grid(row=0, column=0, sticky="nsew")

        row = 0
        frame.columnconfigure(1, weight=1)

        def add_labeled_entry(label: str, text_var: tk.StringVar) -> None:
            nonlocal row
            ttk.Label(frame, text=label).grid(row=row, column=0, sticky="w", pady=2)
            entry = ttk.Entry(frame, textvariable=text_var, width=40)
            entry.grid(row=row, column=1, sticky="ew", pady=2)
            row += 1

        add_labeled_entry("Installationspfad (fs_basepath):", self.base_path)
        add_labeled_entry("Spieler-Datenpfad (fs_homepath):", self.home_path)
        add_labeled_entry("Konfigurationsname:", self.config_name)
        add_labeled_entry("Servername:", self.hostname)
        add_labeled_entry("MOTD:", self.motd)
        add_labeled_entry("RCon Passwort:", self.rcon_password)

        ttk.Label(frame, text="Spieltyp:").grid(
            row=row, column=0, sticky="w", pady=2
        )
        gametype_values = [f"{value} – {label}" for value, label in GAMETYPES]
        self.gametype_combo = ttk.Combobox(
            frame,
            values=gametype_values,
            textvariable=tk.StringVar(value=gametype_values[0]),
            state="readonly",
            width=38,
        )
        self.gametype_combo.grid(row=row, column=1, sticky="ew", pady=2)
        self.gametype_combo.bind("<<ComboboxSelected>>", self._on_gametype_selected)
        row += 1

        ttk.Label(frame, text="Startkarte:").grid(row=row, column=0, sticky="w", pady=2)
        self.map_combo = ttk.Combobox(
            frame,
            values=[],
            textvariable=self.selected_map,
            width=38,
        )
        self.map_combo.grid(row=row, column=1, sticky="ew", pady=2)
        row += 1

        add_labeled_entry("Max. Spieler:", self.max_clients)
        add_labeled_entry("Zeitlimit (Minuten):", self.time_limit)
        add_labeled_entry("Fraglimit:", self.frag_limit)
        add_labeled_entry("Bots auffüllen bis:", self.bot_min_players)

        ttk.Checkbutton(
            frame, text="Bots aktivieren", variable=self.enable_bots
        ).grid(row=row, column=0, columnspan=2, sticky="w", pady=2)
        row += 1
        ttk.Checkbutton(
            frame,
            text="Downloads erlauben",
            variable=self.allow_downloads,
        ).grid(row=row, column=0, columnspan=2, sticky="w", pady=2)
        row += 1
        ttk.Checkbutton(
            frame,
            text="Ladder-Integration aktivieren",
            variable=self.enable_ladder,
        ).grid(row=row, column=0, columnspan=2, sticky="w", pady=2)
        row += 1
        ttk.Checkbutton(
            frame,
            text="Reiner Server (sv_pure)",
            variable=self.pure_server,
        ).grid(row=row, column=0, columnspan=2, sticky="w", pady=2)
        row += 1

        button_frame = ttk.Frame(frame)
        button_frame.grid(row=row, column=0, columnspan=2, pady=(8, 4), sticky="ew")
        ttk.Button(button_frame, text="Server starten", command=self.start_server).grid(
            row=0, column=0, padx=(0, 5)
        )
        ttk.Button(button_frame, text="Server stoppen", command=self.stop_server).grid(
            row=0, column=1
        )
        row += 1

        self.log = scrolledtext.ScrolledText(
            frame, width=60, height=12, state="disabled"
        )
        self.log.grid(row=row, column=0, columnspan=2, sticky="nsew", pady=(4, 0))

    # ------------------------------------------------------------- callbacks --
    def _on_gametype_selected(self, _event: tk.Event) -> None:
        label = self.gametype_combo.get()
        value = label.split("\u2013", 1)[0].strip()
        self.selected_gametype.set(value)

    def _populate_maps(self) -> None:
        base_dir = Path(self.base_path.get())
        map_dir = base_dir / "baseq3r" / "maps"
        maps = sorted(
            {
                bsp.stem
                for bsp in map_dir.glob("*.bsp")
            }
        )
        if maps:
            self.map_combo["values"] = maps
            self.map_combo.set(maps[0])

    # ----------------------------------------------------------- utilities --
    def append_log(self, text: str) -> None:
        self.log.configure(state="normal")
        self.log.insert("end", text + "\n")
        self.log.see("end")
        self.log.configure(state="disabled")

    def build_config(self) -> List[str]:
        config = [
            f'set sv_hostname "{self.hostname.get()}"',
            f'set g_gametype "{self.selected_gametype.get()}"',
            f'set sv_maxClients "{self.max_clients.get()}"',
            f'set timelimit "{self.time_limit.get()}"',
            f'set fraglimit "{self.frag_limit.get()}"',
            f'set g_motd "{self.motd.get()}"',
            f'set rconPassword "{self.rcon_password.get()}"',
        ]
        if self.enable_bots.get():
            config.extend(
                [
                    "set bot_enable \"1\"",
                    f'set bot_minplayers "{self.bot_min_players.get()}"',
                ]
            )
        else:
            config.append('set bot_enable "0"')
        config.append(
            f'set sv_allowdownload "{int(self.allow_downloads.get())}"'
        )
        config.append(
            f'set sv_ladderEnabled "{int(self.enable_ladder.get())}"'
        )
        config.append(f'set sv_pure "{int(self.pure_server.get())}"')
        selected_map = self.selected_map.get().strip()
        if selected_map:
            config.append(f'map {selected_map}')
        return config

    def start_server(self) -> None:
        if self.process and self.process.poll() is None:
            messagebox.showinfo(
                "Server läuft",
                "Der Server ist bereits gestartet.",
            )
            return

        base_path = Path(self.base_path.get()).expanduser().resolve()
        home_path = Path(self.home_path.get()).expanduser().resolve()
        config_lines = self.build_config()
        config_name = self.config_name.get().strip() or "server_auto.cfg"
        config_path = home_path / "baseq3r" / config_name

        try:
            config_path.parent.mkdir(parents=True, exist_ok=True)
            config_path.write_text("\n".join(config_lines) + "\n", encoding="utf-8")
        except OSError as exc:
            messagebox.showerror(
                "Fehler",
                f"Konfigurationsdatei konnte nicht erstellt werden:\n{exc}",
            )
            return

        binary = find_binary(base_path, self.system_name)
        if binary is None:
            messagebox.showerror(
                "Fehler",
                "Keine Server-Binärdatei gefunden. Bitte den Installationspfad prüfen.",
            )
            return

        command = [
            str(binary),
            "+set",
            "dedicated",
            "1",
            "+set",
            "fs_basepath",
            str(base_path),
            "+set",
            "fs_homepath",
            str(home_path),
            "+exec",
            config_name,
        ]

        self.append_log("Starte: " + " ".join(command))

        try:
            self.process = subprocess.Popen(
                command,
                cwd=str(base_path),
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
        except OSError as exc:
            messagebox.showerror(
                "Fehler",
                f"Server konnte nicht gestartet werden:\n{exc}",
            )
            return

        threading.Thread(target=self._stream_output, daemon=True).start()

    def _stream_output(self) -> None:
        assert self.process is not None
        if self.process.stdout is None:
            return
        for line in self.process.stdout:
            self.append_log(line.rstrip())
        code = self.process.wait()
        self.append_log(f"Server beendet (Exit-Code {code}).")

    def stop_server(self) -> None:
        if not self.process or self.process.poll() is not None:
            self.append_log("Kein laufender Server zu stoppen.")
            return
        self.append_log("Stoppe Server ...")
        self.process.terminate()


def main() -> int:
    if "tkinter" not in sys.modules:
        # Import already happens at module level; the check keeps static analysers
        # happy and provides a clear error message when tkinter is missing.
        raise RuntimeError("tkinter konnte nicht geladen werden.")
    app = ServerLauncher()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

