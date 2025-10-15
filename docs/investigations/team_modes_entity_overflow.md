# Team-mode entity overflow notes

## Symptom
When running any team-based game type the server eventually aborts with `G_Spawn: no free entities`. The entity dump printed right before the abort shows hundreds of entries still tagged as `noclass`.

## Root cause
`CopyToBodyQue` clones the extra collision bounds (`frontBounds`/`rearBounds`) from the dying vehicle so that the corpse keeps its full collision hull. Body queue slots are recycled in a ring buffer, which means the helper entities from a previous corpse may still exist when the slot is reused. Prior to this fix the code simply overwrote the `body->frontBounds`/`rearBounds` pointers with new `G_Spawn()` results, leaving the old helpers orphaned in the world. The leak quickly exhausts the entity pool in small-team modes where deaths happen in quick succession.

## Fix
Before cloning the new helpers we recycle any lingering `frontBounds`/`rearBounds` entities attached to the body queue slot. If a helper already exists we unlink it and repurpose it for the new corpse; otherwise we spawn one. This keeps the helper count bounded by the body queue size and matches the behaviour from the 0.5c release without introducing scoreboard regressions caused by aggressively freeing the helpers.
