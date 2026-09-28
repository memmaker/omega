# Omega 0.80.2 — RVIP import (2026-09-26)

Case O (curses, no Rogue/Moria lineage). `git log`: upstream, then the port.

- Build: `make -f port/Makefile` (X11) · web: `sh web/build.sh`, `web/deploy.sh`.
- Run: `./play.sh` or `~/Desktop/Games/Roguelikes/Omega.app`. Saves in `save/`.
- Port: `port/` (curses shim, `be_x11.c`, `be_web.c`), `rl.c` (explore `X`,
  stairs `<`/`>`, Enter menu, inventory item menu), small hooks in
  command1.c, inv.c (`getitem` cursor, `inventory_control` keys), scr.c,
  save.c, command2.c, char.c, defs.h. Help: `omegalib/help12.txt`.
- Choice menus: `rl_choose` / `rl_choose_keys` / `rl_menu` / `rl_ask`
  (rl.c) show every multiple-choice question as a list in a box at the
  right (under the message rows, clear of Menuw), picked by key or
  arrows/8/2 (9/3 a page) + Enter/space/5. `rl_menu("title", "k:text|...")`
  labels each entry `k) text`, takes k (letters either case unless the keys
  differ only by case) and returns it (ESCAPE for none), so the call sites
  keep their switches and old key sequences still work. An entry keyed
  `\n` starts highlighted (F: Enter = Done). Keys win over movement keys
  (NPC behaviour menu 1-5: there the down arrow of X11/web, sent as '2',
  picks 2). No old keys (M, casting, item/monster lists): a) b) c) ...,
  then A-Z. `rl_ask`: must answer. Used by M, casting, shops, guilds, bank,
  altars, maneuvers (F, getlocation), wishes, portals, the top-line
  inventory (actions, slots a-o/*, pack A..), etc. y/n questions and free
  text (names, amounts) are unchanged.
- be_term.c: Enter is '\n' like X11 and the web (was '\r': name entry
  never ended in the terminal build).
- Web tiles: David Kinder's WinOmega 32x32 sheet (github.com/DavidKinder/Omega
  fae6f21, `32x32.bmp` -> `web/tiles.png`). C picks the tile: `port/tiles.c`
  (`wc_tile`, table = Kinder's `gfxMapData` in `port/map.inc`, plus his
  non-countryside cases: cold blast, incubus/satyr). Levelw cells carry
  `A_TILE` + tile in bits 18+; omega.js only blits, scrolled round the player;
  *Tiles* button, `localStorage`. `drawFull()` blits the A_TILE cells
  (`drawTiles`, T = ch wide, scrolled by `tox`), then every other cell as
  text at x*cw on black: menus and lists over the map (Enter menu, choice
  menus) stay text, never tile-wide. The camera (`RvipWM.center`) gets the
  hero where it is drawn: `(hero.x - tox) * ch` on a tile, else `x * cw`. X11 (`be_x11.c`) draws the same from
  `port/tiles.bmp` (Kinder's BMP as is); `OMEGA_TILES=0 ./play.sh` = text.
- Web messages: `morewait()` never waits (`auto_more`, RVIP 3d); the message
  history is the log window, the live message rows go to `RvipWM.prompt`
  (rvip-wm.js, `js_key(at_cmd)` hides it on a command key). `exit()` is
  `wc_exit` (`-Dexit=` in build.sh): Emscripten runs no atexit handlers, so
  it shows the game-over overlay, unlinks the autosave and idles.
- Web zoom is rvip-wm.js's (memmaker/rvip f4d0f76, RVIP.md W4): one size
  per window and mode (`state.fs` multi, `state.fs1` one window).
  `layout()` sets `px = wm.zoomed('map') || fit()`: one window fits the
  whole screen (side panel, messages, lists) unless zoomed there; multi
  keeps its own map zoom. `size.map` = drawn px, `fontMax.map` = 40. No
  zoom of its own (old layout `px` -> `wm.fs.map` on load). One window
  has no title bar, so no A-/A+ there: it always fits.
- web/build.sh: the help page step needs `~/Desktop/Games/Roguelikes/Docs/
  build-docs.py` (not in a repo); without it help.html stays empty.
- Live: https://ruzzoli.de/roguelikes/omega/ (deployed 2026-09-26; the
  choice menus and web/WM changes of 2026-09-28 are not deployed yet:
  index deploy.sh for rvip-wm.js, then `web/build.sh && web/deploy.sh`).
- Not done: sound (6b), mouse.
- Tested: char creation, city, countryside travel, temple explore + doors,
  menu, item menu quaff + reopen, save/restore, ASan run (clean), web
  menu/autosave/restore in the browser, starving to death in town (RIP
  screen, game-over overlay, fresh game on reload). 2026-09-28, terminal
  build in a pty: char creation menus, M, bank, F + locations, activate,
  top-line inventory (actions, slots, *). Web (local build, headless
  Chromium): Enter menu over the tiles, map zoom +6, one window and back
  (multi zoom kept, one window fits), camera on the walking player.
  Not tested: X11 build (no Xft here).
