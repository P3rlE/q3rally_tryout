"""Pydantic schemas for request and response payloads."""
from __future__ import annotations

from datetime import datetime
import math
from typing import Any, Literal, Optional, get_args

from pydantic import BaseModel, Field, HttpUrl, root_validator, validator


Gametype = Literal[
    "GT_RACING",
    "GT_RACING_DM",
    "GT_SPRINT",
    "GT_DRAG",
    "GT_TEAM_RACING",
    "GT_TEAM_RACING_DM",
    "GT_ELIMINATION",
    "GT_LCS",
    "GT_DERBY",
    "GT_DEATHMATCH",
    "GT_TEAM",
    "GT_CTF",
    "GT_CTF4",
    "GT_DOMINATION",
    "GT_SINGLE_PLAYER",
]


_VALID_GAMETYPES: set[str] = set(get_args(Gametype))
_TEAM_MODES: set[str] = {
    "GT_TEAM_RACING",
    "GT_TEAM_RACING_DM",
    "GT_TEAM",
    "GT_CTF",
    "GT_CTF4",
    "GT_DOMINATION",
}


# A Drag launch faster than this after green is a false start in the game.
DRAG_MIN_REACTION_MS = 100


def _mode_error(code: str, message: str) -> ValueError:
    return ValueError(f"[{code}] {message}")


def _validate_drag_result(player: "Player", player_count: int) -> None:
    """Keep invalid heats out of time rankings without inventing lap times."""
    result = player.__dict__
    status = result.get("dragStatus")
    if not isinstance(status, str) or status not in {"finished", "false_start", "dnf"}:
        raise _mode_error("DRAG_STATUS_INVALID", "Drag needs a terminal result status")
    for name in ("reactionMs", "elapsedMs", "totalRaceMs"):
        value = result.get(name)
        if type(value) is not int or not 0 <= value <= 120000:
            raise _mode_error("DRAG_TIME_INVALID", f"{name} must be integer milliseconds in 0..120000")
    speed = result.get("trapSpeedKph")
    if type(speed) not in (int, float) or not 0 <= speed <= 1000 or not math.isfinite(speed):
        raise _mode_error("DRAG_SPEED_INVALID", "trapSpeedKph must be finite and in 0..1000")
    if result.get("isBot", False):
        raise _mode_error("DRAG_BOT_FORBIDDEN", "Drag results require human drivers")
    position = player.position
    if status == "finished":
        if result["elapsedMs"] <= 0 or result["totalRaceMs"] != result["reactionMs"] + result["elapsedMs"]:
            raise _mode_error("DRAG_TIME_MISMATCH", "A finished Drag total must equal RT + ET, with ET > 0")
        if result["reactionMs"] < DRAG_MIN_REACTION_MS:
            raise _mode_error("DRAG_REACTION_TOO_FAST",
                              f"A reaction under {DRAG_MIN_REACTION_MS} ms is a false start, not a finish")
        if position is None or not 1 <= position <= player_count:
            raise _mode_error("DRAG_POSITION_INVALID", "A finished Drag needs a valid finishing position")
    elif any(result[name] != 0 for name in ("reactionMs", "elapsedMs", "totalRaceMs", "trapSpeedKph")) or position != 0:
        raise _mode_error("DRAG_INVALID_RESULT", "False starts and DNFs have zero timings, trap speed and position")


class ServerInfo(BaseModel):
    name: str = Field(..., description="Human readable server name")
    host: str = Field(..., description="Host:port of the reporting server")
    build: Optional[str] = Field(
        default=None, description="Build identifier reported by the server"
    )


class Settings(BaseModel):
    g_gametype: int

    class Config:
        extra = "allow"


class Player(BaseModel):
    playerId: str
    displayName: Optional[str]
    team: Optional[str | int]
    rawScore: Optional[int]
    score: Optional[int]
    totalTime: Optional[str]
    position: Optional[int]
    damageDealt: Optional[int]
    damageTaken: Optional[int]

    class Config:
        extra = "allow"

    @root_validator(pre=True)
    def ensure_scores(cls, values: dict[str, Any]) -> dict[str, Any]:
        raw = values.get("rawScore")
        score = values.get("score")
        if raw is None and score is None:
            raise ValueError("Player payload must provide rawScore or score")
        if raw is None:
            values["rawScore"] = score
        elif score is None:
            values["score"] = raw
        return values


class Team(BaseModel):
    team: str
    rawScore: Optional[int]
    normalizedScore: Optional[float]

    class Config:
        extra = "allow"


class Event(BaseModel):
    timestamp: datetime
    type: str

    class Config:
        extra = "allow"


