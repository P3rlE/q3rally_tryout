from __future__ import annotations

from datetime import datetime, timezone
import copy
from pathlib import Path
import sys

import pytest
from fastapi.testclient import TestClient
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))

from ladder_service.ladder_service import main
from ladder_service.ladder_service.db import session_scope
from ladder_service.ladder_service.models import Base
from ladder_service.ladder_service.schemas import MatchCreate, _TEAM_MODES, _VALID_GAMETYPES


@pytest.fixture(scope="module", autouse=True)
def override_db(tmp_path_factory: pytest.TempPathFactory) -> None:
    db_path = tmp_path_factory.mktemp("db") / "test.db"
    engine = create_engine(
        f"sqlite:///{db_path}", connect_args={"check_same_thread": False}
    )
    TestingSession = sessionmaker(autocommit=False, autoflush=False, bind=engine)
    Base.metadata.create_all(bind=engine)

    def _override_get_session():
        with session_scope(TestingSession) as session:
            yield session

    main.app.dependency_overrides[main.get_session] = _override_get_session
    yield
    main.app.dependency_overrides.pop(main.get_session, None)


client = TestClient(main.app)


MATCH_TEMPLATE = {
    "matchId": "srv-20240405-183011-42",
    "mode": "GT_RACING",
    "startTime": datetime(2024, 4, 5, 18, 30, 11, tzinfo=timezone.utc).isoformat(),
    "endTime": datetime(2024, 4, 5, 18, 42, 39, tzinfo=timezone.utc).isoformat(),
    "duration": "PT12M28S",
    "startEpoch": 1712332211,
    "map": "q3r_country01",
    "server": {"name": "Q3Rally EU #1", "host": "203.0.113.10:27960", "build": "1.3.0"},
    "settings": {"g_gametype": 141},
    "players": [
        {
            "playerId": "sha256:abc",
            "displayName": "PlayerOne",
            "team": "red",
            "score": 123,
        }
    ],
}


def _build_match(match_id: str, mode: str) -> dict[str, object]:
    payload = {
        **MATCH_TEMPLATE,
        "matchId": match_id,
        "mode": mode,
    }
    if mode in _TEAM_MODES:
        payload["teams"] = [{"team": "red", "rawScore": 10}, {"team": "blue", "rawScore": 8}]
        payload["players"] = [{**MATCH_TEMPLATE["players"][0], "team": "red"}]
    else:
        payload["teams"] = None
        payload["players"] = [{**MATCH_TEMPLATE["players"][0], "team": None}]
    if mode == "GT_DRAG":
        payload["settings"] = {"g_gametype": 9, "dragDuel": False}
        payload["winnerClientNum"] = -1
        payload["players"][0].update({
            "clientNum": 0, "dragStatus": "finished", "reactionMs": 220,
            "elapsedMs": 13000, "totalRaceMs": 13220, "trapSpeedKph": 151.2,
            "position": 1,
        })
    return payload


def _drag_match(match_id: str, *, duel: bool = False) -> dict[str, object]:
    payload = _build_match(match_id, "GT_DRAG")
    payload["settings"]["dragDuel"] = duel
    payload["map"] = "q3r_dragstrip"
    if duel:
        opponent = {**payload["players"][0], "playerId": "sha256:def", "clientNum": 1,
                    "position": 2, "reactionMs": 300, "totalRaceMs": 13300}
        payload["players"].append(opponent)
        payload["winnerClientNum"] = 0
    return payload


def test_create_match() -> None:
    response = client.post("/api/v1/matches", json=MATCH_TEMPLATE)
    assert response.status_code == 201, response.text
    assert response.json() == {"matchId": MATCH_TEMPLATE["matchId"]}


def test_get_match() -> None:
    response = client.get(f"/api/v1/matches/{MATCH_TEMPLATE['matchId']}")
    assert response.status_code == 200
    data = response.json()
    assert data["matchId"] == MATCH_TEMPLATE["matchId"]
    assert "createdAt" in data
    assert data["startEpoch"] == MATCH_TEMPLATE["startEpoch"]
    assert data["players"][0]["rawScore"] == MATCH_TEMPLATE["players"][0]["score"]
    assert data["players"][0]["score"] == MATCH_TEMPLATE["players"][0]["score"]


def test_list_matches() -> None:
    response = client.get("/api/v1/matches?limit=10")
    assert response.status_code == 200
    data = response.json()
    assert len(data["matches"]) >= 1


