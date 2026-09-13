# STATUS — Minesweeper (Orx / Android)

Last updated: 2026-09-13 ~11:54 (America/New_York)

## Current milestone

**APK v3 on S22 — menu UI blocker.** Title vertically stretched; difficulty buttons too small / untappable. **Next:** menu layout + hit-test fix → rebuild APK. Repo: `awschult002/minesweeper` (Chief pushing).

| Owner | Work |
| --- | --- |
| Senior | Title square pixels (no vertical stretch); circular-button-scale menu layout + hit-test |
| Dev | Difficulty button AABBs / Play flow so taps register |
| Tester | Menu fixtures: square title; Beginner/Int/Expert/Play hit; no false start |
| Scribe | This STATUS |
| Chief | GitHub push + DM next APK after fix |


## Dev work (v3) — done in-tree

- **Menu confirm:** ParentCamera-normalized hit-test (no 40×24 world AABB). Tap Beginner/Intermediate/Expert → highlight; tap **Play** → `StartGame`. MENU touches only on `TOUCH_END`; never routed to `touch_bridge`.
- **Default zoom:** `SetDefaultPlayableZoom` — `base_view_w = 9 * cell_size`, portrait aspect, `zoom = 1` (below Senior max). Center = board middle. Not fit-all.
- Difficulties unchanged; `mines_place_random` untouched; frustum 720×1280 kept.
- Assets synced `data/` → `build/android/.../assets/`.

## v3 on-device findings (Alex) — menu blocker

1. Title still **vertically stretched** — fix object/title scale (keep square pixels); do **not** stretch the framebuffer
2. Difficulty buttons **too small vs title** and **untappable** / taps no-op — enlarge hit targets; fix ParentCamera hit-test; coherent portrait menu layout
3. Pan/zoom on menu **not required** if scale/hit-test are correct

Repo: GitHub `awschult002/minesweeper` (push in progress).

## v2 on-device findings (Alex)

1. Portrait via stretch/letterbox — **wrong**; keep desktop pixel/aspect pipeline; make **playfield + camera frustum** portrait
2. Menu tap → jumped into grid then zoomed out — need intentional Beginner/etc confirm
3. Can zoom out, **cannot zoom in** — fix clamps; default zoom = tappable cells after difficulty
4. HUD mode button must be **screen-space overlay** (camera-independent), circular icons (mine / flag / question), working hit-test

## Input / HUD (locked — refined)

| Action | Behavior |
| --- | --- |
| Mode toggle | **Circular screen-space** button, bottom-right; cycles Reveal→Flag→Question; icons mine / flag / `?` |
| Single-tap closed | Current mode |
| Chord | Tap revealed N when flags==N (`?` ignored in \|F\|); via `mines_chord` |
| Pan / zoom | Drag / pinch; **zoom min/max** allow zoom-in; camera clamped to playfield |
| Portrait | Taller **world frustum** — **no** nonuniform backbuffer scale |
| Dropped | 2-finger flag; raw text HUD label |

## Oracle contract (unchanged)

- `?` does not count toward chord \|F\|; chord may open/clear `?`; only via `mines_chord`

## Tester gates

### v3 camera (prior — **298+24**)
1. HUD hit at pan/zoom extremes — still on-device / helper gap
2. Zoom-in from default — **pass**
3. Portrait aspect under zoom — **pass**

### Menu (next green — before playable)
1. **Square title** — scale X==Y (no vertical stretch) under portrait frustum
2. **Hit targets** — Beginner / Intermediate / Expert / Play register at laid-out screen positions (ParentCamera local pick, not world AABB)
3. **No false start** — tap outside buttons doesn’t start; Play with no difficulty selected is a no-op

### Re-smoke on S22 (after menu-fix APK)
- [ ] Title not stretched; difficulty buttons tappable + Play starts
- [ ] Menu pick → **Play** (no auto-dive / no false start)
- [ ] Cells tappable; zoom in; pan clamp; circular HUD at edge
- [ ] Chord / flag / `?` / win-lose OK

## Carry-forward

- GitHub: `https://github.com/awschult002/minesweeper`
- Color-wheel cell tint still deferred
- Prior: TOUCH_* bridge, headless rules, v2 APK delivered

## Open
- [x] APK v3 delivered (**298+24**); camera/HUD/menu-confirm in-tree
- [ ] Menu: square title + enlarged difficulty hit targets + ParentCamera pick
- [ ] Tester menu fixtures green
- [ ] Rebuild APK → Alex re-smoke
- [ ] Color blend + settings color wheel
- [ ] Audio (non-blocking)
- [x] GitHub: `awschult002/minesweeper` (Chief pushing)

## Decision log
| When | Lock |
| --- | --- |
| 2026-09-08 | HUD modes; chord = tap revealed N; `?` out of \|F\|; camera clamp |
| 2026-09-08 | APK v2; fixtures 298+21 |
| 2026-09-08 | No letterbox stretch — portrait via frustum; screen-space circular HUD; zoom-in required; deliberate menu confirm |
| 2026-09-08 | Senior v3: base_view/zoom(0.5–4) playable default; circular ParentCamera HUD + screen hit; portrait frustum no stretch |
| 2026-09-08 | v3 camera/HUD: zoom factor 0.5–4, circular HUD screen hit; suite 298+24; menu+APK still Dev |
| 2026-09-08 | APK v3: menu Play confirm, ~9-cell default zoom, circular HUD; 298+24; on-device smoke open |
| 2026-09-13 | v3 menu blocker: square title, larger difficulty hits, no false start; GitHub awschult002/minesweeper |
