from __future__ import annotations

from datetime import datetime, timezone
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

SERVER_HEADERS = {"Authorization": "Bearer dev-server-key"}


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


def test_create_match_requires_server_key() -> None:
    response = client.post("/api/v1/matches", json=MATCH_TEMPLATE)
    assert response.status_code == 401


def test_create_match() -> None:
    response = client.post("/api/v1/matches", json=MATCH_TEMPLATE, headers=SERVER_HEADERS)
    assert response.status_code == 201, response.text
    assert response.json() == {"matchId": MATCH_TEMPLATE["matchId"]}


def test_get_match() -> None:
    response = client.get(f"/api/v1/matches/{MATCH_TEMPLATE['matchId']}", headers=SERVER_HEADERS)
    assert response.status_code == 200
    data = response.json()
    assert data["matchId"] == MATCH_TEMPLATE["matchId"]
    assert "createdAt" in data
    assert data["startEpoch"] == MATCH_TEMPLATE["startEpoch"]
    assert data["players"][0]["rawScore"] == MATCH_TEMPLATE["players"][0]["score"]
    assert data["players"][0]["score"] == MATCH_TEMPLATE["players"][0]["score"]


def test_list_matches() -> None:
    response = client.get("/api/v1/matches?limit=10", headers=SERVER_HEADERS)
    assert response.status_code == 200
    data = response.json()
    assert len(data["matches"]) >= 1


def test_list_matches_filter_mode() -> None:
    alt_match = {
        **MATCH_TEMPLATE,
        "matchId": "srv-20240405-183011-43",
        "mode": "ARCADE_RACING",
    }
    created = client.post("/api/v1/matches", json=alt_match, headers=SERVER_HEADERS)
    assert created.status_code == 201, created.text

    response = client.get("/api/v1/matches?mode=GT_RACING", headers=SERVER_HEADERS)
    assert response.status_code == 200, response.text
    data = response.json()
    assert data["matches"], "Expected at least one GT_RACING match in response"
    for match in data["matches"]:
        assert match["mode"] == "GT_RACING"
        assert match["matchId"] != alt_match["matchId"]

    cleanup = client.delete(f"/api/v1/matches/{alt_match['matchId']}", headers=SERVER_HEADERS)
    assert cleanup.status_code == 204


def test_list_matches_supports_team_race_dm_mode() -> None:
    match = {
        **MATCH_TEMPLATE,
        "matchId": "srv-20240405-183011-44",
        "mode": "GT_TEAM_RACING_DM",
    }
    created = client.post("/api/v1/matches", json=match, headers=SERVER_HEADERS)
    assert created.status_code == 201, created.text

    response = client.get("/api/v1/matches?mode=GT_TEAM_RACING_DM", headers=SERVER_HEADERS)
    assert response.status_code == 200, response.text
    data = response.json()
    assert any(entry["matchId"] == match["matchId"] for entry in data["matches"])

    cleanup = client.delete(f"/api/v1/matches/{match['matchId']}", headers=SERVER_HEADERS)
    assert cleanup.status_code == 204


def test_create_match_accepts_sprint_mode() -> None:
    match = {
        **MATCH_TEMPLATE,
        "matchId": "srv-20240405-183011-45",
        "mode": "sprint",
        "settings": {"g_gametype": 145},
    }

    response = client.post("/api/v1/matches", json=match, headers=SERVER_HEADERS)
    assert response.status_code == 201, response.text

    stored = client.get(f"/api/v1/matches/{match['matchId']}", headers=SERVER_HEADERS)
    assert stored.status_code == 200, stored.text
    payload = stored.json()
    assert payload["mode"] == "GT_SPRINT"

    cleanup = client.delete(f"/api/v1/matches/{match['matchId']}", headers=SERVER_HEADERS)
    assert cleanup.status_code == 204


def test_delete_match() -> None:
    response = client.delete(f"/api/v1/matches/{MATCH_TEMPLATE['matchId']}", headers=SERVER_HEADERS)
    assert response.status_code == 204
    follow_up = client.get(f"/api/v1/matches/{MATCH_TEMPLATE['matchId']}", headers=SERVER_HEADERS)
    assert follow_up.status_code == 404


def test_register_login_refresh_and_profile_flow() -> None:
    register = client.post(
        "/api/v1/auth/register",
        json={"user_id": "racer_1", "password": "TopSecret42"},
    )
    assert register.status_code == 200, register.text
    register_data = register.json()
    assert "access_token" in register_data
    assert "refresh_token" in register_data

    login = client.post(
        "/api/v1/auth/login",
        json={"user_id": "racer_1", "password": "TopSecret42"},
    )
    assert login.status_code == 200, login.text

    user_headers = {"Authorization": f"Bearer {login.json()['access_token']}"}
    profile = client.get("/api/v1/profile", headers=user_headers)
    assert profile.status_code == 200
    assert profile.json()["user_id"] == "racer_1"

    avatar_update = client.put(
        "/api/v1/profile/avatar",
        headers=user_headers,
        json={"avatar_ref": "avatars/racer_1.png"},
    )
    assert avatar_update.status_code == 200
    assert avatar_update.json()["profile"]["avatar_ref"] == "avatars/racer_1.png"

    refresh = client.post("/api/v1/auth/refresh", json={"refresh_token": login.json()["refresh_token"]})
    assert refresh.status_code == 200, refresh.text


def test_auth_rate_limit() -> None:
    for _ in range(20):
        response = client.post(
            "/api/v1/auth/login",
            json={"user_id": "nobody", "password": "wrong-password"},
        )
        assert response.status_code in {401, 429}

    blocked = client.post(
        "/api/v1/auth/login",
        json={"user_id": "nobody", "password": "wrong-password"},
    )
    assert blocked.status_code == 429