def test_list_matches_filter_mode() -> None:
    alt_match = {
        **MATCH_TEMPLATE,
        "matchId": "srv-20240405-183011-43",
        "mode": "ARCADE_RACING",
    }
    created = client.post("/api/v1/matches", json=alt_match)
    assert created.status_code == 201, created.text

    response = client.get("/api/v1/matches?mode=GT_RACING")
    assert response.status_code == 200, response.text
    data = response.json()
    assert data["matches"], "Expected at least one GT_RACING match in response"
    for match in data["matches"]:
        assert match["mode"] == "GT_RACING"
        assert match["matchId"] != alt_match["matchId"]

    cleanup = client.delete(f"/api/v1/matches/{alt_match['matchId']}")
    assert cleanup.status_code == 204


def test_list_matches_supports_team_race_dm_mode() -> None:
    match = _build_match("srv-20240405-183011-44", "GT_TEAM_RACING_DM")
    created = client.post("/api/v1/matches", json=match)
    assert created.status_code == 201, created.text

    response = client.get("/api/v1/matches?mode=GT_TEAM_RACING_DM")
    assert response.status_code == 200, response.text
    data = response.json()
    assert any(entry["matchId"] == match["matchId"] for entry in data["matches"])

    cleanup = client.delete(f"/api/v1/matches/{match['matchId']}")
    assert cleanup.status_code == 204


def test_create_match_accepts_sprint_mode() -> None:
    match = {
        **MATCH_TEMPLATE,
        "matchId": "srv-20240405-183011-45",
        "mode": "sprint",
        "settings": {"g_gametype": 145},
    }

    response = client.post("/api/v1/matches", json=match)
    assert response.status_code == 201, response.text

    stored = client.get(f"/api/v1/matches/{match['matchId']}")
    assert stored.status_code == 200, stored.text
    payload = stored.json()
    assert payload["mode"] == "GT_SPRINT"

    cleanup = client.delete(f"/api/v1/matches/{match['matchId']}")
    assert cleanup.status_code == 204


@pytest.mark.parametrize("alias", ["GT_DRAG", "drag", "Drag Race", "gt_drag_race"])
def test_drag_result_round_trips_and_filters_as_its_own_mode(alias: str) -> None:
    match = _drag_match("drag-alias-" + alias.replace(" ", "-"))
    match["mode"] = alias
    created = client.post("/api/v1/matches", json=match)
    assert created.status_code == 201, created.text
    try:
        stored = client.get(f"/api/v1/matches/{match['matchId']}")
        assert stored.status_code == 200, stored.text
        data = stored.json()
        assert data["mode"] == "GT_DRAG"
        assert data["settings"] == {"g_gametype": 9, "dragDuel": False}
        assert data["winnerClientNum"] == -1
        for field in ("dragStatus", "reactionMs", "elapsedMs", "totalRaceMs", "trapSpeedKph", "position"):
            assert data["players"][0][field] == match["players"][0][field]
        for filter_alias in ("GT_DRAG", "drag race"):
            filtered = client.get("/api/v1/matches", params={"mode": filter_alias})
            assert filtered.status_code == 200, filtered.text
            assert any(row["matchId"] == match["matchId"] for row in filtered.json()["matches"])
            assert all(row["mode"] == "GT_DRAG" for row in filtered.json()["matches"])
    finally:
        client.delete(f"/api/v1/matches/{match['matchId']}")


@pytest.mark.parametrize("status", ["false_start", "dnf"])
def test_drag_keeps_invalid_results_without_fabricated_times(status: str) -> None:
    match = _drag_match("drag-invalid-" + status, duel=True)
    player = match["players"][1]
    player.update({"dragStatus": status, "reactionMs": 0, "elapsedMs": 0,
                   "totalRaceMs": 0, "trapSpeedKph": 0, "position": 0})
    created = client.post("/api/v1/matches", json=match)
    assert created.status_code == 201, created.text
    try:
        stored = client.get(f"/api/v1/matches/{match['matchId']}").json()
        assert stored["players"][1]["dragStatus"] == status
        assert stored["players"][1]["totalRaceMs"] == 0
        assert stored["players"][1]["position"] == 0
        assert stored["winnerClientNum"] == 0
    finally:
        client.delete(f"/api/v1/matches/{match['matchId']}")


