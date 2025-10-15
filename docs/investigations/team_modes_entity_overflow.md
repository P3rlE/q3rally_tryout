# Team gamemode crash log (Oct 15, 2025)

## Observed symptoms
- Dedicated server running `maps/q3r_mk` aborts with `G_Spawn: no free entities`.
- Entity dump immediately before the abort shows hundreds of entries still tagged as `noclass`, meaning their spawn records were never reused or freed.

## Interpretation
- The panic originates from `G_Spawn`, which initialises every freshly allocated entity with the fallback classname `"noclass"` and prints the dump before raising the fatal error once `MAX_GENTITIES` is exhausted.【F:engine/code/game/g_utils.c†L380-L435】
- The Domination crash predates the CTF4 scoring refactor. The only gameplay change in the v0.6 window that touches Domination entities is the Sept 30 2025 merge (`6947ca9`) that introduced automatic sigil registration and map validation helpers.【F:engine/code/game/g_spawn.c†L344-L372】【F:engine/code/game/g_team.c†L602-L729】

## Why commit `6947ca9` is the prime suspect
- `G_CallSpawn` now rewrites every `IT_TEAM` pickup on Domination maps into a sigil powerup and marks it for global broadcast, which was not done in v0.5c.【F:engine/code/game/g_spawn.c†L344-L372】 On layouts that reuse team entities for scripting (as `q3r_mk` does), this dramatically increases the number of always-on Domination helpers.
- After each map load the auto-sigil helper introduced in that merge would create a temporary thinker entity (spawned via `G_Spawn()` with the default `"noclass"` name) that scans the level for convertible items and can promote them into additional sigils.【F:engine/code/game/g_team.c†L666-L729】 These newly introduced helpers match the otherwise unexplained `noclass` rows that dominate the crash dump, making the Sept 30 patch the first regression point after v0.5c.

## Conclusion
- The Oct 15 crash is consistent with the entity-management changes from the Sept 30 auto-sigil patch (`6947ca9`), not with the later CTF4 announcement work. Remediation should focus on reducing or gating the extra Domination helper entities spawned by `G_CallSpawn`/`G_ValidateSigils`, or restoring the simpler v0.5c behaviour when legacy maps are in use.

## Temporary mitigation for investigation
- Disabled the Domination auto-sigil helper so that `G_SpawnEntitiesFromString` no longer spawns the additional `noclass` thinker, reproducing the leaner v0.5c entity lifecycle while testing.【F:engine/code/game/g_spawn.c†L804-L845】
- Skipped the automatic `Team_SetSigilStatus` broadcast for the auto-promoted third sigil to avoid extra persistent Domination state while the helper is disabled.【F:engine/code/game/g_team.c†L713-L729】