class MatchCreate(BaseModel):
    matchId: str
    mode: Gametype
    startTime: datetime
    endTime: datetime
    duration: Optional[str]
    map: Optional[str]
    server: Optional[ServerInfo]
    serverUrl: Optional[HttpUrl] = Field(
        default=None, description="Optional public URL of the reporting server"
    )
    settings: Settings
    players: list[Player] = Field(..., min_items=1)
    teams: list[Team] | None = None
    events: list[Event] | None = None

    @root_validator(pre=True)
    def validate_drag_raw_positions(cls, values: dict[str, Any]) -> dict[str, Any]:
        mode = values.get("mode")
        if isinstance(mode, str) and mode.strip().upper().replace(" ", "_") in {"DRAG", "DRAG_RACE", "GT_DRAG", "GT_DRAG_RACE"}:
            settings = values.get("settings")
            if isinstance(settings, dict) and (type(settings.get("g_gametype")) is not int or settings["g_gametype"] != 9):
                raise _mode_error("DRAG_GAMETYPE_INVALID", "Drag uses integer g_gametype 9")
            if "gametype" in values and (type(values["gametype"]) is not int or values["gametype"] != 9):
                raise _mode_error("DRAG_GAMETYPE_INVALID", "Drag uses integer gametype 9")
            players = values.get("players")
            if isinstance(players, list):
                for player in players:
                    if isinstance(player, dict) and type(player.get("position")) is not int:
                        raise _mode_error("DRAG_POSITION_INVALID", "Drag positions must be integers")
        return values

    @validator("players")
    def ensure_player_ids(cls, value: list[Player]) -> list[Player]:
        missing_id = any(not player.playerId for player in value)
        if missing_id:
            raise ValueError("All players must provide a playerId")
        return value

    @validator("mode", pre=True)
    def normalize_mode(cls, value: object) -> str:
        if isinstance(value, str):
            candidate = value.strip()
            if candidate:
                upper = candidate.upper().replace(" ", "_")
                if upper in {"DRAG_RACE", "GT_DRAG_RACE"}:
                    return "GT_DRAG"
                if not upper.startswith("GT_"):
                    upper = f"GT_{upper}"
                if upper in _VALID_GAMETYPES:
                    return upper
        return "GT_ELIMINATION"

    @root_validator
    def validate_mode_specific_payload(cls, values: dict[str, Any]) -> dict[str, Any]:
        mode = values.get("mode")
        players = values.get("players") or []
        teams = values.get("teams")

        if mode == "GT_DRAG":
            settings = values.get("settings")
            if settings is not None and settings.g_gametype != 9:
                raise _mode_error("DRAG_GAMETYPE_INVALID", "Drag uses g_gametype 9")
            if "gametype" in values and values["gametype"] != 9:
                raise _mode_error("DRAG_GAMETYPE_INVALID", "Drag uses gametype 9")
            if not 1 <= len(players) <= 2:
                raise _mode_error("DRAG_PLAYER_COUNT_INVALID", "Drag has one practice driver or two competitors")
            if settings is not None:
                duel = getattr(settings, "dragDuel", None)
                if type(duel) is not bool or duel != (len(players) == 2):
                    raise _mode_error("DRAG_DUEL_INVALID", "dragDuel must identify the armed two-driver roster")
            if any(not player.playerId.strip() for player in players) or len({player.playerId for player in players}) != len(players):
                raise _mode_error("DRAG_PLAYER_DUPLICATE", "Drag drivers need distinct player IDs")
            client_numbers = [player.__dict__.get("clientNum") for player in players]
            if any(type(number) is not int or not 0 <= number < 64 for number in client_numbers) or len(set(client_numbers)) != len(client_numbers):
                raise _mode_error("DRAG_CLIENT_INVALID", "Drag driver slots must be distinct integers in 0..63")
            for player in players:
                _validate_drag_result(player, len(players))
            finished = [player for player in players if player.__dict__["dragStatus"] == "finished"]
            for player in finished:
                expected_position = 1 + sum(other.__dict__["totalRaceMs"] < player.__dict__["totalRaceMs"] for other in finished)
                if player.position != expected_position:
                    raise _mode_error("DRAG_POSITION_INVALID", "Drag finishing positions must follow valid total times")
            # Match facts select the winner. A reported slot cannot turn a solo
            # run, tie or invalid result into a competitive victory.
            best = min((player.__dict__["totalRaceMs"] for player in finished), default=0)
            fastest = [player for player in finished if player.__dict__["totalRaceMs"] == best]
            values["winnerClientNum"] = fastest[0].__dict__["clientNum"] if len(players) == 2 and len(fastest) == 1 else -1

        if mode not in _TEAM_MODES:
            if teams:
                raise _mode_error(
                    "MODE_FORBIDS_TEAMS",
                    f"mode '{mode}' forbids teams payload",
                )
            for player in players:
                if player.team is not None:
                    player.team = None
            values["teams"] = None
            return values

        if not teams:
            raise _mode_error(
                "MODE_REQUIRES_TEAMS",
                f"mode '{mode}' requires a non-empty teams array",
            )

        team_names: set[str] = set()
        for team in teams:
            normalized_name = team.team.strip().lower()
            if not normalized_name:
                raise _mode_error("TEAM_NAME_INVALID", "team name must not be blank")
            if normalized_name in team_names:
                raise _mode_error(
                    "TEAM_DUPLICATE",
                    f"duplicate team '{team.team}' in teams payload",
                )
            team.team = normalized_name
            team_names.add(normalized_name)

        for player in players:
            if player.team is None:
                raise _mode_error(
                    "PLAYER_TEAM_REQUIRED",
                    f"player '{player.playerId}' is missing team in mode '{mode}'",
                )
            normalized_player_team = str(player.team).strip().lower()
            if normalized_player_team not in team_names:
                raise _mode_error(
                    "PLAYER_TEAM_UNKNOWN",
                    f"player '{player.playerId}' references unknown team '{player.team}'",
                )
            player.team = normalized_player_team

        return values

    class Config:
        extra = "allow"


class MatchRead(MatchCreate):
    id: int
    createdAt: datetime

    class Config:
        orm_mode = True