@pytest.mark.parametrize(("field", "value", "code"), [
    ("dragStatus", "running", "DRAG_STATUS_INVALID"),
    ("dragStatus", [], "DRAG_STATUS_INVALID"),
    ("dragStatus", {}, "DRAG_STATUS_INVALID"),
    ("reactionMs", -1, "DRAG_TIME_INVALID"),
    ("reactionMs", True, "DRAG_TIME_INVALID"),
    ("reactionMs", "220", "DRAG_TIME_INVALID"),
    ("elapsedMs", 0, "DRAG_TIME_MISMATCH"),
    ("elapsedMs", 13000.5, "DRAG_TIME_INVALID"),
    ("totalRaceMs", 13221, "DRAG_TIME_MISMATCH"),
    ("totalRaceMs", 120001, "DRAG_TIME_INVALID"),
    ("trapSpeedKph", -1, "DRAG_SPEED_INVALID"),
    ("trapSpeedKph", 1000.1, "DRAG_SPEED_INVALID"),
    ("trapSpeedKph", "151.2", "DRAG_SPEED_INVALID"),
    ("position", 0, "DRAG_POSITION_INVALID"),
    ("position", 2, "DRAG_POSITION_INVALID"),
    ("position", 1.5, "DRAG_POSITION_INVALID"),
    ("position", True, "DRAG_POSITION_INVALID"),
    ("isBot", True, "DRAG_BOT_FORBIDDEN"),
])
def test_drag_rejects_invalid_result_facts_before_storage(field: str, value: object, code: str) -> None:
    match = _drag_match("drag-reject-" + field)
    match["players"][0][field] = value
    rejected = client.post("/api/v1/matches", json=match)
    assert rejected.status_code == 422, rejected.text
    assert code in rejected.json()["validation"]["errorCodes"]
    assert client.get(f"/api/v1/matches/{match['matchId']}").status_code == 404


def test_drag_accepts_minimum_reaction_and_boundary_frame_finish() -> None:
    for reaction, elapsed in ((100, 13000), (101, 60050), (60000, 60000)):
        match = _drag_match(f"drag-boundary-{reaction}-{elapsed}")
        match["players"][0].update({"reactionMs": reaction, "elapsedMs": elapsed,
                                    "totalRaceMs": reaction + elapsed})
        created = client.post("/api/v1/matches", json=match)
        assert created.status_code == 201, created.text
        client.delete(f"/api/v1/matches/{match['matchId']}")


@pytest.mark.parametrize("reaction", [0, 1, 99])
def test_drag_rejects_a_finish_faster_than_the_minimum_reaction(reaction: int) -> None:
    match = _drag_match(f"drag-too-fast-{reaction}")
    elapsed = match["players"][0]["elapsedMs"]
    match["players"][0].update({"reactionMs": reaction, "totalRaceMs": reaction + elapsed})
    rejected = client.post("/api/v1/matches", json=match)
    assert rejected.status_code == 422, rejected.text
    assert "DRAG_REACTION_TOO_FAST" in rejected.json()["validation"]["errorCodes"]
    assert client.get(f"/api/v1/matches/{match['matchId']}").status_code == 404


def test_drag_rejects_invalid_times_and_prevents_solo_wins() -> None:
    match = _drag_match("drag-bad-invalid-time")
    match["players"][0]["dragStatus"] = "false_start"
    rejected = client.post("/api/v1/matches", json=match)
    assert rejected.status_code == 422, rejected.text
    assert "DRAG_INVALID_RESULT" in rejected.json()["validation"]["errorCodes"]
    match = _drag_match("drag-bad-solo-win")
    match["winnerClientNum"] = 0
    created = client.post("/api/v1/matches", json=match)
    assert created.status_code == 201, created.text
    stored = client.get(f"/api/v1/matches/{match['matchId']}").json()
    assert stored["winnerClientNum"] == -1
    client.delete(f"/api/v1/matches/{match['matchId']}")


def test_drag_validates_roster_duel_flag_and_winner() -> None:
    base = _drag_match("drag-bad-roster", duel=True)
    mutations = [
        (lambda p: p["settings"].update(g_gametype=3), "DRAG_GAMETYPE_INVALID"),
        (lambda p: p["settings"].update(g_gametype=9.0), "DRAG_GAMETYPE_INVALID"),
        (lambda p: p["settings"].update(dragDuel=False), "DRAG_DUEL_INVALID"),
        (lambda p: p["settings"].pop("dragDuel"), "DRAG_DUEL_INVALID"),
        (lambda p: p["players"].append(copy.deepcopy(p["players"][1])), "DRAG_PLAYER_COUNT_INVALID"),
        (lambda p: p["players"][1].update(playerId=p["players"][0]["playerId"]), "DRAG_PLAYER_DUPLICATE"),
        (lambda p: p["players"][1].update(playerId=" \t"), "DRAG_PLAYER_DUPLICATE"),
        (lambda p: p["players"][1].update(clientNum=0), "DRAG_CLIENT_INVALID"),
        (lambda p: p["players"][1].pop("clientNum"), "DRAG_CLIENT_INVALID"),
        (lambda p: p["players"][1].update(position=1), "DRAG_POSITION_INVALID"),
    ]
    for mutate, code in mutations:
        match = copy.deepcopy(base)
        mutate(match)
        rejected = client.post("/api/v1/matches", json=match)
        assert rejected.status_code == 422, rejected.text
        assert code in rejected.json()["validation"]["errorCodes"]


