# Omega Rebirth v0.7.0 — RVIP port (web only)

Case O (curses, many fixed windows). Upstream: github.com/Lyle-Tafoya/Omega
a83c9d7 ("omega rebirth version 0.7.0", C++23), imported 2026-09-30 over the
old 0.80.2 port (history kept in `git log`). No native frontends any more.

- Layout: `src/` rebirth + port hooks, `lib/` game data (was `omegalib/`),
  `port/` curses shim (`curses.h`, `wcurses.c`, `be_web.c`, `tiles.cpp`,
  `map.inc`, `gromega.inc`), `web/` page, build, deploy.
- Build: `sh web/build.sh` (em++ from Homebrew) -> `web/dist`. Objects cached
  in `build-web/`; header changes are not tracked: `rm -rf build-web` after
  editing a header. help.html needs `~/Desktop/Games/Roguelikes/Docs/build-docs.py`
  (not in a repo; `web/make-help.py` adds the web parts + FAQ/hints/spoilers).
- Deploy: `web/deploy.sh` (refuses a dirty or unpushed tree) -> 
  https://ruzzoli.de/roguelikes/omega/. The page loads `../rvip-wm.js` (one
  copy, rvip-tools; never copy it).
- Local test: a folder of symlinks (`omega` -> `web/dist`, `rvip-wm.js`,
  `rvip-app.js`, `rvip-sound.js` -> rvip-tools/web, `fonts`), `python3 -m
  http.server 8765`, http://localhost:8765/omega/index.html.
- Native check only: `sh build-term.sh` (ncurses, `./omega-term`).
- Wizard mode (`^g`) needs user name == `WIZARD` ("wtanksle", defs.h); the
  web user is "player". For testing build once with
  `'-DWIZARD=\"player\"'` in CXXFLAGS; never commit that. In wizard mode `M`
  lists every city site.

## Port (src/rl.cpp + hooks)
- `X` explore (paints each step), `<`/`>` walk to known stairs, Enter
  command menu (help12/13).
- Choice menus `rl_choose` / `rl_choose_keys` / `rl_menu` / `rl_ask`: M,
  casting, shops, guilds, bank, altars, F + locations, wishes, portals,
  NPC behaviour ... y/n and free text unchanged.
- Inventory: one `i` list (slots + pack, A = top of pack) -> item menu
  use/equip/take off/drop/call; `w` equip from pack, `W` take off, `d` drops
  pack or worn items; every item prompt offers pack items (held in slot 0
  for the command, `rl_return_held`, rummage time); `g` on a pile =
  checklist; auto-pickup only money and stacking items.
- Web: log window from the message history (`rl_messages`; colour markup
  `|x` stripped, rebirth's own "text xN" folded into "text (xN)"), live
  message rows -> `RvipWM.prompt`, Inventory/Visible lists, autosave at the
  command prompt (every 2 s), `wc_exit` game-over overlay (deletes the
  autosave unless saved with S), run-report beacon.
- Pop-ups (any game window over a pane: menus, item menu, checklists,
  shops, prompts): `doupdate` passes the box of its text to `be_popup`;
  multi-window draws only that box in `#full`, placed by `RvipWM.popup()`
  over the unchanged layout (one-window shows the whole screen anyway).
- Pauses: `omega_sleep()` (defs.h) = sleep_for natively, on the web
  `doupdate` + `emscripten_sleep` (casino reels, bank, ... animate).
- Tiles: `Wc_tileset` 1 = David Kinder's WinOmega 32x32 sheet
  (`web/tiles.png`, `map.inc` = his gfxMapData), 2 = gromega 0.80.2a
  (`omegalib/omegatiles.xpm` from
  www.alcyone.com/binaries/omega/gromega-0.80.2a-src.tar.gz, degridded to
  `web/gromega.png`, 8x12, code = row*32+col; `gromega.inc` = monster name
  -> code from its minit.h/ochars.h; terrain and item classes by hand in
  `tiles.cpp`). Joining as gromega's truetiles.c: walls/hedges/water pick
  one of 47 pieces from the 8 neighbours the map window shows (`gjoin`,
  wcurses.c `tilecell`; walls join doors and blank cells; a refresh
  retiles the neighbours of changed cells); the countryside takes
  gromega's hand-drawn country.dat per cell (`port/gromega-country.inc`,
  {tile, tile while a pass/site is hidden}). Portcullis: gromega's V tile
  when walls are north and south, else H (gromega set them per map; the
  arena gate = V as there); doors have one open/closed tile in gromega too. Cells keyed by char + PC colour.
  omega.js cycles None/WinOmega/gromega (IndexedDB `web-tiles`),
  `be_getkey` polls `Module.om.tileset()`, `wc_retile()` redraws the map.
- Rebirth fixes kept: F default sequence cut in half, SUPPRESS_PRINTING on
  restore, scrolling_buffer "_ " sentinel.

## Tested (2026-09-30, local build in the built-in browser)
Char creation (point-buy arrows), city, M (all sites in wizard mode),
casino slots (reels animate), Julie's weapon purchase, pawn shop sell from
pack, tavern (closed by day), paladins and mercenaries (join/refuse), bank
menu, F maneuvers menu + aim, Export (Player.sav download) and Import
(replaces, restores), save/autosave/restore, death overlay, tile cycling
None/WinOmega/gromega + persistence, log without colour markup (the
"x2 (x2)" fold not seen live). Inventory stage: pile pickup, pack items in prompts, w/W, d, i menu.
Not clicked: altar menus, bank crash animation, thieves'/college guilds.

## Open
- gromega joining checked in the city, countryside (magic-mapped) and
  WoodHenge; no random dungeon level clicked.
- Presentation rule 6 (text windows as HTML, only the map a canvas).
- Sound (stage 6 search for upstream audio), mouse.
