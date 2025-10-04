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
from typing import Dict, Iterable, List, Optional, Set, Tuple

import tkinter as tk
from tkinter import filedialog
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


GAMETYPE_HOME_SUFFIX: Dict[str, str] = {
    "0": "Q3Rally-Racing",
    "1": "Q3Rally-RacingDM",
    "2": "Q3Rally-SP",
    "3": "Q3Rally-DemolitionDerby",
    "4": "Q3Rally-LCS",
    "5": "Q3Rally-Elimination",
    "6": "Q3Rally-DM",
    "7": "Q3Rally-TDM",
    "8": "Q3Rally-TeamRacing",
    "9": "Q3Rally-TRDM",
    "10": "Q3Rally-CTF",
    "11": "Q3Rally-CTF4",
    "12": "Q3Rally-Domination",
}


GAMETYPE_CONFIG_SLUG: Dict[str, str] = {
    "0": "racing",
    "1": "racing-dm",
    "2": "sp",
    "3": "derby",
    "4": "lcs",
    "5": "elimination",
    "6": "dm",
    "7": "tdm",
    "8": "teamracing",
    "9": "trdm",
    "10": "ctf",
    "11": "ctf4",
    "12": "domination",
}


GAMETYPE_TOKENS: Dict[str, Set[str]] = {
    "0": {"q3r_racing"},
    "1": {"q3r_racing_dm"},
    "2": {"q3r_single", "q3r_racing"},
    "3": {"q3r_derby"},
    "4": {"q3r_lcs"},
    "5": {"q3r_elimination"},
    "6": {"q3r_dm"},
    "7": {"q3r_team_dm"},
    "8": {"q3r_team_racing"},
    "9": {"q3r_team_racing_dm"},
    "10": {"q3r_ctf"},
    "11": {"q3r_ctf4"},
    "12": {"q3r_dom"},
}


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
        self.config_name = tk.StringVar()
        self.hostname = tk.StringVar(value="My Q3Rally Server")
        self.motd = tk.StringVar(value="Welcome to Q3Rally!")
        self.rcon_password = tk.StringVar()
        self.selected_gametype = tk.StringVar(value=GAMETYPES[0][0])
        self.selected_map = tk.StringVar()
        self.map_playlist: List[str] = []
        self.max_clients = tk.StringVar(value="16")
        self.time_limit = tk.StringVar(value="0")
        self.frag_limit = tk.StringVar(value="0")
        self.bot_min_players = tk.StringVar(value="0")

        self.enable_bots = tk.BooleanVar(value=False)
        self.allow_downloads = tk.BooleanVar(value=False)
        self.enable_ladder = tk.BooleanVar(value=True)

        self.process: Optional[subprocess.Popen[str]] = None
        self.map_catalog: Dict[str, Set[str]] = {}

        self.home_path_overridden = False
        self.config_name_overridden = False
        self._last_auto_home: Optional[str] = None
        self._last_auto_config: Optional[str] = None

        self._build_ui()
        self._populate_maps()
        self._update_gametype_defaults()

    # ------------------------------------------------------------------ UI --
    def _build_ui(self) -> None:
        frame = ttk.Frame(self, padding=10)
        frame.grid(row=0, column=0, sticky="nsew")

        row = 0
        frame.columnconfigure(1, weight=1)
        frame.columnconfigure(2, weight=0)

        def add_labeled_entry(label: str, text_var: tk.StringVar) -> None:
            nonlocal row
            ttk.Label(frame, text=label).grid(row=row, column=0, sticky="w", pady=2)
            entry = ttk.Entry(frame, textvariable=text_var, width=40)
            entry.grid(row=row, column=1, sticky="ew", pady=2)
            row += 1

        ttk.Label(
            frame, text="Installationspfad (fs_basepath):"
        ).grid(row=row, column=0, sticky="w", pady=2)
        base_entry = ttk.Entry(frame, textvariable=self.base_path, width=40)
        base_entry.grid(row=row, column=1, sticky="ew", pady=2)
        base_entry.bind("<FocusOut>", lambda _event: self._populate_maps())
        base_entry.bind("<Return>", lambda _event: self._populate_maps())
        ttk.Button(
            frame,
            text="Auswählen…",
            command=self._select_base_path,
            width=12,
        ).grid(row=row, column=2, sticky="ew", padx=(5, 0), pady=2)
        row += 1
        ttk.Label(
            frame, text="Spieler-Datenpfad (fs_homepath):"
        ).grid(row=row, column=0, sticky="w", pady=2)
        home_entry = ttk.Entry(frame, textvariable=self.home_path, width=40)
        home_entry.grid(row=row, column=1, sticky="ew", pady=2)
        home_entry.bind("<Key>", self._mark_home_override)
        row += 1

        ttk.Label(frame, text="Konfigurationsname:").grid(
            row=row, column=0, sticky="w", pady=2
        )
        config_entry = ttk.Entry(frame, textvariable=self.config_name, width=40)
        config_entry.grid(row=row, column=1, sticky="ew", pady=2)
        config_entry.bind("<Key>", self._mark_config_override)
        row += 1
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
        ttk.Button(
            frame,
            text="Zur Liste hinzufügen",
            command=self._add_map_to_playlist,
            width=18,
        ).grid(row=row, column=2, sticky="ew", padx=(5, 0), pady=2)
        row += 1

        ttk.Label(frame, text="Mapliste:").grid(row=row, column=0, sticky="nw", pady=2)
        playlist_frame = ttk.Frame(frame)
        playlist_frame.grid(row=row, column=1, columnspan=2, sticky="ew", pady=2)
        playlist_frame.columnconfigure(0, weight=1)

        self.map_listbox = tk.Listbox(
            playlist_frame,
            height=6,
            selectmode=tk.EXTENDED,
            exportselection=False,
        )
        self.map_listbox.grid(row=0, column=0, sticky="nsew")
        list_scroll = ttk.Scrollbar(
            playlist_frame, orient="vertical", command=self.map_listbox.yview
        )
        list_scroll.grid(row=0, column=1, sticky="ns")
        self.map_listbox.configure(yscrollcommand=list_scroll.set)

        button_box = ttk.Frame(playlist_frame)
        button_box.grid(row=0, column=2, padx=(5, 0), sticky="n")
        ttk.Button(
            button_box, text="Entfernen", command=self._remove_selected_maps, width=14
        ).grid(row=0, column=0, pady=(0, 2))
        ttk.Button(
            button_box, text="Nach oben", command=self._move_selected_up, width=14
        ).grid(row=1, column=0, pady=2)
        ttk.Button(
            button_box, text="Nach unten", command=self._move_selected_down, width=14
        ).grid(row=2, column=0, pady=2)
        row += 1

        add_labeled_entry("Max. Spieler:", self.max_clients)
        add_labeled_entry("Zeitlimit (Minuten):", self.time_limit)
        add_labeled_entry("Fraglimit:", self.frag_limit)
        add_labeled_entry("Bots auffüllen bis:", self.bot_min_players)

        ttk.Checkbutton(
            frame, text="Bots aktivieren", variable=self.enable_bots
        ).grid(row=row, column=0, columnspan=3, sticky="w", pady=2)
        row += 1
        ttk.Checkbutton(
            frame,
            text="Downloads erlauben",
            variable=self.allow_downloads,
        ).grid(row=row, column=0, columnspan=3, sticky="w", pady=2)
        row += 1
        ttk.Checkbutton(
            frame,
            text="Ladder-Integration aktivieren",
            variable=self.enable_ladder,
        ).grid(row=row, column=0, columnspan=3, sticky="w", pady=2)
        row += 1
        button_frame = ttk.Frame(frame)
        button_frame.grid(row=row, column=0, columnspan=3, pady=(8, 4), sticky="ew")
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
        self.log.grid(row=row, column=0, columnspan=3, sticky="nsew", pady=(4, 0))

    # ------------------------------------------------------------- callbacks --
    def _on_gametype_selected(self, _event: tk.Event) -> None:
        label = self.gametype_combo.get()
        value = label.split("\u2013", 1)[0].strip()
        self.selected_gametype.set(value)
        self._update_gametype_defaults()
        self._update_map_options()

    def _populate_maps(self) -> None:
        base_dir = Path(self.base_path.get()).expanduser()
        map_dir = base_dir / "baseq3r" / "maps"
        self.map_catalog = {}

        if not map_dir.is_dir():
            self._update_map_options()
            return

        map_names = sorted({bsp.stem for bsp in map_dir.glob("*.bsp")})
        lookup: Dict[str, str] = {}
        for name in map_names:
            self.map_catalog[name] = set()
            lookup[name.lower()] = name

        arena_dir = base_dir / "baseq3r" / "scripts"
        if arena_dir.is_dir():
            for arena_file in arena_dir.glob("*.arena"):
                try:
                    content = arena_file.read_text(encoding="utf-8")
                except UnicodeDecodeError:
                    content = arena_file.read_text(encoding="latin-1", errors="ignore")
                for map_name, tokens in self._parse_arena_blocks(content):
                    key = lookup.get(map_name.lower())
                    if key is not None:
                        self.map_catalog[key].update(tokens)

        self._update_map_options()
        self._update_gametype_defaults()

    def _select_base_path(self) -> None:
        current = Path(self.base_path.get()).expanduser()
        initial = current if current.is_dir() else Path.home()
        selection = filedialog.askdirectory(
            parent=self,
            title="Installationspfad auswählen",
            initialdir=str(initial),
        )
        if selection:
            self.base_path.set(selection)
            self._populate_maps()

    def _update_map_options(self) -> None:
        tokens = GAMETYPE_TOKENS.get(self.selected_gametype.get(), set())
        if tokens:
            available = sorted(
                map_name
                for map_name, map_tokens in self.map_catalog.items()
                if map_tokens & tokens
            )
        else:
            available = sorted(self.map_catalog)

        current = self.selected_map.get()
        if available:
            if current not in available:
                self.selected_map.set(available[0])
        else:
            self.selected_map.set("")

        self.map_combo["values"] = available
        self._sync_playlist_with_available(available)

    def _update_gametype_defaults(self) -> None:
        gametype = self.selected_gametype.get()

        suffix = GAMETYPE_HOME_SUFFIX.get(gametype)
        base_home = default_homepath(self.system_name)
        if suffix:
            proposed_home = str((base_home.parent / suffix).resolve())
        else:
            proposed_home = str(base_home)

        if (
            not self.home_path_overridden
            or self.home_path.get() == (self._last_auto_home or "")
        ):
            self.home_path.set(proposed_home)
            self._last_auto_home = proposed_home

        slug = GAMETYPE_CONFIG_SLUG.get(gametype)
        if slug:
            proposed_config = f"{slug}-serverconfig.cfg"
        else:
            proposed_config = "server_auto.cfg"

        if (
            not self.config_name_overridden
            or self.config_name.get() == (self._last_auto_config or "")
        ):
            self.config_name.set(proposed_config)
            self._last_auto_config = proposed_config

    def _mark_home_override(self, _event: tk.Event) -> None:
        self.home_path_overridden = True

    def _mark_config_override(self, _event: tk.Event) -> None:
        self.config_name_overridden = True

    @staticmethod
    def _parse_arena_blocks(text: str) -> Iterable[Tuple[str, Set[str]]]:
        for section in text.split("{"):
            if "}" not in section:
                continue
            block = section.split("}", 1)[0]
            map_name: Optional[str] = None
            tokens: Set[str] = set()
            for raw_line in block.splitlines():
                line = raw_line.strip()
                if not line or line.startswith("//"):
                    continue
                if line.startswith("map"):
                    value = ServerLauncher._extract_quoted_value(line)
                    if value:
                        map_name = value
                elif line.startswith("type"):
                    value = ServerLauncher._extract_quoted_value(line)
                    if value:
                        tokens.update(value.split())
            if map_name:
                yield map_name, tokens

    @staticmethod
    def _extract_quoted_value(line: str) -> Optional[str]:
        if "\"" not in line:
            return None
        parts = line.split("\"")
        if len(parts) >= 3:
            return parts[1].strip()
        return None


    # ----------------------------------------------------------- utilities --
    def append_log(self, text: str) -> None:
        self.log.configure(state="normal")
        self.log.insert("end", text + "\n")
        self.log.see("end")
        self.log.configure(state="disabled")

    def _refresh_map_listbox(self) -> None:
        self.map_listbox.delete(0, tk.END)
        for entry in self.map_playlist:
            self.map_listbox.insert(tk.END, entry)

    def _add_map_to_playlist(self) -> None:
        candidate = self.selected_map.get().strip()
        if not candidate:
            return
        if candidate not in self.map_combo["values"]:
            return
        if candidate in self.map_playlist:
            return
        self.map_playlist.append(candidate)
        self._refresh_map_listbox()

    def _remove_selected_maps(self) -> None:
        selection = sorted(self.map_listbox.curselection(), reverse=True)
        if not selection:
            return
        for index in selection:
            try:
                del self.map_playlist[index]
            except IndexError:
                continue
        self._refresh_map_listbox()

    def _move_selected_up(self) -> None:
        selection = self.map_listbox.curselection()
        if not selection:
            return
        indices = list(selection)
        if 0 in indices:
            return
        for index in indices:
            self.map_playlist[index - 1], self.map_playlist[index] = (
                self.map_playlist[index],
                self.map_playlist[index - 1],
            )
        self._refresh_map_listbox()
        self._reselect_indices([i - 1 for i in indices])

    def _move_selected_down(self) -> None:
        selection = self.map_listbox.curselection()
        if not selection:
            return
        indices = list(selection)
        if len(self.map_playlist) - 1 in indices:
            return
        for index in reversed(indices):
            self.map_playlist[index + 1], self.map_playlist[index] = (
                self.map_playlist[index],
                self.map_playlist[index + 1],
            )
        self._refresh_map_listbox()
        self._reselect_indices([i + 1 for i in indices])

    def _reselect_indices(self, indices: List[int]) -> None:
        self.map_listbox.selection_clear(0, tk.END)
        for index in indices:
            self.map_listbox.selection_set(index)
        if indices:
            self.map_listbox.see(indices[0])

    def _sync_playlist_with_available(self, available: Iterable[str]) -> None:
        allowed = set(available)
        if not allowed and not self.map_playlist:
            return
        filtered = [entry for entry in self.map_playlist if entry in allowed]
        if filtered != self.map_playlist:
            self.map_playlist = filtered
            self._refresh_map_listbox()

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
        if self.map_playlist:
            total = len(self.map_playlist)
            for idx, map_name in enumerate(self.map_playlist, start=1):
                next_idx = idx + 1 if idx < total else 1
                config.append(
                    f'set d{idx} "map {map_name}; set nextmap vstr d{next_idx}"'
                )
            config.append("vstr d1")
        else:
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
            "2",
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