def test_drag_tied_duel_has_no_winner() -> None:
    match = _drag_match("drag-tie", duel=True)
    match["players"][1].update(totalRaceMs=13220, reactionMs=220, position=1)
    match["winnerClientNum"] = 0
    created = client.post("/api/v1/matches", json=match)
    assert created.status_code == 201, created.text
    stored = client.get(f"/api/v1/matches/{match['matchId']}").json()
    assert stored["winnerClientNum"] == -1
    client.delete(f"/api/v1/matches/{match['matchId']}")


def test_drag_winner_comes_from_total_time_instead_of_reported_slot() -> None:
    match = _drag_match("drag-normalize-winner", duel=True)
    match["winnerClientNum"] = 1
    created = client.post("/api/v1/matches", json=match)
    assert created.status_code == 201, created.text
    stored = client.get(f"/api/v1/matches/{match['matchId']}").json()
    assert stored["winnerClientNum"] == 0
    client.delete(f"/api/v1/matches/{match['matchId']}")


@pytest.mark.parametrize("speed", [float("nan"), float("inf"), -float("inf"), 10**500])
def test_drag_schema_rejects_nonfinite_and_oversized_trap_speeds(speed: object) -> None:
    match = _drag_match("drag-bad-speed")
    match["players"][0]["trapSpeedKph"] = speed
    with pytest.raises(ValueError, match="DRAG_SPEED_INVALID"):
        MatchCreate.parse_obj(match)


def test_mode_forbids_teams_with_validation_codes() -> None:
    match = _build_match("srv-20240405-183011-46", "GT_RACING")
    match["teams"] = [{"team": "red", "rawScore": 1}]

    response = client.post("/api/v1/matches", json=match)
    assert response.status_code == 422, response.text
    data = response.json()
    assert "MODE_FORBIDS_TEAMS" in data["validation"]["errorCodes"]
    assert any(item["errorCode"] == "MODE_FORBIDS_TEAMS" for item in data["detail"])


def test_team_mode_requires_teams() -> None:
    match = _build_match("srv-20240405-183011-47", "GT_TEAM")
    match["teams"] = None

    response = client.post("/api/v1/matches", json=match)
    assert response.status_code == 422, response.text
    assert "MODE_REQUIRES_TEAMS" in response.json()["validation"]["errorCodes"]


def test_team_mode_rejects_unknown_player_team() -> None:
    match = _build_match("srv-20240405-183011-48", "GT_CTF")
    match["players"][0]["team"] = "spectator"

    response = client.post("/api/v1/matches", json=match)
    assert response.status_code == 422, response.text
    assert "PLAYER_TEAM_UNKNOWN" in response.json()["validation"]["errorCodes"]


def test_non_team_mode_sanitizes_player_team() -> None:
    match = _build_match("srv-20240405-183011-49", "GT_ELIMINATION")
    match["players"][0]["team"] = "red"

    response = client.post("/api/v1/matches", json=match)
    assert response.status_code == 201, response.text

    stored = client.get(f"/api/v1/matches/{match['matchId']}")
    assert stored.status_code == 200, stored.text
    data = stored.json()
    assert data["players"][0]["team"] is None
    assert data["teams"] is None

    cleanup = client.delete(f"/api/v1/matches/{match['matchId']}")
    assert cleanup.status_code == 204


@pytest.mark.parametrize("mode", sorted(_VALID_GAMETYPES))
def test_all_modes_accept_mode_specific_payload(mode: str) -> None:
    match_id = f"srv-20240405-183011-{mode.lower()}"
    match = _build_match(match_id, mode)

    created = client.post("/api/v1/matches", json=match)
    assert created.status_code == 201, f"{mode}: {created.text}"

    stored = client.get(f"/api/v1/matches/{match_id}")
    assert stored.status_code == 200, f"{mode}: {stored.text}"
    payload = stored.json()
    if mode in _TEAM_MODES:
        assert payload["teams"] is not None
        assert payload["players"][0]["team"] == "red"
    else:
        assert payload["teams"] is None
        assert payload["players"][0]["team"] is None

    cleanup = client.delete(f"/api/v1/matches/{match_id}")
    assert cleanup.status_code == 204


def test_delete_match() -> None:
    response = client.delete(f"/api/v1/matches/{MATCH_TEMPLATE['matchId']}")
    assert response.status_code == 204
    follow_up = client.get(f"/api/v1/matches/{MATCH_TEMPLATE['matchId']}")
    assert follow_up.status_code == 404
