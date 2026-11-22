"""Verify winners and profiles for race reports that reach the PHP service."""

import pytest

from test_php_drag_ladder import PLAYER_A, PLAYER_B, php_ladder


def race_report(mode, *, times, winner=-1, team_times=None):
    team_race = team_times is not None
    return {
        "matchId": "race-outcome",
        "mode": mode,
        "map": "q3r_country01",
        "server": {"name": "Race Test", "dedicated": True},
        "settings": {"winnerClientNum": winner},
        "teamScores": [0, 10, 1, 0, 0, 0],
        "teamTimes": team_times,
        "players": [
            {"clientNum": slot, "playerId": player_id, "cleanName": name,
             "team": slot + 1 if team_race else 0, "totalRaceMs": times[slot],
             "score": 0, "position": slot + 1}
            for slot, (player_id, name) in enumerate(((PLAYER_A, "Alice"), (PLAYER_B, "Bob")))
        ],
    }


@pytest.mark.parametrize("mode", ["GT_RACING", "GT_RACING_DM", "GT_SPRINT"])
@pytest.mark.parametrize("winner", [-1, 0])
def test_dnf_cannot_win_from_progress_or_a_legacy_slot(php_ladder, mode, winner):
    run, _ = php_ladder
    row = run([race_report(mode, times=[0, 0], winner=winner)])[0]
    assert row["index"]["winner"] == ""
    assert all(p["wins"] == p["losses"] == 0 for p in row["profiles"])


def test_valid_legacy_finish_can_still_win_by_position(php_ladder):
    run, _ = php_ladder
    row = run([race_report("GT_RACING", times=[60_000, 0])])[0]
    assert row["index"]["winner"] == "Alice"
    assert row["profiles"][0]["wins"] == row["profiles"][1]["losses"] == 1


@pytest.mark.parametrize("mode", ["GT_TEAM_RACING", "GT_TEAM_RACING_DM"])
def test_equal_team_times_are_a_draw_even_with_different_frag_scores(php_ladder, mode):
    run, _ = php_ladder
    row = run([race_report(mode, times=[60_000, 61_000],
                           team_times=[0, 60_000, 60_000, 0, 0, 0])])[0]
    assert row["index"]["winnerTeam"] == 0
    assert all(p["wins"] == p["losses"] == 0 for p in row["profiles"])


def test_team_racing_dm_uses_the_adjusted_time(php_ladder):
    run, _ = php_ladder
    row = run([race_report("GT_TEAM_RACING_DM", times=[60_000, 61_000],
                           team_times=[0, 55_000, 54_000, 0, 0, 0])])[0]
    assert row["index"]["winnerTeam"] == 2
    assert row["profiles"][0]["losses"] == row["profiles"][1]["wins"] == 1
